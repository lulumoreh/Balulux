#ifndef LEXERF_H
#define LEXERF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Enums
typedef enum {
    ACCEPT,
    ERROR,
    STRING_ERROR,
    START,
    NUMBER,
    DECIMAL_POINT,     
    DECIMAL_NUMBER,    
    IDENTIFIER,
    SEPARATOR,
    OPERATOR,
    EQUAL,
    // Existing type keywords
    INT_I, 
    INT_N,
    INT_T,
    STR_S,
    STR_T,
    STR_R,
    DOUBLE_D,
    DOUBLE_O,
    DOUBLE_U,
    DOUBLE_B,
    DOUBLE_L,
    DOUBLE_E,
    // New void keyword states
    VOID_V,
    VOID_O,
    VOID_I,
    VOID_D,
    // Return keyword states
    RETURN_R,
    RETURN_E,
    RETURN_T,
    RETURN_U,
    RETURN_R2,
    RETURN_N,
    STRING_LITERAL,
    STRING_END,
    // Existing other states (LULOG, IF, ELSE, etc.)
    LULOG_L,
    LULOG_U,
    LULOG_L2,   
    LULOG_O,    
    LULOG_G,
    LULOOP_L,
    LULOOP_U,
    LULOOP_L2,
    LULOOP_O,
    LULOOP_O2,
    LULOOP_P,
    IF_I,
    IF_F,
    ELSE_E,
    ELSE_L,
    ELSE_S,
    ELSE_E2,
    AND_A,
    AND_N,
    AND_D,
    OR_O,
    OR_R,
    NOT_N,
    NOT_O,
    NOT_T,
    ARRAY_A,
    ARRAY_R,
    ARRAY_R2,
    ARRAY_A2,
    ARRAY_Y,
    // New comment states
    COMMENT_SLASH,
    SINGLE_LINE_COMMENT,
    // New state for luload function
    LULOAD_L,
    LULOAD_U,
    LULOAD_L2,
    LULOAD_O,
    LULOAD_A,
    LULOAD_D,
    LULOAD_KEYWORD,
} State;

typedef enum {
    NUMBER_TOKEN,
    KEYWORD_TOKEN,       
    TYPE_TOKEN,          
    STRING_LITERAL_TOKEN,
    STRING_ERROR_TOKEN,
    IDENTIFIER_TOKEN,
    SEPARATOR_TOKEN,     
    OPERATOR_TOKEN,      
    EQUAL_TOKEN,         
    LOGICAL_OP_TOKEN,    
    ARRAY_TOKEN,         
    COMMENT_TOKEN,
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
#define STATES_NUM 74  // Increased to accommodate new states
#define ASCI_CHARS 256
extern State transition_matrix[STATES_NUM][ASCI_CHARS];
extern int line_number;

// Function Prototypes
void initialize_transition_matrix();
Token *lexer(FILE *file, int *flag);
void print_token(Token token);
void free_tokens(Token *tokens);
TokenType getType(State state);


typedef void (*pFunLexer)(char*, int*, Token*, int*, int*, State*, State*, char*, int*, int*);


#endif