#ifndef KV_H
#define KV_H

#define INITIAL_CAPACITY 4

#include <stdlib.h>

typedef struct KVEntry KVEntry;
typedef struct KVStore KVStore;

typedef enum {
    KV_OK,
    KV_SUCCESS,
    KV_KEY_NOT_FOUND,
    KV_EXISTS,
    KV_MEMORY_ERROR,
    KV_INVALID_ARGUMENT
} KVStatus;

KVStore *kv_create(void);
void kv_destroy(KVStore *kvstore);
KVStatus kv_set(KVStore *store, const char *key, const char *value);
char *kv_get(KVStore *store, const char *key);
KVStatus kv_delete(KVStore *store, const char *key);


#endif
