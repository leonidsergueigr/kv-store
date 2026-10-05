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

int main(void) {
    test_create_destroy();
    test_set_get();
    test_get_missing();
    test_update();
    test_growth();
    test_delete();
    test_delete_errors();
    test_set_invalid();

    printf("%u verifications, %u echecs\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}