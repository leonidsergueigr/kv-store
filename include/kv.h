#ifndef KV_H
#define KV_H

#define INITIAL_CAPACITY 4

typedef struct KVEntry KVEntry;
typedef struct KVStore KVStore;

typedef enum {
  KV_OK,
  KV_SUCCESS,
  KV_KEY_NOT_FOUND,
  KV_EXISTS,
  KV_MEMORY_ERROR,
  KV_INVALID_ARGUMENT,
  KV_HASH_ERROR,
  KV_IO_ERROR
} KVStatus;

KVStore *kv_create(void);
void kv_destroy(KVStore *kvstore);
KVStatus kv_set(KVStore *store, const char *key, const char *value);
char *kv_get(KVStore *store, const char *key);
KVStatus kv_delete(KVStore *store, const char *key);
typedef void (*KVCallback)(const char *key, const char *value);
void kv_foreach(const KVStore *store, KVCallback callback);
char *kv_strdup(const char *s);
KVStatus kv_save(const KVStore *store, const char *path);
KVStatus kv_load(KVStore **store, const char *path);

#endif
