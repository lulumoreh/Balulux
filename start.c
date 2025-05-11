#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexerf.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"
#include "codegen.h"

int main(int argc, char** argv) {
    const char* version = "1.0";
    const char* input_file = NULL;
    const char* output_file = NULL;
    
    // Process command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("BALULUX Compiler Version %s\n", version);
            printf("Usage: %s [options] <source_file.lx>\n\n", argv[0]);
            printf("Options:\n");
            printf("  -o <file>       Specify output file name (default: source_file_name.asm)\n");
            printf("  --help          Display this help message\n");
            printf("  --version       Display compiler version information\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("BALULUX Compiler Version %s\n", version);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            // Make sure there's a filename after -o
            if (i + 1 < argc) {
                output_file = argv[i + 1];
                i++; // Skip next argument since we've processed it
            } else {
                printf("Error: Missing filename after -o option\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            printf("Error: Unknown option '%s'\n", argv[i]);
            printf("Use --help for more information\n");
            return 1;
        } else {
            // If not an option, assume it's the input file
            if (input_file == NULL) {
                input_file = argv[i];
            } else {
                printf("Error: Too many input files specified. Only one file is allowed.\n");
                return 1;
            }
        }
    }
    
    // Check if input file was provided
    if (input_file == NULL) {
        printf("Error: No input file specified\n");
        printf("Usage: %s [options] <source_file.lx>\n", argv[0]);
        printf("Use --help for more information\n");
        return 1;
    }
    
    // Generate default output filename if not specified
    if (output_file == NULL) {
        // Allocate memory for the default output filename
        char* temp_output = malloc(strlen(input_file) + 5); // +5 for ".asm\0"
        if (!temp_output) {
            printf("Error: Memory allocation failed\n");
            return 1;
        }
        
        // Copy input filename without extension
        strcpy(temp_output, input_file);
        
        // Replace extension with .asm
        char* extension = strrchr(temp_output, '.');
        if (extension) {
            strcpy(extension, ".asm");
        } else {
            // If no extension found, just append .asm
            strcat(temp_output, ".asm");
        }
        
        output_file = temp_output;
    }
    
    // Open the input file
    FILE* file = fopen(input_file, "r");
    if (!file) {
        printf("Error: Input file '%s' not found!\n", input_file);
        return 1;
    }
    
    printf("Compiling %s to %s...\n", input_file, output_file);
    
    // Lexical analysis
    printf("Performing lexical analysis...\n");
    int error_flag = 0;
    Token* tokens = lexer(file, &error_flag);
    fclose(file);
    
    if (error_flag) {
        printf("Lexical analysis failed!\n");
        free_tokens(tokens);
        return 1;
    }
    
    // Parsing
    printf("\nPerforming parsing...\n");
    
    // Debug token stream
    printf("Token stream before parsing:\n");
    for (int i = 0; tokens[i].type != END_OF_TOKENS; i++) {
        const char* type_name = "UNKNOWN";
        switch (tokens[i].type) {
            case TYPE_TOKEN: type_name = "TYPE"; break;
            case KEYWORD_TOKEN: type_name = "KEYWORD"; break;
            case IDENTIFIER_TOKEN: type_name = "IDENTIFIER"; break;
            case NUMBER_TOKEN: type_name = "NUMBER"; break;
            case STRING_LITERAL_TOKEN: type_name = "STRING"; break;
            case EQUAL_TOKEN: type_name = "EQUAL"; break;
            case OPERATOR_TOKEN: type_name = "OPERATOR"; break;
            case SEPARATOR_TOKEN: type_name = "SEPARATOR"; break;
        }
        printf("Token %d: [%s] '%s' (line %d)\n", 
               i, type_name, tokens[i].value ? tokens[i].value : "NULL", tokens[i].line_num);
    }
    
    Parser* parser = create_parser(tokens);
    if (!parser) {
        printf("Failed to create parser\n");
        free_tokens(tokens);
        return 1;
    }
    
    printf("Starting parse()...\n");
    parse(parser);
    
    if (!parser->root) {
        printf("Parsing failed - AST root is NULL\n");
        free_parser(parser);
        return 1;
    }
    
    printf("AST built successfully.\n");
    
    // Print AST for debugging
    printf("Abstract Syntax Tree:\n");
    print_ast(parser->root, 0);
    
    // Create symbol table
    printf("\nBuilding symbol table...\n");
    SymbolTable* symbol_table = create_symbol_table(100);
    if (!symbol_table) {
        printf("Failed to create symbol table\n");
        free_parser(parser);
        return 1;
    }
    
    build_symbol_table(symbol_table, parser->root);
    print_symbol_table(symbol_table);
    
    // Semantic analysis
    printf("\nPerforming semantic analysis...\n");
    SemanticContext* semantic_context = initialize_semantic_analyzer(symbol_table);
    if (!semantic_context) {
        printf("Failed to initialize semantic analyzer\n");
        free_symbol_table(symbol_table);
        free_parser(parser);
        return 1;
    }
    
    bool semantic_ok = analyze_semantics(semantic_context, parser->root);
    if (!semantic_ok) {
        printf("Semantic analysis failed: %s\n", semantic_context->error_message);
        free_semantic_analyzer(semantic_context);
        free_symbol_table(symbol_table);
        free_parser(parser);
        return 1;
    }
    
    printf("Semantic analysis successful\n");
    
    // Code generation
    printf("\nGenerating code...\n");
    CodeGenContext* generator = initialize_code_generator(output_file, symbol_table, input_file);
    if (!generator) {
        printf("Failed to initialize code generator\n");
        free_semantic_analyzer(semantic_context);
        free_symbol_table(symbol_table);
        free_parser(parser);
        return 1;
    }
    
    bool codegen_ok = generate_code(generator, parser->root);
    if (!codegen_ok) {
        printf("Code generation failed\n");
        free_code_generator(generator);
        free_semantic_analyzer(semantic_context);
        free_symbol_table(symbol_table);
        free_parser(parser);
        return 1;
    }
    
    printf("Code generation successful\n");
    printf("Assembly code written to %s\n", output_file);
    
    // Clean up
    free_code_generator(generator);
    free_semantic_analyzer(semantic_context);
    free_symbol_table(symbol_table);
    free_parser(parser);
    free_tokens(tokens);
    
    // Free the dynamically allocated output filename if we created it
    // We only need to free it if we automatically generated the name
    // This happens when no -o option was provided
    if (output_file != NULL) {
        // Check if we need to free: Only free if it was created in our default case
        // and not directly assigned from command line arguments
        int was_from_cmdline = 0;
        for (int i = 1; i < argc; i++) {
            if (output_file == argv[i]) {
                was_from_cmdline = 1;
                break;
            }
        }
        
        if (!was_from_cmdline) {
            free((void*)output_file);
        }
    }
    
    return 0;
}