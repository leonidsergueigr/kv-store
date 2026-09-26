#include "kv.h"
#include <stdio.h>
int main(void) {
    KVStore *kvstore = kv_create();
    kv_destroy(kvstore);
    return 0;
}