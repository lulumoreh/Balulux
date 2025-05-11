#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of helper functions
static bool analyze_function(SemanticContext *context, ASTNode *function);
static bool analyze_block(SemanticContext *context, ASTNode *block);
static bool analyze_variable_declaration(SemanticContext *context, ASTNode *var_decl);
static bool analyze_expression(SemanticContext *context, ASTNode *expr);
static bool analyze_if_statement(SemanticContext *context, ASTNode *if_stmt);
static bool analyze_luloop_statement(SemanticContext *context, ASTNode *luloop);
static bool analyze_lulog_statement(SemanticContext *context, ASTNode *lulog);
static bool analyze_luload_statement(SemanticContext *context, ASTNode *luload);
static bool analyze_return_statement(SemanticContext *context, ASTNode *ret);
static bool analyze_condition(SemanticContext *context, ASTNode *cond);
static bool analyze_function_call(SemanticContext *context, ASTNode *call);
static bool analyze_binary_operation(SemanticContext *context, ASTNode *binary_op);
static bool check_assignment_type(SemanticContext *context, const char *var_type, 
                                 const char *expr_type, int line);

// Initialize semantic analyzer
SemanticContext* initialize_semantic_analyzer(SymbolTable *symbol_table) {
    SemanticContext *context = (SemanticContext *)malloc(sizeof(SemanticContext));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for semantic analyzer\n");
        return NULL;
    }
    
    context->symbol_table = symbol_table;
    context->current_function = NULL;
    context->current_function_return_type = NULL;
    context->error_count = 0;
    context->error_message[0] = '\0';
    
    return context;
}

// Free semantic analyzer
void free_semantic_analyzer(SemanticContext *context) {
    if (context) {
        if (context->current_function) {
            free(context->current_function);
        }
        if (context->current_function_return_type) {
            free(context->current_function_return_type);
        }
        free(context);
    }
}

// Report a semantic error
void report_semantic_error(SemanticContext *context, SemanticErrorType error, 
                         const char *message, int line) {
    const char *error_type_str = "Unknown error";
    
    switch (error) {
        case SEM_ERROR_UNDEFINED_VARIABLE:
            error_type_str = "Undefined variable";
            break;
        case SEM_ERROR_UNDEFINED_FUNCTION:
            error_type_str = "Undefined function";
            break;
        case SEM_ERROR_TYPE_MISMATCH:
            error_type_str = "Type mismatch";
            break;
        case SEM_ERROR_INVALID_OPERATION:
            error_type_str = "Invalid operation";
            break;
        case SEM_ERROR_PARAMETER_COUNT:
            error_type_str = "Parameter count mismatch";
            break;
        case SEM_ERROR_RETURN_TYPE_MISMATCH:
            error_type_str = "Return type mismatch";
            break;
        case SEM_ERROR_DIVISION_BY_ZERO:
            error_type_str = "Division by zero";
            break;
        case SEM_ERROR_DUPLICATE_DECLARATION:
            error_type_str = "Duplicate declaration";
            break;
        default:
            break;
    }
    
    fprintf(stderr, "FATAL: Semantic error [%s] at line %d: %s\n", error_type_str, line, message);
    fprintf(stderr, "       Please fix the semantic errors before continuing.\n");
    
    // Immediately exit on semantic error
    exit(1);
    context->error_count++;
    
    // Store the error message if it's the first one
    if (context->error_message[0] == '\0') {
        snprintf(context->error_message, sizeof(context->error_message), 
                 "Semantic error [%s] at line %d: %s", error_type_str, line, message);
    }
}

// Perform semantic analysis on the AST
bool analyze_semantics(SemanticContext *context, ASTNode *root) {
    if (!context || !root) return false;
    
    if (root->type != NODE_PROGRAM) {
        fprintf(stderr, "Expected program node at root\n");
        return false;
    }
    
    // Analyze each function in the program
    bool success = true;
    for (int i = 0; i < root->num_children; i++) {
        ASTNode *child = root->children[i];
        if (child->type == NODE_FUNCTION) {
            if (!analyze_function(context, child)) {
                success = false;
            }
        }
    }
    
    return success && (context->error_count == 0);
}

// Check if types are compatible (for assignments and comparisons)
static bool are_types_compatible(const char *type1, const char *type2) {
    if (strcmp(type1, type2) == 0) return true;
    
    // For simplicity, we only allow exact matches
    // In a more complex language, this would check type compatibility rules
    return false;
}

