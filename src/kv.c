#include "kv.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef struct KVEntry {
    char *key;
    char *value;
} KVEntry;

typedef struct KVStore {
    KVEntry *entries;
    size_t size;
    size_t capacity;
} KVStore;


KVStore *kv_create(void){
    KVStore *kvstore = malloc(sizeof *kvstore);
    if (kvstore == NULL)
        return NULL;
    kvstore->entries = calloc(INITIAL_CAPACITY, sizeof *kvstore->entries);
    if (kvstore->entries == NULL) {
        free(kvstore);
        return NULL;
    }
    kvstore->size = 0;
    kvstore->capacity = INITIAL_CAPACITY;
    return kvstore;
}


void kv_destroy(KVStore *kvstore) {
    if (kvstore == NULL)
        return;
    for (size_t i = 0; i < kvstore->size; i++) {
        free(kvstore->entries[i].key);
        free(kvstore->entries[i].value);
    }
    free(kvstore->entries);
    free(kvstore);
}
