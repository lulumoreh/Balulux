#include "lexerf.h"

int main(int argc, char *argv[]) 
{
    if(argc < 2)
    {
        printf("Error: correct syntax: %s <filename.lx>\n", argv[0]);
        exit(1);
    }
    FILE *file = fopen(argv [1], "r");
    if (!file) {
        printf("File not found!\n");
        return 1;
    }

    Token *tokens = lexer(file);

    for (int i = 0; tokens[i].type != END_OF_TOKENS; i++) 
    {
        print_token(tokens[i]);
    }

    free_tokens(tokens);
    return 0;
}