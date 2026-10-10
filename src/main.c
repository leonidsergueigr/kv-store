#include "kv.h"
#include "parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_CMD_LEN 256

static void print_entry(const char *key, const char *value){
    printf("%s = %s\n", key, value);
}

int main(void) {
    KVStore *kvstore = kv_create();
    if (kvstore == NULL) {
        fprintf(stderr, "Failed to create KVStore\n");
        return 1;
    }

    char command[MAX_CMD_LEN];

    printf("Enter command (SET/GET/DELETE/LIST/EXIT)\n");
    while (true) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        char *newline = strchr(command, '\n');
        if (newline != NULL) {
            *newline = '\0';
        } else {
            int ch = getchar();
            if (ch != '\n' && ch != EOF) {
                while (ch != '\n' && ch != EOF) {
                    ch = getchar();
                }
                fprintf(stderr, "Error: command too long\n");
                continue;
            }
        }

        char *cursor = command;
        char *cmd = get_current_word(&cursor);
        if (cmd == NULL) {
            continue;
        }

        CommandType type = get_command_type(cmd);
        switch (type) {
            case CMD_SET: {
                char *key = get_current_word(&cursor);
                char *value = get_remaining_text(cursor);
                if (key == NULL || value == NULL) {
                    printf("Usage: SET <key> <value>\n");
                    break;
                }

                KVStatus status = kv_set(kvstore, key, value);
                if (status == KV_OK) {
                    printf("Key-value pair stored successfully.\n");
                } else if (status == KV_MEMORY_ERROR) {
                    fprintf(stderr, "Memory allocation failed\n");
                } else {
                    fprintf(stderr, "Failed to store key-value pair\n");
                }
                break;
            }
            case CMD_GET: {
                char *key = get_current_word(&cursor);
                if (key == NULL || get_remaining_text(cursor) != NULL) {
                    printf("Usage: GET <key>\n");
                    break;
                }
                char *value = kv_get(kvstore, key);
                if (value != NULL) {
                    printf("Value: %s\n", value);
                } else {
                    printf("Key not found\n");
                }
                break;
            }
            case CMD_DELETE:{
                char *key = get_current_word(&cursor);
                if (key == NULL || get_remaining_text(cursor) != NULL){
                    printf("Usage: DELETE <key>\n");
                    break;
                }
                KVStatus status = kv_delete(kvstore, key);
                if (status == KV_OK){
                    printf("Key deleted successfully.\n");
                } else if (status == KV_KEY_NOT_FOUND){
                    printf("Key not found\n");
                } else {
                    fprintf(stderr, "Failed to delete key\n");
                }
                break;
            }
            case CMD_LIST:
                if (get_remaining_text(cursor) != NULL){
                    printf("Usage: LIST\n");
                    break;
                }
                kv_foreach(kvstore, print_entry);
                break;
            case CMD_EXIT:
                printf("Exiting...\n");
                kv_destroy(kvstore);
                return 0;
            default:
                printf("command invalid\n");
                break;
        }
    }

    kv_destroy(kvstore);
    return 0;
}