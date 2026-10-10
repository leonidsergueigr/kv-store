#include "parser.h"

#include <stdio.h>
#include <string.h>

static unsigned g_checks = 0;
static unsigned g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            fprintf(stderr, "ECHEC %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)


static void test_command_type(void) {
    CHECK(get_command_type("SET") == CMD_SET);
    CHECK(get_command_type("GET") == CMD_GET);
    CHECK(get_command_type("DELETE") == CMD_DELETE);
    CHECK(get_command_type("LIST") == CMD_LIST);
    CHECK(get_command_type("EXIT") == CMD_EXIT);

    CHECK(get_command_type(NULL) == CMD_INVALID);
    CHECK(get_command_type("") == CMD_INVALID);
    CHECK(get_command_type("set") == CMD_INVALID);
    CHECK(get_command_type("SETX") == CMD_INVALID);
    CHECK(get_command_type(" SET") == CMD_INVALID);
}


static void test_skip_spaces(void) {
    CHECK(skip_spaces(NULL) == NULL);

    char aucun[] = "abc";
    CHECK(strcmp(skip_spaces(aucun), "abc") == 0);

    char un[] = " abc";
    CHECK(strcmp(skip_spaces(un), "abc") == 0);

    char plusieurs[] = "    abc";
    CHECK(strcmp(skip_spaces(plusieurs), "abc") == 0);

    char tabulations[] = " \t\tabc";
    CHECK(strcmp(skip_spaces(tabulations), "abc") == 0);

    char que_espaces[] = "   ";
    CHECK(*skip_spaces(que_espaces) == '\0');

    char vide[] = "";
    CHECK(*skip_spaces(vide) == '\0');
}


static void test_remaining_text(void) {
    CHECK(get_remaining_text(NULL) == NULL);

    char simple[] = "value";
    CHECK(strcmp(get_remaining_text(simple), "value") == 0);

    char espace[] = "    value";
    CHECK(strcmp(get_remaining_text(espace), "value") == 0);

    char interne[] = " value avec espaces ";
    CHECK(strcmp(get_remaining_text(interne), "value avec espaces ") == 0);

    char espaces_seuls[] = "    ";
    CHECK(get_remaining_text(espaces_seuls) == NULL);

    char vide[] = "";
    CHECK(get_remaining_text(vide) == NULL);
}


static void test_current_word(void) {
    CHECK(get_current_word(NULL) == NULL);

    char *curseur_nul = NULL;
    CHECK(get_current_word(&curseur_nul) == NULL);

    char vide[] = "";
    char *cur_vide = vide;
    CHECK(get_current_word(&cur_vide) == NULL);

    char espaces_seuls[] = "   ";
    char *cur_esp = espaces_seuls;
    CHECK(get_current_word(&cur_esp) == NULL);

    char mots[] = "SET   key   value";
    char *cur = mots;
    CHECK(strcmp(get_current_word(&cur), "SET") == 0);
    CHECK(strcmp(get_current_word(&cur), "key") == 0);
    CHECK(strcmp(get_current_word(&cur), "value") == 0);
    CHECK(get_current_word(&cur) == NULL);

    char tabulations[] = "SET\tkey\tvalue";
    char *cur_tab = tabulations;
    CHECK(strcmp(get_current_word(&cur_tab), "SET") == 0);
    CHECK(strcmp(get_current_word(&cur_tab), "key") == 0);
    CHECK(strcmp(get_current_word(&cur_tab), "value") == 0);
    CHECK(get_current_word(&cur_tab) == NULL);

    char meneurs[] = "   SET key";
    char *cur_men = meneurs;
    CHECK(strcmp(get_current_word(&cur_men), "SET") == 0);
    CHECK(strcmp(get_current_word(&cur_men), "key") == 0);
    CHECK(get_current_word(&cur_men) == NULL);

    char fin[] = "SET ";
    char *cur_fin = fin;
    CHECK(strcmp(get_current_word(&cur_fin), "SET") == 0);
    CHECK(get_current_word(&cur_fin) == NULL);
}


static void test_full_command(void) {
    char ligne[] = "SET   ma-cle   ma valeur avec espaces";

    char *cur = ligne;
    char *cmd = get_current_word(&cur);
    CHECK(cmd != NULL && strcmp(cmd, "SET") == 0);
    CHECK(get_command_type(cmd) == CMD_SET);

    char *cle = get_current_word(&cur);
    CHECK(cle != NULL && strcmp(cle, "ma-cle") == 0);

    char *valeur = get_remaining_text(cur);
    CHECK(valeur != NULL && strcmp(valeur, "ma valeur avec espaces") == 0);
}


int main(void) {
    test_command_type();
    test_skip_spaces();
    test_remaining_text();
    test_current_word();
    test_full_command();

    printf("%u verifications, %u echecs\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}