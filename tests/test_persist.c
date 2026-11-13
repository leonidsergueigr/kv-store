#include "kv.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned g_checks = 0;
static unsigned g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            fprintf(stderr, "ECHEC %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)


#define PATH_MAIN "build/test_persist_main.db"
#define PATH_BAD "build/test_persist_bad.db"
#define PATH_BIG "build/test_persist_big.db"

static long file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long n = ftell(f);
    fclose(f);
    return n;
}

static int write_raw_file(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (f == NULL)
        return 0;
    int ok = (len == 0) ? 1 : (fwrite(data, 1, len, f) == len);
    if (fclose(f) != 0)
        ok = 0;
    return ok;
}

static int copy_prefix(const char *src, const char *dst, size_t keep) {
    FILE *in = fopen(src, "rb");
    if (in == NULL)
        return 0;
    FILE *out = fopen(dst, "wb");
    if (out == NULL) {
        fclose(in);
        return 0;
    }
    int ok = 1;
    size_t left = keep;
    char buf[4096];
    while (left > 0) {
        size_t chunk = left < sizeof buf ? left : sizeof buf;
        size_t got = fread(buf, 1, chunk, in);
        if (got != chunk || fwrite(buf, 1, got, out) != got) {
            ok = 0;
            break;
        }
        left -= got;
    }
    fclose(in);
    if (fclose(out) != 0)
        ok = 0;
    return ok;
}

static int read_first_pair(const char *path, char *key, size_t keycap,
                           char *value, size_t valcap) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return 0;
    uint32_t klen = 0;
    uint32_t vlen = 0;
    int ok = 1;
    if (fread(&klen, 1, sizeof klen, f) != sizeof klen)
        ok = 0;
    else if ((size_t)klen + 1 > keycap ||
             fread(key, 1, klen, f) != klen)
        ok = 0;
    else {
        key[klen] = '\0';
        if (fread(&vlen, 1, sizeof vlen, f) != sizeof vlen)
            ok = 0;
        else if ((size_t)vlen + 1 > valcap ||
                 fread(value, 1, vlen, f) != vlen)
            ok = 0;
        else
            value[vlen] = '\0';
    }
    fclose(f);
    return ok;
}

static int patch_file(const char *path, long offset,
                      const unsigned char *bytes, size_t n) {
    FILE *f = fopen(path, "r+b");
    if (f == NULL)
        return 0;
    int ok = 1;
    if (fseek(f, offset, SEEK_SET) != 0)
        ok = 0;
    else if (fwrite(bytes, 1, n, f) != n)
        ok = 0;
    if (fclose(f) != 0)
        ok = 0;
    return ok;
}
static KVStore *g_dest;
static unsigned g_cmp_total;
static unsigned g_cmp_missing;
static unsigned g_cmp_wrong;

static void cmp_entry(const char *key, const char *value) {
    g_cmp_total++;
    char *other = kv_get(g_dest, key);
    if (other == NULL) {
        g_cmp_missing++;
    } else if (strcmp(other, value) != 0) {
        g_cmp_wrong++;
    }
}

static unsigned g_count;
static void count_entry(const char *key, const char *value) {
    (void)key;
    (void)value;
    g_count++;
}

static unsigned count_entries(KVStore *store) {
    g_count = 0;
    kv_foreach(store, count_entry);
    return g_count;
}