// Get the type of an expression
char* get_expression_type(SemanticContext *context, ASTNode *expr) {
    if (!context || !expr) return NULL;
    
    switch (expr->type) {
        case NODE_NUMBER:
            return strdup("int");
            
        case NODE_STRING:
            return strdup("string");
            
        case NODE_LULOAD:
            // luload returns an integer value
            return strdup("int");
            
        case NODE_IDENTIFIER: {
            Symbol *symbol = lookup_symbol(context->symbol_table, expr->value);
            if (!symbol) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Undefined variable '%s'", expr->value);
                report_semantic_error(context, SEM_ERROR_UNDEFINED_VARIABLE, msg, 0);
                return NULL;
            }
            return strdup(symbol->data_type);
        }
        
        case NODE_BINARY_OP: {
            char *left_type = get_expression_type(context, expr->children[0]);
            char *right_type = get_expression_type(context, expr->children[1]);
            
            if (!left_type || !right_type) {
                free(left_type);
                free(right_type);
                return NULL;
            }
            
            // Type checking for binary operations
            if (strcmp(left_type, "int") != 0 || strcmp(right_type, "int") != 0) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Binary operation '%s' requires int operands", expr->value);
                report_semantic_error(context, SEM_ERROR_TYPE_MISMATCH, msg, 0);
                free(left_type);
                free(right_type);
                return NULL;
            }
            
            free(left_type);
            free(right_type);
            return strdup("int");
        }
        
        case NODE_EXPR: {
            // For assignment expressions
            if (strcmp(expr->value, "=") == 0) {
                if (expr->num_children < 2) return NULL;
                
                // Left side must be an identifier
                ASTNode *left = expr->children[0];
                if (left->type != NODE_IDENTIFIER) {
                    report_semantic_error(context, SEM_ERROR_INVALID_OPERATION,
                                        "Left side of assignment must be a variable", 0);
                    return NULL;
                }
                
                // Check if the variable exists
                Symbol *symbol = lookup_symbol(context->symbol_table, left->value);
                if (!symbol) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Undefined variable '%s' in assignment", left->value);
                    report_semantic_error(context, SEM_ERROR_UNDEFINED_VARIABLE, msg, 0);
                    return NULL;
                }
                
                // Check the type of the right side
                char *right_type = get_expression_type(context, expr->children[1]);
                if (!right_type) return NULL;
                
                // Check if types are compatible
                if (!are_types_compatible(symbol->data_type, right_type)) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Cannot assign %s to %s", right_type, symbol->data_type);
                    report_semantic_error(context, SEM_ERROR_TYPE_MISMATCH, msg, 0);
                    free(right_type);
                    return NULL;
                }
                
                free(right_type);
                return strdup(symbol->data_type);
            } 
            // Function call
            else {
                // TODO: Implement function call type determination
                // For now, we'll assume int return type
                return strdup("int");
            }
        }
        
        default:
            return NULL;
    }
}

// Check if an assignment type is valid
static bool check_assignment_type(SemanticContext *context, const char *var_type, 
                                 const char *expr_type, int line) {
    if (!var_type || !expr_type) return false;
    
    if (!are_types_compatible(var_type, expr_type)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Cannot assign value of type '%s' to variable of type '%s'",
                 expr_type, var_type);
        report_semantic_error(context, SEM_ERROR_TYPE_MISMATCH, msg, line);
        return false;
    }
    
    return true;
}

// Analyze a function declaration
static bool analyze_function(SemanticContext *context, ASTNode *function) {
    if (!context || !function || function->type != NODE_FUNCTION) return false;
    
    // Save current function context
    char *prev_func = context->current_function;
    char *prev_return_type = context->current_function_return_type;
    
    // Set current function context
    context->current_function = strdup(function->value);
    
    // Get return type
    const char *return_type = "void";  // Default if not found
    for (int i = 0; i < function->num_children; i++) {
        if (function->children[i]->type == NODE_TYPE) {
            return_type = function->children[i]->value;
            break;
        }
    }
    context->current_function_return_type = strdup(return_type);
    
    // Process parameters (already added to symbol table during symbol table construction)
    
    // Process function body
    bool success = true;
    for (int i = 0; i < function->num_children; i++) {
        if (function->children[i]->type == NODE_BLOCK) {
            if (!analyze_block(context, function->children[i])) {
                success = false;
            }
            break;
        }
    }
    
    // Restore previous function context
    free(context->current_function);
    free(context->current_function_return_type);
    context->current_function = prev_func;
    context->current_function_return_type = prev_return_type;
    
    return success;
}

