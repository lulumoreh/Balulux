#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple hash function for strings
static unsigned int hash(const char *str, int size) {
    unsigned int hash = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = hash * 31 + str[i];
    }
    return hash % size;
}

// Create a new symbol table
SymbolTable* create_symbol_table(int size) {
    SymbolTable *table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!table) {
        fprintf(stderr, "Memory allocation failed for symbol table\n");
        return NULL;
    }
    
    table->buckets = (Symbol**)calloc(size, sizeof(Symbol*));
    if (!table->buckets) {
        fprintf(stderr, "Memory allocation failed for symbol table buckets\n");
        free(table);
        return NULL;
    }
    
    table->size = size;
    table->scope_level = 0;  // Start at global scope (level 0)
    
    return table;
}

// Free a symbol table
void free_symbol_table(SymbolTable *table) {
    if (!table) return;
    
    // Free all symbols
    for (int i = 0; i < table->size; i++) {
        Symbol *current = table->buckets[i];
        while (current) {
            Symbol *next = current->next;
            free(current->name);
            free(current->data_type);
            free(current);
            current = next;
        }
    }
    
    // Free buckets and table
    free(table->buckets);
    free(table);
}

// Enter a new scope
void enter_scope(SymbolTable *table) {
    if (!table) return;
    table->scope_level++;
}

// Exit the current scope
void exit_scope(SymbolTable *table) {
    if (!table || table->scope_level == 0) return;
    
    // For the final print, we're NOT removing symbols when exiting a scope
    // This allows us to see ALL variables that were defined, even after the scope has closed
    // In a real compiler, you would typically remove them as shown in the commented code below
    
    /*
    // Remove all symbols in the current scope
    for (int i = 0; i < table->size; i++) {
        Symbol **current = &(table->buckets[i]);
        while (*current) {
            if ((*current)->scope_level == table->scope_level) {
                Symbol *to_remove = *current;
                *current = to_remove->next;
                free(to_remove->name);
                free(to_remove->data_type);
                free(to_remove);
            } else {
                current = &((*current)->next);
            }
        }
    }
    */
    
    table->scope_level--;
}

// Add a symbol to the table
bool add_symbol(SymbolTable *table, const char *name, SymbolType type, const char *data_type, int line) {
    if (!table || !name || !data_type) return false;
    
    // Check if symbol already exists in the current scope
    unsigned int index = hash(name, table->size);
    Symbol *current = table->buckets[index];
    
    while (current) {
        if (current->scope_level == table->scope_level && strcmp(current->name, name) == 0) {
            fprintf(stderr, "Symbol '%s' already defined at line %d\n", name, current->line_declared);
            return false;
        }
        current = current->next;
    }
    
    // Create new symbol
    Symbol *new_symbol = (Symbol*)malloc(sizeof(Symbol));
    if (!new_symbol) {
        fprintf(stderr, "Memory allocation failed for symbol\n");
        return false;
    }
    
    new_symbol->name = strdup(name);
    new_symbol->data_type = strdup(data_type);
    new_symbol->type = type;
    new_symbol->scope_level = table->scope_level;
    new_symbol->line_declared = line;
    
    // Add to beginning of list for this bucket
    new_symbol->next = table->buckets[index];
    table->buckets[index] = new_symbol;
    
    return true;
}

// Look up a symbol in the table
Symbol* lookup_symbol(SymbolTable *table, const char *name) {
    if (!table || !name) return NULL;
    
    unsigned int index = hash(name, table->size);
    Symbol *current = table->buckets[index];
    Symbol *best_match = NULL;
    
    // Find the symbol with the highest scope level (most local)
    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (!best_match || current->scope_level > best_match->scope_level) {
                best_match = current;
            }
        }
        current = current->next;
    }
    
    return best_match;
}

// Print the symbol table (for debugging)
void print_symbol_table(SymbolTable *table) {
    if (!table) return;
    
    printf("\n=== Symbol Table (Current Scope: %d) ===\n", table->scope_level);
    printf("%-15s %-12s %-10s %-10s %-10s\n", 
           "Name", "Type", "Data Type", "Scope", "Line");
    printf("----------------------------------------\n");
    
    // For each bucket
    for (int i = 0; i < table->size; i++) {
        Symbol *current = table->buckets[i];
        
        // For each symbol in the bucket
        while (current) {
            const char *type_str;
            switch (current->type) {
                case SYMBOL_VARIABLE: type_str = "Variable"; break;
                case SYMBOL_FUNCTION: type_str = "Function"; break;
                case SYMBOL_PARAMETER: type_str = "Parameter"; break;
                default: type_str = "Unknown";
            }
            
            printf("%-15s %-12s %-10s %-10d %-10d\n",
                   current->name, type_str, current->data_type, 
                   current->scope_level, current->line_declared);
                   
            current = current->next;
        }
    }
    
    printf("====================\n\n");
}

