#include "parser.h"

// In main.c
int main() {
    // Step 1: Open the file and read it for tokenization
    FILE *file = fopen("test.lx", "r");
    if (!file) {
        printf("File not found!\n");
        return 1;
    }
    
    // Step 2: Run the lexical analyzer to get tokens
    int flag = 0;
    Token* tokens = lexer(file, &flag);
    fclose(file);

    // If lexer returned an error flag, terminate early
    if (flag) return 0;

    // Step 3: Create and initialize the parser
    Parser *parser = create_parser(tokens);
    
    // Step 4: Parse the input and generate the AST
    // Use the enhanced token-based AST builder instead of regular parsing
    build_ast_from_tokens(parser);

    if (parser->ast_root) {
        printf("AST created successfully.\n");
        printf("Program node has %d children\n", parser->ast_root->num_children);
        print_ast(parser->ast_root, 0);
    } else {
        printf("No AST was generated.\n");
    }

    // Step 6: Clean up the parser and tokens
    free_parser(parser);
    free_tokens(tokens);  // Free the tokens array
    
    return 0;
}


// #include "parser.h"

// int main(int argc, char *argv[]) 
// {
//     if(argc < 2)
//     {
//         printf("Error: correct syntax: %s <filename.lx>\n", argv[0]);
//         return 1;
//     }
    
//     // Open the input file
//     FILE *file = fopen(argv[1], "r");
//     if (!file) {
//         fprintf(stderr, "Error: Could not open file '%s'\n", argv[1]);
//         return 1;
//     }
    
//     // Run the lexical analyzer to get tokens
//     Token *tokens = lexer(file);
//     fclose(file);
    
//     // Create and initialize the parser
//     Parser *parser = create_parser(tokens);
    
//     // Parse the input
//     parse(parser);
    
//     // Print the result
//     print_parse_result(parser);
    
//     // Clean up
//     free_parser(parser);
    
//     // Note: The tokens are freed inside free_parser
    
//     return 0;
// }