static void check_same_content(KVStore *expected, KVStore *actual) {
    CHECK(count_entries(expected) == count_entries(actual));

    g_dest = actual;
    g_cmp_total = 0;
    g_cmp_missing = 0;
    g_cmp_wrong = 0;
    kv_foreach(expected, cmp_entry);
    CHECK(g_cmp_missing == 0); /* chaque cle de expected est dans actual */
    CHECK(g_cmp_wrong == 0);   /* avec la bonne valeur */

    g_dest = expected;
    g_cmp_total = 0;
    g_cmp_missing = 0;
    g_cmp_wrong = 0;
    kv_foreach(actual, cmp_entry);
    CHECK(g_cmp_missing == 0); /* et reciproquement */
    CHECK(g_cmp_wrong == 0);
}
static void test_roundtrip(void) {
    remove(PATH_MAIN);

    KVStore *src = kv_create();
    CHECK(src != NULL);

    CHECK(kv_set(src, "alpha", "un") == KV_OK);
    CHECK(kv_set(src, "beta", "deux") == KV_OK);
    CHECK(kv_set(src, "gamma", "trois") == KV_OK);
    CHECK(kv_set(src, "delta", "quatre") == KV_OK);
    CHECK(kv_set(src, "beta", "deux-modifie") == KV_OK);

    CHECK(kv_save(src, PATH_MAIN) == KV_OK);
    CHECK(file_size(PATH_MAIN) > 0);

    /* nouveau store : on part de NULL, kv_load doit le remplacer */
    KVStore *loaded = NULL;
    CHECK(kv_load(&loaded, PATH_MAIN) == KV_OK);
    CHECK(loaded != NULL);

    check_same_content(src, loaded);

    /* et on peut continuer a travailler dessus */
    CHECK(kv_set(loaded, "epsilon", "cinq") == KV_OK);
    CHECK(kv_get(loaded, "epsilon") != NULL);
    CHECK(strcmp(kv_get(loaded, "beta"), "deux-modifie") == 0);

    kv_destroy(src);
    kv_destroy(loaded);
    remove(PATH_MAIN);
}

static void test_save_overwrite(void) {
    remove(PATH_MAIN);

    KVStore *first = kv_create();
    CHECK(first != NULL);
    CHECK(kv_set(first, "obsolete", "1") == KV_OK);
    CHECK(kv_save(first, PATH_MAIN) == KV_OK);

    KVStore *second = kv_create();
    CHECK(second != NULL);
    CHECK(kv_set(second, "nouvelle", "2") == KV_OK);
    CHECK(kv_save(second, PATH_MAIN) == KV_OK);

    KVStore *loaded = NULL;
    CHECK(kv_load(&loaded, PATH_MAIN) == KV_OK);
    CHECK(kv_get(loaded, "obsolete") == NULL);
    CHECK(kv_get(loaded, "nouvelle") != NULL);

    kv_destroy(first);
    kv_destroy(second);
    kv_destroy(loaded);
    remove(PATH_MAIN);
}

static void test_empty_and_whitespace(void) {
    remove(PATH_MAIN);

    KVStore *src = kv_create();
    CHECK(src != NULL);

    CHECK(kv_set(src, "", "") == KV_OK);
    CHECK(kv_set(src, "cle-vide", "") == KV_OK);
    CHECK(kv_set(src, "", "valeur-sur-cle-vide") == KV_OK); /* mise a jour */
    CHECK(kv_set(src, "espaces", "  debut et fin  ") == KV_OK);
    CHECK(kv_set(src, "multi", "ligne1\nligne2\ttab\r\nfin") == KV_OK);
    CHECK(kv_set(src, "unicode", "valeur avec accents: ete") == KV_OK);
    CHECK(kv_set(src, "  ", "cle faite d'espaces") == KV_OK);

    CHECK(kv_save(src, PATH_MAIN) == KV_OK);

    KVStore *loaded = NULL;
    CHECK(kv_load(&loaded, PATH_MAIN) == KV_OK);
    CHECK(loaded != NULL);

    check_same_content(src, loaded);
    CHECK(strcmp(kv_get(loaded, ""), "valeur-sur-cle-vide") == 0);
    CHECK(strcmp(kv_get(loaded, "multi"), "ligne1\nligne2\ttab\r\nfin") == 0);
    CHECK(strcmp(kv_get(loaded, "espaces"), "  debut et fin  ") == 0);

    kv_destroy(src);
    kv_destroy(loaded);
    remove(PATH_MAIN);
}


