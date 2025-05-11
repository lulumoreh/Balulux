#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include "parser.h"

// Symbol types
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER
} SymbolType;

// Symbol structure
typedef struct Symbol {
    char *name;                  // Name of the symbol
    SymbolType type;             // Type of symbol (variable, function, parameter)
    char *data_type;             // Data type (int, void, etc.)
    int scope_level;             // Scope level (0 for global, >0 for nested)
    int line_declared;           // Line number where the symbol is declared
    struct Symbol *next;         // Next symbol in the same hash bucket
} Symbol;

// Symbol table structure
typedef struct {
    Symbol **buckets;            // Hash table buckets
    int size;                    // Size of the hash table
    int scope_level;             // Current scope level
} SymbolTable;

// Create a new symbol table
SymbolTable* create_symbol_table(int size);

// Free a symbol table
void free_symbol_table(SymbolTable *table);

// Enter a new scope
void enter_scope(SymbolTable *table);

// Exit the current scope
void exit_scope(SymbolTable *table);

// Add a symbol to the table
bool add_symbol(SymbolTable *table, const char *name, SymbolType type, const char *data_type, int line);

// Look up a symbol in the table
Symbol* lookup_symbol(SymbolTable *table, const char *name);

// Print the symbol table (for debugging)
void print_symbol_table(SymbolTable *table);

// Build the symbol table from an AST
void build_symbol_table(SymbolTable *table, ASTNode *node);

#endif // SYMBOL_TABLE_H