// Helper function to process function declarations
static void process_function(SymbolTable *table, ASTNode *function_node) {
    if (!table || !function_node || function_node->type != NODE_FUNCTION) return;
    
    // Get function name
    const char *function_name = function_node->value;
    
    // Find the return type
    const char *return_type = "void";  // Default
    for (int i = 0; i < function_node->num_children; i++) {
        if (function_node->children[i]->type == NODE_TYPE) {
            return_type = function_node->children[i]->value;
            break;
        }
    }
    
    // Add function to symbol table
    add_symbol(table, function_name, SYMBOL_FUNCTION, return_type, 0);  // Line number not available
    printf("Added function %s with return type %s to scope %d\n", function_name, return_type, table->scope_level);
    
    // Enter new scope for function body
    enter_scope(table);
    printf("Entered function scope %d for %s\n", table->scope_level, function_name);
    
    // Process parameters
    for (int i = 0; i < function_node->num_children; i++) {
        if (function_node->children[i]->type == NODE_PARAM) {
            ASTNode *param_list = function_node->children[i];
            
            // Process each parameter
            for (int j = 0; j < param_list->num_children; j++) {
                ASTNode *param = param_list->children[j];
                // Parameters can be created as NODE_VAR_DECL (from parse_parameters) or NODE_PARAM
                if (param->type == NODE_PARAM || param->type == NODE_VAR_DECL) {
                    const char *param_name = param->value;
                    const char *param_type = "int";  // Default
                    
                    // Find parameter type
                    for (int k = 0; k < param->num_children; k++) {
                        if (param->children[k]->type == NODE_TYPE) {
                            param_type = param->children[k]->value;
                            break;
                        }
                    }
                    
                    // Add parameter to symbol table
                    add_symbol(table, param_name, SYMBOL_PARAMETER, param_type, 0);  // Line number not available
                    printf("Added parameter %s of type %s to scope %d\n", param_name, param_type, table->scope_level);
                }
            }
        }
    }
    
    // Process function body 
    for (int i = 0; i < function_node->num_children; i++) {
        if (function_node->children[i]->type == NODE_BLOCK) {
            // Don't enter a new scope here, as the block will do that itself
            // Just directly process the block's children
            ASTNode *block = function_node->children[i];
            for (int j = 0; j < block->num_children; j++) {
                build_symbol_table(table, block->children[j]);
            }
            break;
        }
    }
    
    // Exit function scope
    printf("Exiting function scope %d for %s\n", table->scope_level, function_name);
    exit_scope(table);
}

// Helper function to process variable declarations
static void process_variable(SymbolTable *table, ASTNode *var_node) {
    if (!table || !var_node || var_node->type != NODE_VAR_DECL) return;
    
    // Get variable name
    const char *var_name = var_node->value;
    
    // Find variable type
    const char *var_type = "int";  // Default
    for (int i = 0; i < var_node->num_children; i++) {
        if (var_node->children[i]->type == NODE_TYPE) {
            var_type = var_node->children[i]->value;
            break;
        }
    }
    
    // Add variable to symbol table with current scope
    add_symbol(table, var_name, SYMBOL_VARIABLE, var_type, 0);  // Line number not available
    
    // Debug print
    printf("Added variable %s of type %s to scope %d\n", var_name, var_type, table->scope_level);
}

// Recursive function to build symbol table from AST
void build_symbol_table(SymbolTable *table, ASTNode *node) {
    if (!table || !node) return;
    
    // Process current node
    switch (node->type) {
        case NODE_FUNCTION:
            process_function(table, node);
            break;
            
        case NODE_VAR_DECL:
            process_variable(table, node);
            break;
            
        case NODE_BLOCK:
            // Check if the parent is a function node
            // If so, don't enter a new scope as function already provides one
            ASTNode *parent = node->parent;
            bool is_function_body = (parent && parent->type == NODE_FUNCTION);
            
            if (!is_function_body) {
                // Enter new scope for non-function blocks
                enter_scope(table);
                printf("Entered block scope %d\n", table->scope_level);
            } else {
                printf("Processing function body without new scope: %d\n", table->scope_level);
            }
            
            // Process all statements in the block
            for (int i = 0; i < node->num_children; i++) {
                build_symbol_table(table, node->children[i]);
            }
            
            // Exit block scope (only if we entered one)
            if (!is_function_body) {
                printf("Exiting block scope %d\n", table->scope_level);
                exit_scope(table);
            }
            break;
            
        case NODE_IF:
            // Process all children (condition, if-block, else-node)
            for (int i = 0; i < node->num_children; i++) {
                build_symbol_table(table, node->children[i]);
            }
            break;
            
        case NODE_ELSE:
            // Process else block
            for (int i = 0; i < node->num_children; i++) {
                build_symbol_table(table, node->children[i]);
            }
            break;
            
        case NODE_LULOOP:
            // Process all children (condition, loop-block)
            for (int i = 0; i < node->num_children; i++) {
                build_symbol_table(table, node->children[i]);
            }
            break;
            
        case NODE_PROGRAM:
            // Process all top-level declarations
            for (int i = 0; i < node->num_children; i++) {
                build_symbol_table(table, node->children[i]);
            }
            break;
            
        default:
            // Process other node types if needed
            break;
    }
}