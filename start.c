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
        printf("FATAL: Lexical analysis failed! Compilation halted due to fatal errors.\n");
        printf("       Please fix the lexical errors before continuing.\n");
        free_tokens(tokens);
        return 1;
    }
    
    // Parsing
    printf("\nPerforming parsing...\n");
    
    // Check for missing semicolons before parsing
    int missing_semicolon = 0;
    int line_with_error = 0;
    int token_count = 0;
    
    while (tokens[token_count].type != END_OF_TOKENS) {
        token_count++;
    }
    
    // Scan token stream specifically for lulog calls missing semicolons
    for (int i = 0; i < token_count - 2; i++) {
        // Look for pattern: lulog(a) followed by anything other than semicolon
        if (tokens[i].type == KEYWORD_TOKEN && 
            strcmp(tokens[i].value, "lulog") == 0 &&
            i + 3 < token_count &&
            tokens[i+1].type == SEPARATOR_TOKEN && strcmp(tokens[i+1].value, "(") == 0 &&
            // Any token in between for the argument
            tokens[i+3].type == SEPARATOR_TOKEN && strcmp(tokens[i+3].value, ")") == 0) {
            
            // No more skipping special files - check every file for errors
            
            // Debug token information
            printf("DEBUG: lulog found at token %d. ", i);
            if (i+4 < token_count) {
                printf("Next token = '%s' (type %d)\n", tokens[i+4].value, tokens[i+4].type);
            } else {
                printf("Next token is out of bounds\n");
            }
            
            // Check if the next token is not a semicolon
            if (i + 4 >= token_count || 
                !(tokens[i+4].type == SEPARATOR_TOKEN && strcmp(tokens[i+4].value, ";") == 0)) {
                
                // Check for special case with comments or inline comments
                int has_comment_after = 0;
                for (int j = i+4; j < token_count && j < i+10; j++) {
                    if (tokens[j].line_num == tokens[i+3].line_num) {
                        // If we find a comment or non-separator token on the same line
                        // after the function call, it's likely missing a semicolon
                        if (tokens[j].type != SEPARATOR_TOKEN || 
                            (tokens[j].type == SEPARATOR_TOKEN && strcmp(tokens[j].value, "}") != 0)) {
                            has_comment_after = 1;
                            break;
                        }
                    }
                }
                
                if (has_comment_after) {
                    missing_semicolon = 1;
                    line_with_error = tokens[i+3].line_num;
                    printf("FATAL: Syntax error - missing semicolon after function call on line %d\n", 
                           tokens[i+3].line_num);
                    break;
                }
                
                // Make sure it's not followed by a closing brace (which would be valid)
                if (i + 4 >= token_count || 
                    !(tokens[i+4].type == SEPARATOR_TOKEN && strcmp(tokens[i+4].value, "}") == 0)) {
                    missing_semicolon = 1;
                    line_with_error = tokens[i+3].line_num;
                    printf("FATAL: Syntax error - missing semicolon after '%s()' on line %d\n", 
                           tokens[i].value, tokens[i+3].line_num);
                    break;
                }
            }
        }
        
        // Also check for luload() calls missing semicolons
        if (tokens[i].type == KEYWORD_TOKEN && 
            strcmp(tokens[i].value, "luload") == 0 &&
            i + 2 < token_count &&
            tokens[i+1].type == SEPARATOR_TOKEN && strcmp(tokens[i+1].value, "(") == 0 &&
            tokens[i+2].type == SEPARATOR_TOKEN && strcmp(tokens[i+2].value, ")") == 0) {
            
            // Debug token information
            printf("DEBUG: luload found at token %d. ", i);
            if (i+3 < token_count) {
                printf("Next token = '%s' (type %d)\n", tokens[i+3].value, tokens[i+3].type);
            } else {
                printf("Next token is out of bounds\n");
            }
            
            // We're no longer skipping any files, check all files for errors
            // if (strstr(input_file, "lulu.lx") != NULL) {
            //     printf("Skipping semicolon check for original file: %s\n", input_file);
            //     continue;
            // }
            
            // Check if the next token is not a semicolon
            if (i + 3 < token_count && 
                !(tokens[i+3].type == SEPARATOR_TOKEN && strcmp(tokens[i+3].value, ";") == 0)) {
                
                // Make sure it's not in an assignment context
                int is_assignment = 0;
                for (int j = i-2; j >= 0 && j >= i-5; j--) {
                    if (tokens[j].type == EQUAL_TOKEN) {
                        is_assignment = 1;
                        break;
                    }
                }
                
                if (!is_assignment) {
                    missing_semicolon = 1;
                    line_with_error = tokens[i+2].line_num;
                    printf("FATAL: Syntax error - missing semicolon after '%s()' on line %d\n", 
                           tokens[i].value, tokens[i+2].line_num);
                    printf("       Missing semicolons are syntax errors that must be fixed.\n");
                    free_tokens(tokens);
                    exit(1);
                }
            }
        }
    }
    
    if (missing_semicolon) {
        free_tokens(tokens);
        printf("FATAL: Compilation failed due to syntax error on line %d\n", line_with_error);
        printf("       Missing semicolons are syntax errors that must be fixed.\n");
        return 1;
    }
    
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
    
    // Check for parsing errors
    if (parser->has_fatal_error || parser->error_count > 0) {
        printf("FATAL: Parsing failed with %d errors. Compilation halted.\n", parser->error_count);
        if (parser->error_message[0] != '\0') {
            printf("       Last error: %s\n", parser->error_message);
        }
        free_parser(parser);
        return 1;
    }
    
    if (!parser->root) {
        printf("FATAL: Parsing failed - AST root is NULL\n");
        printf("       Compilation halted due to fatal parsing errors.\n");
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
        printf("FATAL: Semantic analysis failed: %s\n", semantic_context->error_message);
        printf("       Please fix the semantic errors before continuing.\n");
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