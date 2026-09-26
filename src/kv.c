#include <stdlib.h>
#include <string.h>

typedef struct {
    char *key;
    char *value;
} KVEntry;

typedef struct {
    KVEntry *entries;
    size_t size;
    size_t capacity;
} KVStore;