#ifndef KV_H
#define KV_H

#define INITIAL_CAPACITY 4

#include <stdlib.h>

typedef struct KVEntry KVEntry;
typedef struct KVStore KVStore;

KVStore *kv_create(void);

#endif