// Analyze a code block
static bool analyze_block(SemanticContext *context, ASTNode *block) {
    if (!context || !block || block->type != NODE_BLOCK) return false;
    
    bool success = true;
    for (int i = 0; i < block->num_children; i++) {
        ASTNode *stmt = block->children[i];
        
        switch (stmt->type) {
            case NODE_VAR_DECL:
                if (!analyze_variable_declaration(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_EXPR:
                if (!analyze_expression(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_RETURN:
                if (!analyze_return_statement(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_IF:
                if (!analyze_if_statement(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_LULOOP:
                if (!analyze_luloop_statement(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_LULOG:
                if (!analyze_lulog_statement(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_LULOAD:
                if (!analyze_luload_statement(context, stmt)) {
                    success = false;
                }
                break;
                
            case NODE_BLOCK:
                if (!analyze_block(context, stmt)) {
                    success = false;
                }
                break;
                
            default:
                fprintf(stderr, "Unexpected node type in block: %d\n", stmt->type);
                success = false;
                break;
        }
    }
    
    return success;
}

// Analyze a variable declaration
static bool analyze_variable_declaration(SemanticContext *context, ASTNode *var_decl) {
    if (!context || !var_decl || var_decl->type != NODE_VAR_DECL) return false;
    
    const char *var_name = var_decl->value;
    
    // Get variable type
    const char *var_type = "int";  // Default
    for (int i = 0; i < var_decl->num_children; i++) {
        if (var_decl->children[i]->type == NODE_TYPE) {
            var_type = var_decl->children[i]->value;
            break;
        }
    }
    
    // Check initializer if present
    for (int i = 0; i < var_decl->num_children; i++) {
        ASTNode *child = var_decl->children[i];
        if (child->type != NODE_TYPE) {
            char *expr_type = get_expression_type(context, child);
            if (!expr_type) return false;
            
            bool result = check_assignment_type(context, var_type, expr_type, 0);
            free(expr_type);
            
            if (!result) return false;
        }
    }
    
    return true;
}

// Analyze an expression
static bool analyze_expression(SemanticContext *context, ASTNode *expr) {
    if (!context || !expr) return false;
    
    switch (expr->type) {
        case NODE_EXPR: {
            // Assignment expression
            if (strcmp(expr->value, "=") == 0) {
                if (expr->num_children < 2) return false;
                
                // Left side must be an identifier
                ASTNode *left = expr->children[0];
                if (left->type != NODE_IDENTIFIER) {
                    report_semantic_error(context, SEM_ERROR_INVALID_OPERATION,
                                        "Left side of assignment must be a variable", 0);
                    return false;
                }
                
                // Check if the variable exists
                Symbol *symbol = lookup_symbol(context->symbol_table, left->value);
                if (!symbol) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Undefined variable '%s' in assignment", left->value);
                    report_semantic_error(context, SEM_ERROR_UNDEFINED_VARIABLE, msg, 0);
                    return false;
                }
                
                // Check the right side
                ASTNode *right = expr->children[1];
                char *right_type = get_expression_type(context, right);
                if (!right_type) return false;
                
                bool result = check_assignment_type(context, symbol->data_type, right_type, 0);
                free(right_type);
                
                return result;
            }
            // Function call
            else {
                return analyze_function_call(context, expr);
            }
        }
        
        case NODE_BINARY_OP:
            return analyze_binary_operation(context, expr);
            
        case NODE_IDENTIFIER: {
            Symbol *symbol = lookup_symbol(context->symbol_table, expr->value);
            if (!symbol) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Undefined variable '%s'", expr->value);
                report_semantic_error(context, SEM_ERROR_UNDEFINED_VARIABLE, msg, 0);
                return false;
            }
            return true;
        }
        
        case NODE_NUMBER:
        case NODE_STRING:
            return true;
            
        default:
            return false;
    }
}

// Analyze an if statement
static bool analyze_if_statement(SemanticContext *context, ASTNode *if_stmt) {
    if (!context || !if_stmt || if_stmt->type != NODE_IF) return false;
    
    bool success = true;
    
    // Analyze condition
    for (int i = 0; i < if_stmt->num_children; i++) {
        ASTNode *child = if_stmt->children[i];
        
        if (child->type == NODE_CONDITION) {
            if (!analyze_condition(context, child)) {
                success = false;
            }
        }
        else if (child->type == NODE_BLOCK) {
            if (!analyze_block(context, child)) {
                success = false;
            }
        }
        else if (child->type == NODE_ELSE) {
            for (int j = 0; j < child->num_children; j++) {
                if (child->children[j]->type == NODE_BLOCK) {
                    if (!analyze_block(context, child->children[j])) {
                        success = false;
                    }
                }
            }
        }
    }
    
    return success;
}

// Analyze a luloop statement
static bool analyze_luloop_statement(SemanticContext *context, ASTNode *luloop) {
    if (!context || !luloop || luloop->type != NODE_LULOOP) return false;
    
    bool success = true;
    
    // Analyze condition
    for (int i = 0; i < luloop->num_children; i++) {
        ASTNode *child = luloop->children[i];
        
        if (child->type == NODE_CONDITION) {
            if (!analyze_condition(context, child)) {
                success = false;
            }
        }
        else if (child->type == NODE_BLOCK) {
            if (!analyze_block(context, child)) {
                success = false;
            }
        }
    }
    
    return success;
}

// Analyze a lulog statement
static bool analyze_lulog_statement(SemanticContext *context, ASTNode *lulog) {
    if (!context || !lulog || lulog->type != NODE_LULOG) return false;
    
    // lulog can output any expression, so we just need to check that the expression is valid
    if (lulog->num_children > 0) {
        ASTNode *expr = lulog->children[0];
        char *expr_type = get_expression_type(context, expr);
        
        if (!expr_type) return false;
        
        // lulog can handle any type
        free(expr_type);
        return true;
    }
    
    return true;
}

// Analyze a luload statement
static bool analyze_luload_statement(SemanticContext *context, ASTNode *luload) {
    if (!context || !luload || luload->type != NODE_LULOAD) return false;
    
    // luload doesn't take arguments, it just returns an integer from user input
    // Nothing to verify here except the node type, which we already checked
    return true;
}

// Analyze a return statement
static bool analyze_return_statement(SemanticContext *context, ASTNode *ret) {
    if (!context || !ret || ret->type != NODE_RETURN) return false;
    
    // Get the expected return type from the function
    const char *expected_type = context->current_function_return_type;
    if (!expected_type) {
        report_semantic_error(context, SEM_ERROR_INVALID_OPERATION,
                            "Return statement outside of function", 0);
        return false;
    }
    
    // Void functions can have empty return
    if (strcmp(expected_type, "void") == 0) {
        if (ret->num_children > 0) {
            report_semantic_error(context, SEM_ERROR_RETURN_TYPE_MISMATCH,
                                "Void function cannot return a value", 0);
            return false;
        }
        return true;
    }
    
    // Non-void functions must return a value
    if (ret->num_children == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Function '%s' must return a value of type '%s'",
                 context->current_function, expected_type);
        report_semantic_error(context, SEM_ERROR_RETURN_TYPE_MISMATCH, msg, 0);
        return false;
    }
    
    // Check the type of the returned expression
    ASTNode *expr = ret->children[0];
    char *expr_type = get_expression_type(context, expr);
    if (!expr_type) return false;
    
    if (!are_types_compatible(expected_type, expr_type)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Cannot return %s from function with return type %s",
                 expr_type, expected_type);
        report_semantic_error(context, SEM_ERROR_RETURN_TYPE_MISMATCH, msg, 0);
        free(expr_type);
        return false;
    }
    
    free(expr_type);
    return true;
}

// Analyze a condition
static bool analyze_condition(SemanticContext *context, ASTNode *cond) {
    if (!context || !cond || cond->type != NODE_CONDITION) return false;
    
    if (cond->num_children == 0) return false;
    
    // Conditions typically have a binary operation
    ASTNode *expr = cond->children[0];
    char *expr_type = get_expression_type(context, expr);
    
    if (!expr_type) return false;
    
    // Conditions should evaluate to int (boolean)
    if (strcmp(expr_type, "int") != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Condition must be of type int, got %s", expr_type);
        report_semantic_error(context, SEM_ERROR_TYPE_MISMATCH, msg, 0);
        free(expr_type);
        return false;
    }
    
    free(expr_type);
    return true;
}

// Analyze a function call
static bool analyze_function_call(SemanticContext *context, ASTNode *call) {
    if (!context || !call) return false;
    
    // TODO: Implement function call analysis with parameter checking
    // For now, we'll assume all function calls are valid
    
    return true;
}

// Analyze a binary operation
static bool analyze_binary_operation(SemanticContext *context, ASTNode *binary_op) {
    if (!context || !binary_op || binary_op->type != NODE_BINARY_OP) return false;
    
    if (binary_op->num_children < 2) return false;
    
    char *left_type = get_expression_type(context, binary_op->children[0]);
    char *right_type = get_expression_type(context, binary_op->children[1]);
    
    if (!left_type || !right_type) {
        free(left_type);
        free(right_type);
        return false;
    }
    
    const char *op = binary_op->value;
    
    // Check for division by zero
    if (strcmp(op, "/") == 0 && 
        binary_op->children[1]->type == NODE_NUMBER && 
        strcmp(binary_op->children[1]->value, "0") == 0) {
        report_semantic_error(context, SEM_ERROR_DIVISION_BY_ZERO,
                            "Division by zero", 0);
        free(left_type);
        free(right_type);
        return false;
    }
    
    // Type checking for binary operations
    if (strcmp(left_type, "int") != 0 || strcmp(right_type, "int") != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Binary operation '%s' requires int operands, got %s and %s",
                op, left_type, right_type);
        report_semantic_error(context, SEM_ERROR_TYPE_MISMATCH, msg, 0);
        free(left_type);
        free(right_type);
        return false;
    }
    
    free(left_type);
    free(right_type);
    return true;
}