#define BIG_N 100000

static char *g_seen;
static unsigned g_seen_count;
static unsigned g_seen_dups;

static void mark_entry(const char *key, const char *value) {
    (void)value;
    if (strncmp(key, "cle-", 4) != 0) {
        g_seen_dups++;
        return;
    }
    long idx = strtol(key + 4, NULL, 10);
    if (idx < 0 || idx >= BIG_N) {
        g_seen_dups++;
        return;
    }
    if (g_seen[idx])
        g_seen_dups++;
    g_seen[idx] = 1;
    g_seen_count++;
}

static void test_many_keys(void) {
    remove(PATH_BIG);

    KVStore *src = kv_create();
    CHECK(src != NULL);

    char key[32];
    char value[128];
    for (long i = 0; i < BIG_N; i++) {
        snprintf(key, sizeof key, "cle-%06ld", i);
        snprintf(value, sizeof value,
                 "valeur-%06ld-abcdefghijklmnopqrstuvwxyz-0123456789", i);
        CHECK(kv_set(src, key, value) == KV_OK);
    }
    CHECK(count_entries(src) == BIG_N);

    CHECK(kv_save(src, PATH_BIG) == KV_OK);
    CHECK(file_size(PATH_BIG) > 0);

    KVStore *loaded = NULL;
    CHECK(kv_load(&loaded, PATH_BIG) == KV_OK);
    CHECK(loaded != NULL);

    /* verification par kv_get (exerce le rehash de la table rechargee) */
    for (long i = 0; i < BIG_N; i++) {
        snprintf(key, sizeof key, "cle-%06ld", i);
        snprintf(value, sizeof value,
                 "valeur-%06ld-abcdefghijklmnopqrstuvwxyz-0123456789", i);
        char *got = kv_get(loaded, key);
        if (got == NULL || strcmp(got, value) != 0) {
            g_failures++;
            fprintf(stderr, "ECHEC %s: cle %s absente ou alteree\n", __FILE__,
                    key);
            break;
        }
    }

    g_seen = calloc(BIG_N, 1);
    CHECK(g_seen != NULL);
    if (g_seen != NULL) {
        g_seen_count = 0;
        g_seen_dups = 0;
        kv_foreach(loaded, mark_entry);
        CHECK(g_seen_count == BIG_N);
        CHECK(g_seen_dups == 0);
        for (long i = 0; i < BIG_N; i++) {
            if (!g_seen[i]) {
                g_failures++;
                fprintf(stderr, "ECHEC %s: cle-%.6ld manquante\n", __FILE__, i);
                break;
            }
        }
        free(g_seen);
        g_seen = NULL;
    }

    kv_destroy(src);
    kv_destroy(loaded);
    remove(PATH_BIG);
}

static void expect_load_error(const char *path, KVStatus expected) {
    KVStore *store = kv_create();
    CHECK(store != NULL);
    CHECK(kv_set(store, "temoin", "intact") == KV_OK);

    KVStore *before = store;
    KVStatus status = kv_load(&store, path);
    CHECK(status == expected);
    CHECK(store == before); /* pas de remplacement du store */
    char *witness = kv_get(store, "temoin");
    CHECK(witness != NULL);
    CHECK(witness != NULL && strcmp(witness, "intact") == 0);

    /* le store reste utilisable apres l'echec */
    CHECK(kv_set(store, "apres", "ok") == KV_OK);
    CHECK(kv_get(store, "apres") != NULL);

    kv_destroy(store);
}

static void test_missing_file(void) {
    remove(PATH_BAD);
    expect_load_error(PATH_BAD, KV_IO_ERROR);
}

static void test_directory_as_file(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);
    KVStatus status = kv_load(&store, "build");
    CHECK(status == KV_IO_ERROR);
    kv_destroy(store);
}

