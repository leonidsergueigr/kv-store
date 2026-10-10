#ifndef PARSER_H
#define PARSER_H

typedef enum CommandType {
    CMD_SET,
    CMD_GET,
    CMD_DELETE,
    CMD_LIST,
    CMD_EXIT,
    CMD_INVALID
} CommandType;

char* get_current_word(char **cursor);
char *skip_spaces(char *cursor);
CommandType get_command_type(const char *cmd);
char *get_remaining_text(char *cursor);
#endif
