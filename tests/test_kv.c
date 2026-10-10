#include "kv.h"

#include <stdio.h>
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


static void test_create_destroy(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);
    kv_destroy(store);

    kv_destroy(NULL);
}


static void test_set_get(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    CHECK(kv_set(store, "cle", "valeur") == KV_OK);

    char *value = kv_get(store, "cle");
    CHECK(value != NULL);
    CHECK(value != NULL && strcmp(value, "valeur") == 0);

    kv_destroy(store);
}


static void test_get_missing(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    CHECK(kv_get(store, "absente") == NULL);
    CHECK(kv_get(NULL, "cle") == NULL);
    CHECK(kv_get(store, NULL) == NULL);

    kv_destroy(store);
}


static void test_update(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    CHECK(kv_set(store, "a", "1") == KV_OK);
    CHECK(kv_set(store, "a", "2") == KV_OK);

    char *value = kv_get(store, "a");
    CHECK(value != NULL && strcmp(value, "2") == 0);

    kv_destroy(store);
}


static void test_growth(void) {
    enum { N = 64 };
    KVStore *store = kv_create();
    CHECK(store != NULL);

    char key[32];
    char value[32];

    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        snprintf(value, sizeof value, "val-%d", i);
        CHECK(kv_set(store, key, value) == KV_OK);
    }

    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        snprintf(value, sizeof value, "val-%d", i);
        char *got = kv_get(store, key);
        CHECK(got != NULL && strcmp(got, value) == 0);
    }

    kv_destroy(store);
}

static void test_delete(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    const char *keys[] = {"a", "b", "c", "d", "e"};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        CHECK(kv_set(store, keys[i], keys[i]) == KV_OK);

    CHECK(kv_delete(store, "c") == KV_OK);
    CHECK(kv_get(store, "c") == NULL);

    CHECK(kv_get(store, "a") != NULL);
    CHECK(kv_get(store, "b") != NULL);
    CHECK(kv_get(store, "d") != NULL);
    CHECK(kv_get(store, "e") != NULL);

    CHECK(kv_set(store, "f", "f") == KV_OK);
    CHECK(kv_get(store, "f") != NULL);

    kv_destroy(store);
}

static void test_delete_errors(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    CHECK(kv_delete(store, "absente") == KV_KEY_NOT_FOUND);
    CHECK(kv_delete(NULL, "a") == KV_INVALID_ARGUMENT);
    CHECK(kv_delete(store, NULL) == KV_INVALID_ARGUMENT);

    kv_destroy(store);
}

static void test_set_invalid(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    CHECK(kv_set(NULL, "a", "1") == KV_INVALID_ARGUMENT);
    CHECK(kv_set(store, NULL, "1") == KV_INVALID_ARGUMENT);
    CHECK(kv_set(store, "a", NULL) == KV_INVALID_ARGUMENT);

    kv_destroy(store);
}

typedef struct {
    unsigned calls;
    char keys[8][32];
    char values[8][32];
} ForeachCapture;

static ForeachCapture g_capture;

static void capture_entry(const char *key, const char *value) {
    if (g_capture.calls < 8) {
        snprintf(g_capture.keys[g_capture.calls], sizeof g_capture.keys[0], "%s", key);
        snprintf(g_capture.values[g_capture.calls], sizeof g_capture.values[0], "%s", value);
    }
    g_capture.calls++;
}

static void test_foreach(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    memset(&g_capture, 0, sizeof g_capture);

    /* store vide : aucune iteration */
    kv_foreach(store, capture_entry);
    CHECK(g_capture.calls == 0);

    CHECK(kv_set(store, "a", "1") == KV_OK);
    CHECK(kv_set(store, "b", "2") == KV_OK);
    CHECK(kv_set(store, "c", "3") == KV_OK);

    /* l'ordre suit l'ordre d'insertion */
    kv_foreach(store, capture_entry);
    CHECK(g_capture.calls == 3);
    CHECK(strcmp(g_capture.keys[0], "a") == 0);
    CHECK(strcmp(g_capture.values[0], "1") == 0);
    CHECK(strcmp(g_capture.keys[1], "b") == 0);
    CHECK(strcmp(g_capture.values[1], "2") == 0);
    CHECK(strcmp(g_capture.keys[2], "c") == 0);
    CHECK(strcmp(g_capture.values[2], "3") == 0);

    /* une mise a jour ne change pas le nombre d'entrees */
    CHECK(kv_set(store, "a", "10") == KV_OK);
    kv_foreach(store, capture_entry);
    CHECK(g_capture.calls == 6);
    CHECK(strcmp(g_capture.keys[3], "a") == 0);
    CHECK(strcmp(g_capture.values[3], "10") == 0);

    kv_destroy(store);
}

static void test_foreach_null(void) {
    KVStore *store = kv_create();
    CHECK(store != NULL);

    /* robustesse : ne doit pas planter sur des arguments nuls */
    kv_foreach(NULL, capture_entry);
    kv_foreach(store, NULL);
    kv_foreach(NULL, NULL);

    kv_destroy(store);
}

int main(void) {
    test_create_destroy();
    test_set_get();
    test_get_missing();
    test_update();
    test_growth();
    test_delete();
    test_delete_errors();
    test_set_invalid();
    test_foreach();
    test_foreach_null();

    printf("%u verifications, %u echecs\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}