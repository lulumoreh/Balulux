#ifndef LEXERF_H
#define LEXERF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Enums
typedef enum {
    START,
    NUMBER,
    IDENTIFIER,
    SEPARATOR,
    OPERATOR,
    EQUAL,
    KEYWORD,
    ACCEPT,
    ERROR,
    INT_I, //keywords states
    INT_N,
    INT_T,
    LULOG_L,
    LULOG_U,
    LULOG_L2,   
    LULOG_O,    
    LULOG_G
} State;

typedef enum {
    NUMBER_TOKEN,
    KEYWORD_TOKEN,
    IDENTIFIER_TOKEN,
    SEPARATOR_TOKEN,
    OPERATOR_TOKEN,
    EQUAL_TOKEN,
    END_OF_TOKENS
} TokenType;

// Structs
typedef struct {
    TokenType type;
    char *value;
    int line_num;
} Token;

// Global Variables
#define UNTIL_BREAK 1
#define STATES_NUM 17
#define ASCI_CHARS 256
extern State transition_matrix[STATES_NUM][ASCI_CHARS];
extern int line_number;

// Function Prototypes
void initialize_transition_matrix();
Token *lexer(FILE *file);
void print_token(Token token);
void free_tokens(Token *tokens);
void add_keyword_transitions(char* keyword);

#endif