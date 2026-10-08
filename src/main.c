#include "kv.h"
#include "parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_CMD_LEN 256

int main(void) {
    KVStore *kvstore = kv_create();
    char command[MAX_CMD_LEN];

    while (true) {
        printf("Enter command (SET/GET/DELETE/LIST/EXIT)\n> ");
        if (fgets(command, MAX_CMD_LEN, stdin) == NULL) {
            printf("Please enter a valid command!\n");
            continue;
        }

        command[strcspn(command, "\n")] = '\0';
        char *cursor = command;
        char *cmd = get_current_word(&cursor);
        char *key = get_current_word(&cursor);
        char *value = cursor;
        printf("Command: %s\n", cmd);
        printf("Key: %s\n", key);
        printf("Value: %s\n", value);

    }

    kv_destroy(kvstore);
    return 0;
}