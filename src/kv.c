#include "kv.h"
#include "percist.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct KVEntry {
  char *key;
  char *value;
  struct KVEntry *next;
} KVEntry;

typedef struct KVStore {
  KVEntry **buckets;
  size_t size;
  size_t capacity;
} KVStore;

bool hash_string(const char *s, uint64_t *result) {
  if (s == NULL) {
    return false;
  }
  uint64_t hash = 14695981039346656037ULL;
  while (*s) {
    hash ^= (unsigned char)*s;
    hash *= 1099511628211ULL;
    s++;
  }
  *result = hash;
  return true;
}

static KVStatus kv_ensure_capacity(KVStore *store) {
  if (store->size < store->capacity)
    return KV_OK;
  size_t new_cap =
      (store->capacity == 0) ? INITIAL_CAPACITY : store->capacity * 2;
  KVEntry **new_buckets = calloc(new_cap, sizeof *new_buckets);
  if (new_buckets == NULL)
    return KV_MEMORY_ERROR;
  for (size_t i = 0; i < store->capacity; i++) {
    KVEntry *entry = store->buckets[i];
    while (entry != NULL) {
      uint64_t hash;
      if (!hash_string(entry->key, &hash)) {
        return KV_HASH_ERROR;
      }
      KVEntry *suivant = entry->next;
      size_t new_index = hash % new_cap;
      entry->next = new_buckets[new_index];
      new_buckets[new_index] = entry;
      entry = suivant;
    }
  }
  free(store->buckets);
  store->buckets = new_buckets;
  store->capacity = new_cap;
  return KV_OK;
}

char *kv_strdup(const char *s) {
  if (s == NULL)
    return NULL;
  size_t n = strlen(s) + 1;
  char *copy = malloc(n);
  if (copy == NULL)
    return NULL;
  memcpy(copy, s, n);
  return copy;
}

KVStore *kv_create(void) {
  KVStore *kvstore = malloc(sizeof *kvstore);
  if (kvstore == NULL)
    return NULL;
  kvstore->buckets = calloc(INITIAL_CAPACITY, sizeof *kvstore->buckets);
  if (kvstore->buckets == NULL) {
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
  for (size_t i = 0; i < kvstore->capacity; i++) {
    KVEntry *current = kvstore->buckets[i];
    while (current != NULL) {
      KVEntry *suivant = current->next;
      free(current->key);
      free(current->value);
      free(current);
      current = suivant;
    }
  }
  free(kvstore->buckets);
  free(kvstore);
}

static KVEntry *kv_find(KVStore *store, const char *key, size_t index) {
  KVEntry *current = store->buckets[index];
  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

KVStatus kv_set(KVStore *store, const char *key, const char *value) {
  if (store == NULL || key == NULL || value == NULL)
    return KV_INVALID_ARGUMENT;
  uint64_t hash;
  if (!hash_string(key, &hash)) {
    return KV_HASH_ERROR;
  }
  KVStatus status = kv_ensure_capacity(store);
  if (status != KV_OK) {
    return status;
  }
  size_t index = hash % store->capacity;
  KVEntry *entry = kv_find(store, key, index);

  if (entry == NULL) {
    KVEntry *new_entry = malloc(sizeof *new_entry);
    if (new_entry == NULL)
      return KV_MEMORY_ERROR;
    char *key_copy = kv_strdup(key);
    if (key_copy == NULL) {
      free(new_entry);
      return KV_MEMORY_ERROR;
    }
    new_entry->key = key_copy;
    char *value_copy = kv_strdup(value);
    if (value_copy == NULL) {
      free(key_copy);
      free(new_entry);
      return KV_MEMORY_ERROR;
    }
    new_entry->value = value_copy;
    new_entry->next = store->buckets[index];
    store->buckets[index] = new_entry;
    store->size++;
    return KV_OK;
  } else {
    char *old_value = entry->value;
    char *new_value = kv_strdup(value);
    if (new_value == NULL)
      return KV_MEMORY_ERROR;
    free(old_value);
    entry->value = new_value;
    return KV_OK;
  }
}

char *kv_get(KVStore *store, const char *key) {
  if (store == NULL || key == NULL)
    return NULL;
  uint64_t hash;
  if (!hash_string(key, &hash))
    return NULL;
  size_t index = hash % store->capacity;
  KVEntry *entry = kv_find(store, key, index);
  if (entry == NULL)
    return NULL;
  return entry->value;
}

KVStatus kv_delete(KVStore *store, const char *key) {
  if (store == NULL || key == NULL)
    return KV_INVALID_ARGUMENT;
  uint64_t hash;
  if (!hash_string(key, &hash))
    return KV_HASH_ERROR;
  size_t index = hash % store->capacity;
  KVEntry **currentp = &store->buckets[index];
  while (*currentp != NULL) {
    if (strcmp((*currentp)->key, key) == 0) {
      KVEntry *found = *currentp;
      *currentp = found->next;
      free(found->key);
      free(found->value);
      free(found);
      store->size--;
      return KV_OK;
    }
    currentp = &(*currentp)->next;
  }

  return KV_KEY_NOT_FOUND;
}

KVStatus kv_save(const KVStore *store, const char *path) {
  if (store == NULL || path == NULL)
    return KV_INVALID_ARGUMENT;
  PersistWriter *w = persist_writer_open(path);
  if (w == NULL)
    return KV_IO_ERROR;
  for (size_t i = 0; i < store->capacity; i++) {
    KVEntry *current = store->buckets[i];
    while (current != NULL) {
      PersistStatus status =
          persist_write_pair(w, current->key, current->value);
      if (status != PERSIST_OK) {
        persist_writer_abort(w);
        return KV_IO_ERROR;
      }
      current = current->next;
    }
  }
  if (persist_writer_commit(w) != PERSIST_OK) {
    return KV_IO_ERROR;
  }
  return KV_OK;
}

KVStatus kv_load(KVStore **store, const char *path) {
  if (store == NULL || path == NULL)
    return KV_INVALID_ARGUMENT;
  PersistReader *r = persist_reader_open(path);

  if (r == NULL)
    return KV_IO_ERROR;
  KVStore *tmp = kv_create();
  char *key = NULL;
  char *value = NULL;
  while (true) {
    PersistStatus reader = persist_read_pair(r, &key, &value);
    if (reader == PERSIST_END) {
      persist_reader_close(r);
      kv_destroy(*store);
      *store = tmp;
      return KV_OK;
    }
    if (reader == PERSIST_IO_ERROR || reader == PERSIST_FORMAT_ERROR) {
      kv_destroy(tmp);
      persist_reader_close(r);
      return KV_IO_ERROR;
    }
    KVStatus status = kv_set(tmp, key, value);
    if (status != KV_OK) {
      persist_reader_close(r);
      kv_destroy(tmp);
      return status;
    }
    free(key);
    free(value);
    key = NULL;
    value = NULL;
  }
}

void kv_foreach(const KVStore *store, KVCallback callback) {
  if (store == NULL || callback == NULL)
    return;
  for (size_t i = 0; i < store->capacity; i++) {
    KVEntry *current = store->buckets[i];
    while (current != NULL) {
      callback(current->key, current->value);
      current = current->next;
    }
  }
}