static void test_empty_file(void) {
    CHECK(write_raw_file(PATH_BAD, "", 0));

    KVStore *store = kv_create();
    CHECK(store != NULL);
    CHECK(kv_set(store, "temoin", "valeur") == KV_OK);

    CHECK(kv_load(&store, PATH_BAD) == KV_OK);
    CHECK(store != NULL);
    CHECK(count_entries(store) == 0);
    CHECK(kv_get(store, "temoin") == NULL);

    remove(PATH_MAIN);
    CHECK(kv_save(store, PATH_MAIN) == KV_OK);
    CHECK(file_size(PATH_MAIN) == 0);

    kv_destroy(store);
    remove(PATH_BAD);
    remove(PATH_MAIN);
}

static void test_text_file(void) {
    const char *text =
        "ceci n'est pas une base cle-valeur\n"
        "SET foo bar\n"
        "0123456789abcdef\n";
    CHECK(write_raw_file(PATH_BAD, text, strlen(text)));

    expect_load_error(PATH_BAD, KV_IO_ERROR);
    remove(PATH_BAD);
}

static void test_truncated_file(void) {
    /* base valide de 3 paires, puis on coupe les 3 derniers octets */
    remove(PATH_MAIN);
    remove(PATH_BAD);

    KVStore *src = kv_create();
    CHECK(src != NULL);
    CHECK(kv_set(src, "un", "valeur-un") == KV_OK);
    CHECK(kv_set(src, "deux", "valeur-deux") == KV_OK);
    CHECK(kv_set(src, "trois", "valeur-trois") == KV_OK);
    CHECK(kv_save(src, PATH_MAIN) == KV_OK);

    long size = file_size(PATH_MAIN);
    CHECK(size > 3);
    CHECK(copy_prefix(PATH_MAIN, PATH_BAD, (size_t)(size - 3)));
    CHECK(file_size(PATH_BAD) == size - 3);

    expect_load_error(PATH_BAD, KV_IO_ERROR);

    KVStore *src2 = kv_create();
    CHECK(src2 != NULL);
    CHECK(kv_set(src2, "ab", "1") == KV_OK);
    CHECK(kv_set(src2, "cd", "2") == KV_OK);
    CHECK(kv_set(src2, "ef", "3") == KV_OK);
    CHECK(kv_save(src2, PATH_MAIN) == KV_OK);
    char first_key[16];
    char first_value[16];
    CHECK(read_first_pair(PATH_MAIN, first_key, sizeof first_key, first_value,
                          sizeof first_value));
    CHECK(copy_prefix(PATH_MAIN, PATH_BAD, 11));
    KVStore *partial = NULL;
    CHECK(kv_load(&partial, PATH_BAD) == KV_OK);
    CHECK(partial != NULL);
    CHECK(count_entries(partial) == 1);
    char *got = kv_get(partial, first_key);
    CHECK(got != NULL);
    CHECK(got != NULL && strcmp(got, first_value) == 0);

    kv_destroy(src);
    kv_destroy(src2);
    kv_destroy(partial);
    remove(PATH_MAIN);
    remove(PATH_BAD);
}

static void test_truncated_length_header(void) {
    remove(PATH_MAIN);
    remove(PATH_BAD);

    CHECK(write_raw_file(PATH_BAD, "x", 1));
    expect_load_error(PATH_BAD, KV_IO_ERROR);

    CHECK(write_raw_file(PATH_BAD, "xy", 2));
    expect_load_error(PATH_BAD, KV_IO_ERROR);

    KVStore *src = kv_create();
    CHECK(src != NULL);
    CHECK(kv_set(src, "ab", "1") == KV_OK);
    CHECK(kv_save(src, PATH_MAIN) == KV_OK);
    long size = file_size(PATH_MAIN);
    CHECK(size == 11); /* 4 + 2 + 4 + 1 */
    CHECK(copy_prefix(PATH_MAIN, PATH_BAD, (size_t)(size - 3)));
    expect_load_error(PATH_BAD, KV_IO_ERROR);

    kv_destroy(src);
    remove(PATH_MAIN);
    remove(PATH_BAD);
}

