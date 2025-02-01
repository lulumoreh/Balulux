#include "lexerf.h"
// Global Variables
State transition_matrix[STATES_NUM][ASCI_CHARS];
int line_number = 1;

// Functions
void initialize_transition_matrix() 
{
    for (int i = 0; i < STATES_NUM; i++) 
    {
        for (int j = 0; j < ASCI_CHARS; j++) 
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
        transition_matrix[SEPARATOR][i] = ACCEPT; 
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

    //Define transition for keywords
    //int
    transition_matrix[START]['i'] = INT_I;
    transition_matrix[INT_I]['n'] = INT_N;
    transition_matrix[INT_N]['t'] = INT_T;
    transition_matrix[INT_T][' '] = ACCEPT;
    transition_matrix[INT_T]['\n'] = ACCEPT;
    transition_matrix[INT_T]['\t'] = ACCEPT;
    transition_matrix[INT_T]['('] = ACCEPT;
    transition_matrix[INT_T][')'] = ACCEPT;
    transition_matrix[INT_T]['{'] = ACCEPT;
    transition_matrix[INT_T]['}'] = ACCEPT;
    transition_matrix[INT_T][';'] = ACCEPT;
    transition_matrix[INT_T][','] = ACCEPT;
    //lulog
    transition_matrix[START]['l'] = LULOG_L;
    transition_matrix[LULOG_L]['u'] = LULOG_U;
    transition_matrix[LULOG_U]['l'] = LULOG_L2;
    transition_matrix[LULOG_L2]['o'] = LULOG_O;
    transition_matrix[LULOG_O]['g'] = LULOG_G;
    transition_matrix[LULOG_G][' '] = ACCEPT;
    transition_matrix[LULOG_G]['\n'] = ACCEPT;
    transition_matrix[LULOG_G]['\t'] = ACCEPT;
    transition_matrix[LULOG_G]['('] = ACCEPT;
    transition_matrix[LULOG_G][')'] = ACCEPT;
    transition_matrix[LULOG_G]['{'] = ACCEPT;
    transition_matrix[LULOG_G]['}'] = ACCEPT;
    transition_matrix[LULOG_G][';'] = ACCEPT;
    transition_matrix[LULOG_G][','] = ACCEPT;


    transition_matrix[KEYWORD][' '] = ACCEPT;
    transition_matrix[KEYWORD]['\n'] = ACCEPT;
    transition_matrix[KEYWORD]['\t'] = ACCEPT;
    
    transition_matrix[KEYWORD][';'] = ACCEPT;
    transition_matrix[KEYWORD][','] = ACCEPT;
    transition_matrix[KEYWORD]['('] = ACCEPT;
    transition_matrix[KEYWORD][')'] = ACCEPT;
    transition_matrix[KEYWORD]['{'] = ACCEPT;
    transition_matrix[KEYWORD]['}'] = ACCEPT;
    transition_matrix[START]['('] = SEPARATOR;
    transition_matrix[START][')'] = SEPARATOR;
    transition_matrix[START]['{'] = SEPARATOR;
    transition_matrix[START]['}'] = SEPARATOR;

    transition_matrix[IDENTIFIER]['('] = ACCEPT; // Allow function declaration
    transition_matrix[KEYWORD]['('] = ACCEPT;


    transition_matrix[KEYWORD]['='] = ACCEPT;
    transition_matrix[KEYWORD]['+'] = ACCEPT;
    transition_matrix[KEYWORD]['-'] = ACCEPT;
    transition_matrix[KEYWORD]['*'] = ACCEPT;
    transition_matrix[KEYWORD]['/'] = ACCEPT;
    transition_matrix[KEYWORD]['%'] = ACCEPT;

    // add_keyword_transitions("int");
    // add_keyword_transitions("string");
    // add_keyword_transitions("exit");
    // add_keyword_transitions("no_return");
    // add_keyword_transitions("luloop");
    // add_keyword_transitions("lulog");
    // add_keyword_transitions("notequal");
    // add_keyword_transitions("equal");

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

    size_t bytes_read = fread(input, 1, length, file);
    fclose(file);
    input[bytes_read] = '\0';

    State current_state = START;
    int current_index = 0;
    char buffer[256];
    int buffer_index = 0;
    Token *tokens = malloc(sizeof(Token) * 100);
    int token_index = 0;
    TokenType types_arr[] = {
    END_OF_TOKENS,    // START
    NUMBER_TOKEN,     // NUMBER
    IDENTIFIER_TOKEN, // IDENTIFIER
    SEPARATOR_TOKEN,  // SEPARATOR
    OPERATOR_TOKEN,   // OPERATOR
    EQUAL_TOKEN,      // EQUAL
    KEYWORD_TOKEN,    // KEYWORD
    END_OF_TOKENS,    // ACCEPT
    END_OF_TOKENS,    // ERROR
    KEYWORD_TOKEN,    // INT_I
    KEYWORD_TOKEN,    // INT_N
    KEYWORD_TOKEN,    // INT_T
    KEYWORD_TOKEN,    // LULOG_L
    KEYWORD_TOKEN,    // LULOG_U
    KEYWORD_TOKEN,    // LULOG_L2
    KEYWORD_TOKEN,    // LULOG_O
    KEYWORD_TOKEN,    // LULOG_G
};

    while(UNTIL_BREAK) 
    {
        char current_char = input[current_index];
        State next_state = transition_matrix[current_state][(unsigned char)current_char];

        if (next_state == ACCEPT || (current_char == '\0' && buffer_index > 0)) 
        {
            buffer[buffer_index] = '\0';
            Token token;
            token.line_num = line_number;
            token.value = strdup(buffer);
            token.type = types_arr[current_state];
            
            tokens[token_index] = token;
            token_index++;
            buffer_index = 0;
            current_state = START;
            
            if (current_char == '\0') break;
            continue;
        }
        else if (next_state == ERROR) //found an error state
        {
            buffer_index = 0;
            current_state = START;
        }
        else // continue searching for accept
        {
            buffer[buffer_index] = current_char;
            buffer_index++;
            current_state = next_state;
        }

        if (current_char == '\n') 
        {
            line_number++;
        }
        
        if (current_char == '\0') break;
        current_index++;
    }

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