#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"
#include "symbol_table.h"

// Semantic error types
typedef enum {
    SEM_ERROR_NONE,
    SEM_ERROR_UNDEFINED_VARIABLE,
    SEM_ERROR_UNDEFINED_FUNCTION,
    SEM_ERROR_TYPE_MISMATCH,
    SEM_ERROR_INVALID_OPERATION,
    SEM_ERROR_PARAMETER_COUNT,
    SEM_ERROR_RETURN_TYPE_MISMATCH,
    SEM_ERROR_DIVISION_BY_ZERO,
    SEM_ERROR_DUPLICATE_DECLARATION
} SemanticErrorType;

// Semantic context
typedef struct {
    SymbolTable *symbol_table;
    char *current_function;
    char *current_function_return_type;
    int error_count;
    char error_message[256];
} SemanticContext;

// Initialize semantic analyzer
SemanticContext* initialize_semantic_analyzer(SymbolTable *symbol_table);

// Free semantic analyzer
void free_semantic_analyzer(SemanticContext *context);

// Perform semantic analysis on the AST
bool analyze_semantics(SemanticContext *context, ASTNode *root);

// Check types of an expression
char* get_expression_type(SemanticContext *context, ASTNode *expr);

// Report a semantic error
void report_semantic_error(SemanticContext *context, SemanticErrorType error, 
                         const char *message, int line);

#endif // SEMANTIC_H