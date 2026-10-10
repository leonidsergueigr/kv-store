#include "parser.h"
#include <stddef.h>
#include <string.h>

CommandType get_command_type(const char *cmd){
    if (cmd == NULL) return CMD_INVALID;

    if (strcmp(cmd, "SET") == 0) return CMD_SET;
    if (strcmp(cmd, "GET") == 0) return CMD_GET;
    if (strcmp(cmd, "DELETE") == 0) return CMD_DELETE;
    if (strcmp(cmd, "LIST") == 0) return CMD_LIST;
    if (strcmp(cmd, "EXIT") == 0) return CMD_EXIT;

    return CMD_INVALID;
}

char *skip_spaces(char *cursor){
    if (cursor == NULL) return NULL;
    while (*cursor == ' ' || *cursor == '\t')
    {
        cursor++;
    }
    return cursor;
}

char *get_remaining_text(char *cursor){
    if (cursor == NULL) return NULL;
    cursor = skip_spaces(cursor);
    if (*cursor == '\0'){
        return NULL;
    }
    return cursor;
}

char *get_current_word(char **cursor){
    if (cursor == NULL || *cursor == NULL || **cursor == '\0'){
        return NULL;
    }

    *cursor = skip_spaces(*cursor);

    if (**cursor == '\0'){
        return NULL;
    }

    char *start = *cursor;

    while (**cursor != ' ' && **cursor != '\t' && **cursor != '\0'){
        (*cursor)++;
    }
    if (**cursor != '\0'){
        **cursor = '\0';
        (*cursor)++;
    }

    return start;
}