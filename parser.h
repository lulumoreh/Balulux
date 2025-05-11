#ifndef PARSER_H
#define PARSER_H

#include "lexerf.h"
#include <stdbool.h>

// Node types for AST
typedef enum {
    NODE_NONE,     // Placeholder
    NODE_PROGRAM,  // Root node
    NODE_FUNCTION, // Function declaration
    NODE_TYPE,     // Type (int, void, etc.)
    NODE_PARAM,    // Function parameter
    NODE_BLOCK,    // Code block
    NODE_STATEMENT, // Statement
    NODE_VAR_DECL, // Variable declaration
    NODE_EXPR,     // Generic expression
    NODE_BINARY_OP, // Binary operation
    NODE_RETURN,   // Return statement
    NODE_IDENTIFIER, // Identifier
    NODE_NUMBER,   // Numeric literal
    NODE_STRING,   // String literal
    NODE_IF,       // If statement
    NODE_ELSE,     // Else statement
    NODE_LULOOP,   // luloop statement
    NODE_LULOG,    // lulog statement
    NODE_LULOAD,   // luload statement
    NODE_CONDITION // Condition expression
} NodeType;

// AST node
typedef struct ASTNode {
    NodeType type;
    char* value;
    struct ASTNode** children;
    int num_children;
    int capacity;
    struct ASTNode* parent;  // Parent node for scope tracking
} ASTNode;

// Parser
typedef struct {
    Token* tokens;
    int pos;
    int token_count;  // Total number of tokens
    ASTNode* root;
    int error_count;  // Track the number of parsing errors
    int has_fatal_error; // Flag for fatal errors that should halt compilation
    char error_message[256]; // Store the last error message
} Parser;

// AST management
ASTNode* create_node(NodeType type, const char* value);
void add_child(ASTNode* parent, ASTNode* child);
void free_node(ASTNode* node);
void print_ast(ASTNode* node, int indent);

// Parser management
Parser* create_parser(Token* tokens);
void free_parser(Parser* parser);
void parse(Parser* parser);
void parser_report_error(Parser* parser, const char* message, int is_fatal);
int parser_has_errors(Parser* parser);

#endif // PARSER_H