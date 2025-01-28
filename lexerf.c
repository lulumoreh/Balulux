#include "lexerf.h"

// Global Variables
State transition_matrix[8][256];
int line_number = 1;

// Functions
void initialize_transition_matrix() 
{
    for (int i = 0; i < 8; i++) 
    {
        for (int j = 0; j < 256; j++) 
        {
            transition_matrix[i][j] = ERROR;
        }
    }

    //Define transitions for digits (number)
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[START][i] = NUMBER;
        transition_matrix[NUMBER][i] = NUMBER;
    }
    
    transition_matrix[NUMBER][' '] = ACCEPT;
    transition_matrix[NUMBER]['='] = ACCEPT;

    //Define transitions for identifiers (a-z, A-Z, 0-9)
    for (int i = 'a'; i <= 'z'; i++) 
    {
        transition_matrix[START][i] = IDENTIFIER;
        transition_matrix[IDENTIFIER][i] = IDENTIFIER;
    }
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[START][i] = IDENTIFIER;
        transition_matrix[IDENTIFIER][i] = IDENTIFIER;
    }
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[IDENTIFIER][i] = IDENTIFIER;
        transition_matrix[EQUAL][i] = ACCEPT;
    }
    
    transition_matrix[IDENTIFIER][' '] = ACCEPT;
    transition_matrix[IDENTIFIER]['='] = ACCEPT;

    // Define transitions for separators (; , ( ) ) from all valid states
    char separators[] = {';', ',', '(', ')', '{', '}'};
    for (int i = 0; i < 6; i++) 
    {
        transition_matrix[START][separators[i]] = SEPARATOR;
        transition_matrix[NUMBER][separators[i]] = ACCEPT;
        transition_matrix[IDENTIFIER][separators[i]] = ACCEPT;
        transition_matrix[SEPARATOR][separators[i]] = ACCEPT; 
    }
    transition_matrix[SEPARATOR][' '] = ACCEPT;
    transition_matrix[SEPARATOR]['\n'] = ACCEPT;
    transition_matrix[SEPARATOR]['\t'] = ACCEPT;

    // Define transitions for operators (+ - * /) from all valid states
    char operators[] = {'+', '-', '*', '/', '%'};
    for (int i = 0; i < 5; i++) 
    {
        char op = operators[i];
        transition_matrix[START][op] = OPERATOR;
        transition_matrix[NUMBER][op] = ACCEPT;
        transition_matrix[IDENTIFIER][op] = ACCEPT;
    }

    //Define transition for equal sign
    transition_matrix[START]['='] = EQUAL;
    transition_matrix[EQUAL][' '] = ACCEPT;

}

Token *lexer(FILE *file) 
{
    initialize_transition_matrix();

    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *input = malloc(length + 1);
    if (!input) 
    {
        fclose(file);
        printf("Memory allocation failed!\n");
        return NULL;
    }

    fread(input, 1, length, file);
    fclose(file);
    input[length] = '\0';

    State current_state = START;
    int current_index = 0;
    char buffer[256];
    int buffer_index = 0;
    Token *tokens = malloc(sizeof(Token) * 100);
    int token_index = 0;
    TokenType types_arr[8] = { END_OF_TOKENS, NUMBER_TOKEN, IDENTIFIER_TOKEN, 
                                SEPARATOR_TOKEN, OPERATOR_TOKEN, 
                                EQUAL_TOKEN, END_OF_TOKENS, END_OF_TOKENS };
    while (input[current_index] != '\0') 
    {
        char current_char = input[current_index];
        State next_state = transition_matrix[current_state][(unsigned char)current_char];

        if (next_state == ACCEPT) //add token
        {
            buffer[buffer_index] = '\0';
            Token token;
            token.line_num = line_number;
            token.value = strdup(buffer);
            token.type = types_arr[current_state];
            if (token.type == IDENTIFIER_TOKEN && 
                (strcmp(buffer, "int") == 0 || strcmp(buffer, "exit") == 0 || 
                strcmp(buffer, "luloop") == 0 || strcmp(buffer, "notequal") == 0 || 
                strcmp(buffer, "equal") == 0))
            {
                token.type = KEYWORD_TOKEN;
            }
            

            tokens[token_index] = token;
            token_index++;
            buffer_index = 0;
            current_state = START;
            continue;
        }
        else if (next_state == ERROR) //unrecognized token type
        {
            buffer_index = 0;
            current_state = START;
        }
        else //didn't find a token
        {
            buffer[buffer_index++] = current_char;
            current_state = next_state;
        }
        if (current_char == '\n') 
        {
            line_number++;
        }
        current_index++;
    }


    //Add END_OF_TOKENS marker
    tokens[token_index].type = END_OF_TOKENS;
    tokens[token_index].value = NULL;
    tokens[token_index].line_num = line_number;

    free(input);

    return tokens;
}

void print_token(Token token) 
{
    char* token_types[] = {"NUMBER", "KEYWORD", "IDENTIFIER", 
                            "SEPARATOR", "OPERATOR", "EQUAL", "END_OF_TOKENS"};
    printf("TOKEN VALUE: '%s', LINE: %d, TYPE: %s\n"
            ,token.value, token.line_num, token_types[token.type]);
}

void free_tokens(Token *tokens) 
{
    for (int i = 0; tokens[i].type != END_OF_TOKENS; i++) 
    {
        free(tokens[i].value);
    }
    free(tokens);
}

int main() 
{
    FILE *file = fopen("test.lx", "r");
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