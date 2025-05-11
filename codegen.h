#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "symbol_table.h"
#include "semantic.h"

// Code generation context
typedef struct {
    FILE *output_file;           // Output assembly file
    SymbolTable *symbol_table;   // Symbol table
    int label_counter;           // For generating unique labels
    int indent_level;            // For formatting the output
    const char *current_function; // Current function being processed (uses static buffer)
    const char *input_filename;   // Source file name
    
    // Track variable offsets for the current function
    struct VarOffset {
        char name[64];           // Variable name
        int offset;              // Stack offset (from bp)
    } var_offsets[100];
    int var_count;               // Number of variables tracked in this function
} CodeGenContext;

// Initialize code generator
CodeGenContext* initialize_code_generator(const char *output_filename, SymbolTable *symbol_table, const char *input_filename);

// Free code generator resources
void free_code_generator(CodeGenContext *context);

// Generate assembly code from AST
bool generate_code(CodeGenContext *context, ASTNode *root);

// Utility functions
void write_line(CodeGenContext *context, const char *format, ...);
void write_instruction(CodeGenContext *context, const char *format, ...);
void write_label(CodeGenContext *context, const char *format, ...);
void write_comment(CodeGenContext *context, const char *format, ...);
void write_section(CodeGenContext *context, const char *section);

// Generate a unique label
char* generate_label(CodeGenContext *context, const char *prefix);

#endif // CODEGEN_H