static void test_corrupted_lengths(void) {
    remove(PATH_MAIN);
    remove(PATH_BAD);

    KVStore *src = kv_create();
    CHECK(src != NULL);
    CHECK(kv_set(src, "aaa", "111") == KV_OK);
    CHECK(kv_set(src, "bbb", "222") == KV_OK);
    CHECK(kv_set(src, "ccc", "333") == KV_OK);
    CHECK(kv_save(src, PATH_MAIN) == KV_OK);

    const long second = 14;
    const unsigned char ff[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    CHECK(patch_file(PATH_MAIN, second, ff, sizeof ff));
    expect_load_error(PATH_MAIN, KV_IO_ERROR);

    CHECK(kv_save(src, PATH_MAIN) == KV_OK);
    CHECK(patch_file(PATH_MAIN, second + 4 + 3, ff, sizeof ff));
    expect_load_error(PATH_MAIN, KV_IO_ERROR);

    CHECK(kv_save(src, PATH_MAIN) == KV_OK);
    {
        const unsigned char corrupted = 'Z';
        CHECK(patch_file(PATH_MAIN, second + 4, &corrupted, 1));
    }
    KVStore *loaded = NULL;
    CHECK(kv_load(&loaded, PATH_MAIN) == KV_OK);
    CHECK(loaded != NULL);
    CHECK(count_entries(loaded) == 3);
    CHECK(kv_get(loaded, "bbb") == NULL); /* cle alteree */
    kv_destroy(loaded);

    kv_destroy(src);
    remove(PATH_MAIN);
}

static void test_load_replaces_non_empty_store(void) {
    remove(PATH_MAIN);

    KVStore *source = kv_create();
    CHECK(source != NULL);
    CHECK(kv_set(source, "fichier-1", "f1") == KV_OK);
    CHECK(kv_set(source, "fichier-2", "f2") == KV_OK);
    CHECK(kv_save(source, PATH_MAIN) == KV_OK);

    KVStore *target = kv_create();
    CHECK(target != NULL);
    CHECK(kv_set(target, "en-memoire", "m1") == KV_OK);
    CHECK(kv_set(target, "fichier-1", "ancienne-valeur") == KV_OK);

    CHECK(kv_load(&target, PATH_MAIN) == KV_OK);
    CHECK(target != NULL);

    CHECK(kv_get(target, "en-memoire") == NULL);
    CHECK(kv_get(target, "fichier-1") != NULL);
    CHECK(strcmp(kv_get(target, "fichier-1"), "f1") == 0);
    CHECK(count_entries(target) == 2);

    check_same_content(source, target);

    kv_destroy(source);
    kv_destroy(target);
    remove(PATH_MAIN);
}


static void test_invalid_arguments(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    CHECK(kv_save(NULL, PATH_MAIN) == KV_INVALID_ARGUMENT);
    CHECK(kv_save(store, NULL) == KV_INVALID_ARGUMENT);
    CHECK(kv_load(NULL, PATH_MAIN) == KV_INVALID_ARGUMENT);
    CHECK(kv_load(&store, NULL) == KV_INVALID_ARGUMENT);
    CHECK(kv_get(store, "absente") == NULL);

    kv_destroy(store);
}

int main(void) {
    test_roundtrip();
    test_save_overwrite();
    test_empty_and_whitespace();
    test_many_keys();
    test_missing_file();
    test_directory_as_file();
    test_empty_file();
    test_text_file();
    test_truncated_file();
    test_truncated_length_header();
    test_corrupted_lengths();
    test_load_replaces_non_empty_store();
    test_invalid_arguments();

    printf("%u verifications, %u echecs\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
