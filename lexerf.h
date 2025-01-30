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
    ERROR
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
extern State transition_matrix[9][256];
extern int line_number;

// Function Prototypes
void initialize_transition_matrix();
Token *lexer(FILE *file);
void print_token(Token token);
void free_tokens(Token *tokens);
void add_keyword_transitions(char* keyword);

#endif