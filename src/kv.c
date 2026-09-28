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


static KVStatus kv_ensure_capacity(KVStore *store){
    if (store->size < store->capacity) return KV_OK;
    size_t new_cap = (store->capacity == 0)? INITIAL_CAPACITY : store->capacity *2;
    KVEntry *tmp = realloc(store->entries, new_cap * sizeof *tmp);
    if (tmp == NULL){
        return KV_MEMORY_ERROR;
    }
    store->entries = tmp;
    store->capacity = new_cap;
    return KV_OK;
}


static char *kv_strdup(const char *s){
    if (s == NULL) return NULL;
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy == NULL) return NULL;
    memcpy(copy, s, n);
    return copy;
}

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

KVEntry *kv_find(KVStore *store, const char *key){
    for (size_t i = 0; i < store->size; i++){
        if (strcmp(store->entries[i].key, key) == 0){
            return &store->entries[i];
        }
    }
    return NULL;
}

KVStatus kv_set(KVStore *store, const char *key, const char *value){
    if (store == NULL || key == NULL || value == NULL) return KV_INVALID_ARGUMENT;
    KVEntry *entry = kv_find(store, key);
    if (entry == NULL){
        KVStatus status = kv_ensure_capacity(store);
        if (status != KV_OK) return status;
        char *key_copy = kv_strdup(key);
        if (key_copy == NULL) return KV_MEMORY_ERROR;
        store->entries[store->size].key = key_copy;
        char *value_copy = kv_strdup(value);
        if (value_copy == NULL) {
            free(store->entries[store->size].key);
            return KV_MEMORY_ERROR;
        }    
        store->entries[store->size].value = value_copy;
        store->size++;
        return KV_OK;
    } else {
        char *old_value = entry->value;
        char *new_value = kv_strdup(value);
        if (new_value == NULL) return KV_MEMORY_ERROR;
        free(old_value);
        entry->value = new_value;
        return KV_OK;
    }
}
