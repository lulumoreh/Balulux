#ifndef PARSER_H
#define PARSER_H

#include "lexerf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Non-terminal symbols (expand with all your language's non-terminals)
typedef enum {
    NT_PROGRAM,
    NT_FUNCTION,
    NT_PARAMETERS,
    NT_PARAMETER,
    NT_STATEMENTS,
    NT_STATEMENT,
    NT_VARIABLE_DECLARATION,
    NT_ASSIGNMENT,
    NT_IF_STATEMENT,
    NT_LOOP_STATEMENT,
    NT_FUNCTION_CALL,
    NT_RETURN_STATEMENT,
    NT_EXPRESSION,
    NT_TERM,
    NT_OPERATOR,
    NT_TYPE,
    NT_ARGUMENTS,
    NT_ARGUMENT_LIST,
    NT_IDENTIFIER,
    NT_NUMBER,
    NT_STRING_LITERAL,
    NT_SEPARATOR,
    NT_COMMENT,
    NT_ARRAY_INDEX,
    NT_ELEMENT_LIST,
    NT_ARRAY_INIT,
    NT_KEYWORD, // Added
    NT_EQUAL, // Added
    NT_LOGICAL_OP, // Added
    NT_ARRAY, // Added
    NT_END_OF_TOKENS, // Added
    NT_UNKNOWN // Added
} NonTerminal;

// Actions for the parser
typedef enum {
    ACTION_SHIFT,
    ACTION_REDUCE,
    ACTION_ACCEPT,
    ACTION_ERROR
} ActionType;

// Structure for action table entry
typedef struct {
    ActionType type;
    int value; // State number for shift, rule number for reduce
} Action;
// AST Node structure (to hold AST representation)
typedef struct ASTNode {
    NonTerminal type;           // Type of node (expression, statement, etc.)
    struct ASTNode** children;  // Children nodes (if any)
    int num_children;           // Number of children
    char* value;                // Value of the node (e.g., identifier or literal)
} ASTNode;

// Structure for a parse stack item
typedef struct StackItem {
    int state;               // Current state in the parser
    Token token;             // The current token
    bool is_terminal;        // Flag indicating if the token is terminal
    NonTerminal nt;          // Non-terminal symbol (used for reduction)
    ASTNode* ast_node;       // Pointer to the AST node (created during shift or reduce)
} StackItem;

// Production rule structure
typedef struct {
    NonTerminal lhs;         // Left-hand side
    int num_symbols;         // Number of symbols on the right-hand side
    bool is_terminal[10];    // Is each symbol a terminal?
    int symbols[10];         // The right-hand side symbols (terminals or non-terminals)
} Rule;

// Stack structure
typedef struct {
    StackItem* items;
    int capacity;
    int top;
} Stack;

// Structure to hold a lexeme-specific action
typedef struct {
    char* lexeme;           // The specific lexeme value
    ActionType type;        // The action type (shift, reduce, etc.)
    int value;              // The state or rule number
} LexemeAction;

// Structure to hold a list of lexeme actions for a specific token type
typedef struct {
    TokenType token_type;   // The token type these lexeme actions apply to
    int num_actions;        // Number of lexeme-specific actions
    int capacity;           // Allocated capacity
    LexemeAction* actions;  // Array of lexeme actions
} TokenLexemeActions;


// Parser structure (includes AST root node)
typedef struct {
    Token* tokens;
    int current_token;
    Stack stack;
    Rule* rules;
    Action** action_table;
    int** goto_table;
    int num_states;
    int num_terminals;
    int num_non_terminals;
    int num_rules;
    bool has_error;
    char* error_message;

    // New AST-related fields
    ASTNode* ast_root;              // Root of the AST
    TokenLexemeActions** state_lexeme_actions;  // Array indexed by state
} Parser;

// Function Prototypes
Parser* create_parser(Token* tokens);
void parse(Parser* parser);
void print_parse_result(Parser* parser);
void free_parser(Parser* parser);

// Stack operations
void init_stack(Stack* stack);
void push(Stack* stack, StackItem item);
StackItem pop(Stack* stack);
StackItem peek(Stack* stack);
bool is_stack_empty(Stack* stack);
void free_stack(Stack* stack);

// AST-related functions
ASTNode* create_ast_node(NonTerminal type, char* value);
void add_child_to_ast_node(ASTNode* parent, ASTNode* child);
void free_ast(ASTNode* node);

// Helper functions
void init_rules(Parser* parser);
void init_parse_tables(Parser* parser);
char* get_token_type_name(TokenType type);
char* get_non_terminal_name(NonTerminal nt);
void parser_error(Parser* parser, const char* message);

int action_shift(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item);
int action_reduce(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item);
int action_accept(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item);
int action_error(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item);

void free_lexeme_actions(Parser* parser);

// Add to parser.h
typedef int (*pFun)(Parser*, Action, Token, int*, StackItem*);
typedef void (*TokenHandler)(Parser*, ASTNode*, int*, int*);

void print_ast(ASTNode* node, int indent_level);

#endif // PARSER_H