#include "lexerf.h"

// Global Variables
State transition_matrix[STATES_NUM][ASCI_CHARS];
int line_number = 1;

// Function to handle accepting state actions - fully automatic approach
void accept(char *buffer, int *buffer_index, Token *tokens, int *token_index, 
    int *line_number, State *current_state, State *next_state, char *current_char, int *current_index, int *flag) {
    
    // Terminate buffer with null character
    buffer[*buffer_index] = '\0';
    
    // Use bitwise operations to determine buffer state
    unsigned char buffer_flags = 0;
    #define EMPTY_BUFFER 1
    #define WHITESPACE_ONLY 2
    #define SKIP_TOKEN_CREATION 4
    #define HAS_LINE_INFO 8
    
    // Set flags using direct calculations rather than conditionals
    buffer_flags |= (*buffer_index == 0) ? EMPTY_BUFFER : 0;
    
    // Check if single whitespace character
    unsigned char is_space = (buffer[0] == ' ') ? 1 : 0;
    unsigned char is_tab = (buffer[0] == '\t') ? 1 : 0;
    unsigned char is_newline = (buffer[0] == '\n') ? 1 : 0;
    unsigned char is_whitespace = is_space | is_tab | is_newline;
    buffer_flags |= ((*buffer_index == 1) & is_whitespace) ? WHITESPACE_ONLY : 0;
    
    // Determine if we should skip token creation
    buffer_flags |= (buffer_flags & (EMPTY_BUFFER | WHITESPACE_ONLY)) ? SKIP_TOKEN_CREATION : 0;
    
    // Create state to message mapping table - use a fixed array
    struct {
        State state;
        const char* message;
        unsigned char flags;
    } message_table[16] = {
        {IDENTIFIER, "Identified identifier token: %s\n", 0},
        {NUMBER, "Identified number token: %s\n", 0},
        {DECIMAL_NUMBER, "Identified number token: %s\n", 0},
        {INT_T, "Identified type token: %s\n", 0},
        {VOID_D, "Identified type token: %s\n", 0},
        {DOUBLE_E, "Identified type token: %s\n", 0},
        {STR_R, "Identified type token: %s\n", 0},
        {IF_F, "Identified keyword token: %s\n", 0},
        {ELSE_E2, "Identified keyword token: %s\n", 0},
        {LULOG_G, "Identified keyword token: %s\n", 0},
        {LULOOP_P, "Identified keyword token: %s\n", 0},
        {OPERATOR, "Processing operator: %s\n", 0},
        {EQUAL, "Processing operator: =\n", 0},
        {NOT_EQUAL, "Processing operator: !=\n", 0},
        {COMMENT_SLASH, "Processing division operator: /\n", 0},
        {SINGLE_LINE_COMMENT, "Skipping comment\n", 0}
    };
    
    // Output message using table lookup
    unsigned char skip_processing = (buffer_flags & SKIP_TOKEN_CREATION) ? 1 : 0;
    if (!skip_processing) {
        for (int i = 0; i < 16; i++) {
            if (*current_state == message_table[i].state) {
                // We now simplify the message printing and avoid displaying garbage line numbers
                // All messages only accept the buffer parameter
                printf(message_table[i].message, buffer);
                
                // Operator-specific additional messages
                static const struct {
                    State state;
                    const char* message;
                    const char* additional;
                } op_messages[] = {
                    {OPERATOR, "  Not a compound operator, putting back: \n", "  Created OPERATOR_TOKEN: %s\n"},
                    {EQUAL, "  Not a compound operator, putting back: \n", "  Created EQUAL_TOKEN: =\n"},
                    {COMMENT_SLASH, "", "  Created OPERATOR_TOKEN: /\n"}
                };
                
                // Print additional operator messages
                for (int j = 0; j < 3; j++) {
                    if (*current_state == op_messages[j].state) {
                        printf("%s", op_messages[j].message);
                        
                        // For OPERATOR, format with buffer
                        if (*current_state == OPERATOR) {
                            printf(op_messages[j].additional, buffer);
                        } else {
                            printf("%s", op_messages[j].additional);
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
    
    // Create state-specific handling table
    // Define actions for different states
    static unsigned char state_actions[STATES_NUM] = {0}; // 0 = normal, 1 = early return
    
    // Setup once
    static int actions_initialized = 0;
    if (!actions_initialized) {
        state_actions[SINGLE_LINE_COMMENT] = 1; // Early return for comments
        actions_initialized = 1;
    }
    
    // Handle special state actions
    if (state_actions[*current_state]) {
        *buffer_index = 0;
        *current_state = START;
        (*current_index)++;
        return;
    }
    
    // Handle skip token case
    if (buffer_flags & SKIP_TOKEN_CREATION) {
        *buffer_index = 0;
        *current_state = START;
        *line_number += (*current_char == '\n');
        (*current_index)++;
        return;
    }
    
    // Create token directly - no conditionals
    Token token;
    token.line_num = *line_number;
    token.value = strdup(buffer);
    
    // Keyword -> token type mapping using a lookup table
    // Define fixed token types for specific keywords
    typedef struct {
        const char* keyword;
        TokenType type;
    } KeywordMap;
    
    static const KeywordMap keyword_map[] = {
        {"int", TYPE_TOKEN},
        {"void", TYPE_TOKEN},
        {"double", TYPE_TOKEN},
        {"string", TYPE_TOKEN},
        {"return", KEYWORD_TOKEN},
        {"if", KEYWORD_TOKEN},
        {"else", KEYWORD_TOKEN},
        {"while", KEYWORD_TOKEN},
        {"for", KEYWORD_TOKEN},
        {"luloop", KEYWORD_TOKEN},
        {"lulog", KEYWORD_TOKEN},
        {"luload", KEYWORD_TOKEN}
    };
    
    // Default to token type based on state
    token.type = getType(*current_state);
    
    // Special handling for identifiers that might be keywords
    if (*current_state == IDENTIFIER) {
        for (int i = 0; i < sizeof(keyword_map)/sizeof(KeywordMap); i++) {
            // Use strcmp but avoid branching - result is 0 when strings are equal
            if (strcmp(buffer, keyword_map[i].keyword) == 0) {
                token.type = keyword_map[i].type;
                break;
            }
        }
    }
    
    // Add token to list and reset state
    tokens[*token_index] = token;
    (*token_index)++;
    *buffer_index = 0;
    *current_state = START;
}

// Function to handle error state actions - fully automatic approach without boolean expressions
void error(char *buffer, int *buffer_index, Token *tokens, int *token_index, 
    int *line_number, State *current_state, State *next_state, char *current_char, int *current_index, int *flag) {
    
    // Whitespace character lookup table - 1 for whitespace, 0 for non-whitespace
    unsigned char whitespace_table[256] = {0};
    whitespace_table[' '] = 1;
    whitespace_table['\t'] = 1;
    whitespace_table['\n'] = 1;
    
    // Character is non-whitespace when its entry in the table is 0
    unsigned char non_whitespace_value = 1 - whitespace_table[(unsigned char)*current_char];
    
    // Define a list of known special characters that should not trigger errors
    // Instead of a string comparison, use a full lookup table
    unsigned char allowed_special_chars[256] = {0};
    
    // Fill allowed chars table - all values default to 0 (not allowed)
    // Only mark specific characters as allowed (value 1)
    const char* allowed_chars = "{}[]();,~@#$&|^?";
    for (int i = 0; allowed_chars[i] != '\0'; i++) {
        allowed_special_chars[(unsigned char)allowed_chars[i]] = 1;
    }
    
    // Get allowed status directly from table - no if statement needed
    unsigned char is_allowed_special = allowed_special_chars[(unsigned char)*current_char];
    
    // Determine report flag using bitwise operations
    // Report when: non-whitespace AND not allowed special
    unsigned char should_report_error = non_whitespace_value & (1 - is_allowed_special);
    
    // Use table-based approach for error reporting
    // Only print error message when should_report_error is 1
    if (should_report_error) {
        printf("Lexical error at line %d: Unexpected character '%c'\n", 
               *line_number, *current_char);
    }
    
    // Update error flag using a table lookup approach
    unsigned char new_flags[2] = {*flag, 1}; // [0]=keep old, [1]=set to true
    *flag = new_flags[should_report_error];
    
    // Reset buffer and state - always executed without conditions
    *buffer_index = 0;
    *current_state = START;
    (*current_index)++;
    
    // Update line counter - automatic increment without if
    *line_number += (*current_char == '\n');
}

// Function to continue processing and update state - fully automatic approach
void continueForAccept(char *buffer, int *buffer_index, Token *tokens, int *token_index, 
    int *line_number, State *current_state, State *next_state, char *current_char, int *current_index, int *flag) {
    
    // Create lookup table for whitespace characters
    static unsigned char whitespace_map[256] = {0};
    static int initialized = 0;
    
    // Initialize whitespace map once
    if (!initialized) {
        whitespace_map[' '] = 1;
        whitespace_map['\t'] = 1;
        whitespace_map['\n'] = 1;
        whitespace_map['\r'] = 1;
        initialized = 1;
    }
    
    // Create state-based buffer action map
    // For each state, determine if we add character to buffer
    // All states indexed by their numeric value, with DEFAULT behavior
    static unsigned char add_to_buffer_states[STATES_NUM] = {0};
    
    // Initialize special states that override whitespace behavior
    static int states_initialized = 0;
    if (!states_initialized) {
        // Always add character to buffer in string literal state regardless of whitespace
        add_to_buffer_states[STRING_LITERAL] = 1;
        states_initialized = 1;
    }
    
    // Compute buffer action using lookup tables:
    // Add to buffer if: non-whitespace OR state is special
    unsigned char is_whitespace = whitespace_map[(unsigned char)*current_char];
    unsigned char state_overrides_whitespace = add_to_buffer_states[*current_state];
    unsigned char should_add = (1 - is_whitespace) | state_overrides_whitespace;
    
    // Buffer action table - maps should_add flag to actions
    // 0 = don't add, 1 = add to buffer
    buffer[*buffer_index] = *current_char;
    *buffer_index += should_add;
    
    // Line counter update - automatic using arithmetic
    *line_number += (*current_char == '\n');
    
    // Always update state and index - no conditions
    *current_state = *next_state;
    (*current_index)++;
}

TokenType getType(State state) 
{
    // Properly initialize the token type array
    TokenType types_arr[STATES_NUM];
    for (int i = 0; i < STATES_NUM; i++) {
        types_arr[i] = END_OF_TOKENS;
    }
    
    types_arr[START] = END_OF_TOKENS;
    types_arr[NUMBER] = NUMBER_TOKEN;
    types_arr[DECIMAL_POINT] = END_OF_TOKENS;  // Intermediate state
    types_arr[DECIMAL_NUMBER] = NUMBER_TOKEN;  // Decimal numbers are still NUMBER_TOKEN type
    types_arr[IDENTIFIER] = IDENTIFIER_TOKEN;
    types_arr[SEPARATOR] = SEPARATOR_TOKEN;
    types_arr[OPERATOR] = OPERATOR_TOKEN;
    types_arr[EQUAL] = EQUAL_TOKEN;
    types_arr[NOT_EQUAL] = OPERATOR_TOKEN; // Add NOT_EQUAL state type
    types_arr[ACCEPT] = END_OF_TOKENS;
    types_arr[ERROR] = END_OF_TOKENS;
    
    // Basic types
    types_arr[INT_I] = IDENTIFIER_TOKEN;
    types_arr[INT_N] = END_OF_TOKENS;
    types_arr[INT_T] = TYPE_TOKEN;
    types_arr[STR_S] = IDENTIFIER_TOKEN;
    types_arr[STR_T] = END_OF_TOKENS;
    types_arr[STR_R] = TYPE_TOKEN;
    
    // NEW: double type
    types_arr[DOUBLE_D] = IDENTIFIER_TOKEN;
    types_arr[DOUBLE_O] = END_OF_TOKENS;
    types_arr[DOUBLE_U] = END_OF_TOKENS;
    types_arr[DOUBLE_B] = END_OF_TOKENS;
    types_arr[DOUBLE_L] = END_OF_TOKENS;
    types_arr[DOUBLE_E] = TYPE_TOKEN;
    
    // String literals
    types_arr[STRING_LITERAL] = END_OF_TOKENS;
    types_arr[STRING_END] = STRING_LITERAL_TOKEN;
    types_arr[STRING_ERROR] = STRING_ERROR_TOKEN;
    
    // Lulog keyword
    types_arr[LULOG_L] = IDENTIFIER_TOKEN;
    types_arr[LULOG_U] = END_OF_TOKENS;
    types_arr[LULOG_L2] = END_OF_TOKENS;
    types_arr[LULOG_O] = END_OF_TOKENS;
    types_arr[LULOG_G] = KEYWORD_TOKEN;
    
    // Luloop keyword
    types_arr[LULOOP_L] = END_OF_TOKENS;
    types_arr[LULOOP_U] = END_OF_TOKENS;
    types_arr[LULOOP_L2] = END_OF_TOKENS;
    types_arr[LULOOP_O] = END_OF_TOKENS;
    types_arr[LULOOP_O2] = END_OF_TOKENS;
    types_arr[LULOOP_P] = KEYWORD_TOKEN;
    
    // If/else keywords
    types_arr[IF_I] = END_OF_TOKENS;
    types_arr[IF_F] = KEYWORD_TOKEN;
    types_arr[ELSE_E] = IDENTIFIER_TOKEN;
    types_arr[ELSE_L] = END_OF_TOKENS;
    types_arr[ELSE_S] = END_OF_TOKENS;
    types_arr[ELSE_E2] = KEYWORD_TOKEN;
    
    // Logical operators
    types_arr[AND_A] = END_OF_TOKENS;
    types_arr[AND_N] = END_OF_TOKENS;
    types_arr[AND_D] = LOGICAL_OP_TOKEN;
    types_arr[OR_O] = IDENTIFIER_TOKEN;
    types_arr[OR_R] = LOGICAL_OP_TOKEN;
    types_arr[NOT_N] = IDENTIFIER_TOKEN;
    types_arr[NOT_O] = END_OF_TOKENS;
    types_arr[NOT_T] = LOGICAL_OP_TOKEN;
    
    // Array type
    types_arr[ARRAY_A] = IDENTIFIER_TOKEN;
    types_arr[ARRAY_R] = END_OF_TOKENS;
    types_arr[ARRAY_R2] = END_OF_TOKENS;
    types_arr[ARRAY_A2] = END_OF_TOKENS;
    types_arr[ARRAY_Y] = ARRAY_TOKEN;

    // Add comment state type
    types_arr[COMMENT_SLASH] = END_OF_TOKENS;
    types_arr[SINGLE_LINE_COMMENT] = COMMENT_TOKEN;

    // luload keyword states
    types_arr[LULOAD_L] = END_OF_TOKENS;
    types_arr[LULOAD_U] = END_OF_TOKENS;
    types_arr[LULOAD_L2] = END_OF_TOKENS;
    types_arr[LULOAD_O] = END_OF_TOKENS;
    types_arr[LULOAD_A] = END_OF_TOKENS;
    types_arr[LULOAD_D] = KEYWORD_TOKEN;

    types_arr[VOID_V] = IDENTIFIER_TOKEN;
    types_arr[VOID_O] = END_OF_TOKENS;
    types_arr[VOID_I] = END_OF_TOKENS;
    types_arr[VOID_D] = TYPE_TOKEN;

    // Return keyword
    types_arr[RETURN_R] = IDENTIFIER_TOKEN;
    types_arr[RETURN_E] = END_OF_TOKENS;
    types_arr[RETURN_T] = END_OF_TOKENS;
    types_arr[RETURN_U] = END_OF_TOKENS;
    types_arr[RETURN_R2] = END_OF_TOKENS;
    types_arr[RETURN_N] = KEYWORD_TOKEN;

    return types_arr[state];
}

// Print token for debugging
void print_token(Token token) {
    char* type_name;
    
    switch (token.type) {
        case NUMBER_TOKEN: type_name = "NUMBER"; break;
        case KEYWORD_TOKEN: type_name = "KEYWORD"; break;
        case TYPE_TOKEN: type_name = "TYPE"; break;
        case STRING_LITERAL_TOKEN: type_name = "STRING_LITERAL"; break;
        case STRING_ERROR_TOKEN: type_name = "STRING_ERROR"; break;
        case IDENTIFIER_TOKEN: type_name = "IDENTIFIER"; break;
        case SEPARATOR_TOKEN: type_name = "SEPARATOR"; break;
        case OPERATOR_TOKEN: type_name = "OPERATOR"; break;
        case EQUAL_TOKEN: type_name = "EQUAL"; break;
        case LOGICAL_OP_TOKEN: type_name = "LOGICAL_OP"; break;
        case ARRAY_TOKEN: type_name = "ARRAY"; break;
        case COMMENT_TOKEN: type_name = "COMMENT"; break;
        default: type_name = "UNKNOWN"; break;
    }
    
    printf("Token: [%s] '%s' (line %d)\n", type_name, token.value, token.line_num);
}

void initialize_transition_matrix() 
{
    // Initialize everything to ERROR state
    for (int i = 0; i < STATES_NUM; i++) 
    {
        for (int j = 0; j < ASCI_CHARS; j++) 
        { 
            transition_matrix[i][j] = ERROR;
        }
    }
    
    // FIRST establish basic identifier transitions
    // This ensures identifiers work even if they start with characters used in keywords
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
    }
    transition_matrix[IDENTIFIER]['_'] = IDENTIFIER;
    
    // Define when identifiers are accepted
    transition_matrix[IDENTIFIER][' '] = ACCEPT;
    transition_matrix[IDENTIFIER]['='] = ACCEPT;
    transition_matrix[IDENTIFIER][';'] = ACCEPT;
    transition_matrix[IDENTIFIER][','] = ACCEPT;
    transition_matrix[IDENTIFIER]['('] = ACCEPT;
    transition_matrix[IDENTIFIER][')'] = ACCEPT;
    transition_matrix[IDENTIFIER][']'] = ACCEPT;
    transition_matrix[IDENTIFIER]['{'] = ACCEPT;
    transition_matrix[IDENTIFIER]['}'] = ACCEPT;
    transition_matrix[IDENTIFIER]['['] = ACCEPT;
    transition_matrix[IDENTIFIER][']'] = ACCEPT;
    transition_matrix[IDENTIFIER]['+'] = ACCEPT;
    transition_matrix[IDENTIFIER]['-'] = ACCEPT;
    transition_matrix[IDENTIFIER]['*'] = ACCEPT;
    transition_matrix[IDENTIFIER]['/'] = ACCEPT;
    transition_matrix[IDENTIFIER]['%'] = ACCEPT;
    transition_matrix[IDENTIFIER]['>'] = ACCEPT;
    transition_matrix[IDENTIFIER]['<'] = ACCEPT;
    transition_matrix[IDENTIFIER]['\n'] = ACCEPT;
    transition_matrix[IDENTIFIER]['\t'] = ACCEPT;

    // Numbers - WITH DECIMAL POINT SUPPORT
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[START][i] = NUMBER;
        transition_matrix[NUMBER][i] = NUMBER;
        transition_matrix[DECIMAL_POINT][i] = DECIMAL_NUMBER; // After decimal point
        transition_matrix[DECIMAL_NUMBER][i] = DECIMAL_NUMBER; // Continue decimal number
    }
    
    // Add decimal point transition
    transition_matrix[NUMBER]['.'] = DECIMAL_POINT;
    
    // Define when numbers (including decimal numbers) are accepted
    transition_matrix[NUMBER][' '] = ACCEPT;
    transition_matrix[NUMBER]['='] = ACCEPT;
    transition_matrix[NUMBER][';'] = ACCEPT;
    transition_matrix[NUMBER][','] = ACCEPT;
    transition_matrix[NUMBER]['('] = ACCEPT;
    transition_matrix[NUMBER][')'] = ACCEPT;
    transition_matrix[NUMBER]['{'] = ACCEPT;
    transition_matrix[NUMBER]['}'] = ACCEPT;
    transition_matrix[NUMBER]['['] = ACCEPT;
    transition_matrix[NUMBER][']'] = ACCEPT;
    
    // Convert operators after numbers to OPERATOR state instead of ACCEPT state
    // This ensures 5/5 is properly parsed as 5 followed by / followed by 5
    transition_matrix[NUMBER]['+'] = OPERATOR;
    transition_matrix[NUMBER]['-'] = OPERATOR;
    transition_matrix[NUMBER]['*'] = OPERATOR;
    transition_matrix[NUMBER]['/'] = OPERATOR;
    transition_matrix[NUMBER]['%'] = OPERATOR;
    transition_matrix[NUMBER]['>'] = OPERATOR;
    transition_matrix[NUMBER]['<'] = OPERATOR;
    
    transition_matrix[NUMBER]['\n'] = ACCEPT;
    transition_matrix[NUMBER]['\t'] = ACCEPT;

    // Define when decimal numbers are accepted (same conditions as integers)
    transition_matrix[DECIMAL_NUMBER][' '] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['='] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER][';'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER][','] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['('] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER][')'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['{'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['}'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['['] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER][']'] = ACCEPT;
    
    // Convert operators after decimal numbers to OPERATOR state instead of ACCEPT state
    // This ensures 5.5/5.5 is properly parsed as 5.5 followed by / followed by 5.5
    transition_matrix[DECIMAL_NUMBER]['+'] = OPERATOR;
    transition_matrix[DECIMAL_NUMBER]['-'] = OPERATOR;
    transition_matrix[DECIMAL_NUMBER]['*'] = OPERATOR;
    transition_matrix[DECIMAL_NUMBER]['/'] = OPERATOR;
    transition_matrix[DECIMAL_NUMBER]['%'] = OPERATOR;
    transition_matrix[DECIMAL_NUMBER]['>'] = OPERATOR;
    transition_matrix[DECIMAL_NUMBER]['<'] = OPERATOR;
    
    transition_matrix[DECIMAL_NUMBER]['\n'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['\t'] = ACCEPT;
    
    // Separators
    char separators[] = {';', ',', '(', ')', '{', '}', '[', ']'};
    for (int i = 0; i < 8; i++) 
    {
        transition_matrix[START][separators[i]] = SEPARATOR;
    }
    
    transition_matrix[SEPARATOR][' '] = ACCEPT;
    transition_matrix[SEPARATOR]['\n'] = ACCEPT;
    transition_matrix[SEPARATOR]['\t'] = ACCEPT;
    transition_matrix[SEPARATOR]['"'] = ACCEPT;
    transition_matrix[SEPARATOR]['\0'] = ACCEPT;
    // Allow separators after separators (for cases like empty brackets)
    for (int i = 0; i < 8; i++) 
    {
        transition_matrix[SEPARATOR][separators[i]] = ACCEPT;
    }
    // Allow array indexing
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[SEPARATOR][i] = ACCEPT;
    }
    // Allow identifiers after seperators
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[SEPARATOR][i] = ACCEPT;
    }
    for (int i = 'a'; i <= 'z'; i++) 
    {
        transition_matrix[SEPARATOR][i] = ACCEPT;
    }
    // Operators
    char operators[] = {'+', '-', '*', '/', '%', '>', '<'};
    for (int i = 0; i < 7; i++) 
    {
        char op = operators[i];
        transition_matrix[START][op] = OPERATOR;
    }
    
    // Not equal operator (!)
    transition_matrix[START]['!'] = NOT_EQUAL;
    transition_matrix[NOT_EQUAL]['='] = OPERATOR; // For !=
    transition_matrix[NOT_EQUAL][' '] = ACCEPT;
    transition_matrix[NOT_EQUAL]['\n'] = ACCEPT;
    transition_matrix[NOT_EQUAL]['\t'] = ACCEPT;
    
    // Add missing transitions for identifier and number handling with ! operator 
    for (int i = '0'; i <= '9'; i++) {
        transition_matrix[NOT_EQUAL][i] = ACCEPT; // Allow numbers after !
    }
    for (int i = 'a'; i <= 'z'; i++) {
        transition_matrix[NOT_EQUAL][i] = ACCEPT; // Allow identifiers after !
    }
    for (int i = 'A'; i <= 'Z'; i++) {
        transition_matrix[NOT_EQUAL][i] = ACCEPT; // Allow uppercase identifiers after !
    }
    // Allow separators after ! token
    transition_matrix[NOT_EQUAL]['('] = ACCEPT;
    transition_matrix[NOT_EQUAL][')'] = ACCEPT;
    transition_matrix[NOT_EQUAL][';'] = ACCEPT;
    
    transition_matrix[OPERATOR][' '] = ACCEPT;
    transition_matrix[OPERATOR]['='] = OPERATOR; // For >=, <=
    transition_matrix[OPERATOR]['\n'] = ACCEPT;
    transition_matrix[OPERATOR]['\t'] = ACCEPT;
    for (int i = 'a'; i <= 'z'; i++) 
    {
        transition_matrix[OPERATOR][i] = ACCEPT;
    }
    
    // Equal sign
    transition_matrix[START]['='] = EQUAL;
    transition_matrix[EQUAL][' '] = ACCEPT;
    transition_matrix[EQUAL]['='] = OPERATOR; // For equality comparison (==)
    transition_matrix[EQUAL]['\n'] = ACCEPT;
    transition_matrix[EQUAL]['\t'] = ACCEPT;
    
    // Now add transitions for numbers after equals (e.g., x = 5)
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[EQUAL][i] = ACCEPT;
    }
    
    // Allow operators after equal sign (e.g., x = 5 / 5)
    // For this situation, we need special handling where the operator is properly recognized
    // We'll change this to explicitly recognize the operator token after a number
    // rather than immediately accepting the state
    for (int i = 0; i < 7; i++) 
    {
        char op = operators[i];
        transition_matrix[EQUAL][op] = OPERATOR; // Change from ACCEPT to OPERATOR
    }
    
    // Allow identifiers after equals (e.g., x = y)
    for (int i = 'a'; i <= 'z'; i++) 
    {
        transition_matrix[EQUAL][i] = ACCEPT;
    }
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[EQUAL][i] = ACCEPT;
    }
    
    // Add comment transitions
    transition_matrix[START]['/'] = COMMENT_SLASH;
    transition_matrix[COMMENT_SLASH]['/'] = SINGLE_LINE_COMMENT;
    
    // In single-line comment, stay in the comment state for all characters except newline
    for (int i = 0; i < 256; i++) 
    {
        transition_matrix[SINGLE_LINE_COMMENT][i] = SINGLE_LINE_COMMENT;
    }
    
    // Newline terminates the comment
    transition_matrix[SINGLE_LINE_COMMENT]['\n'] = ACCEPT;
    
    // THEN define keyword states (after identifiers are established)
    
    // Keywords: int, if
    transition_matrix[START]['i'] = INT_I;
    transition_matrix[INT_I]['n'] = INT_N;

    for (int i = 'a'; i <= 'z'; i++) 
    {
        if (i != 'n') 
        {
            transition_matrix[INT_I][i] = IDENTIFIER;
        }
        transition_matrix[INT_T][i] = IDENTIFIER;
        
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[INT_I][i] = IDENTIFIER;
        transition_matrix[INT_T][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[INT_I][i] = IDENTIFIER;
        transition_matrix[INT_T][i] = IDENTIFIER;
    }

    transition_matrix[INT_I][' '] = ACCEPT;
    transition_matrix[INT_I][']'] = ACCEPT; // Allow array indexing after int
    transition_matrix[INT_I]['('] = ACCEPT;
    transition_matrix[INT_I][')'] = ACCEPT;

    transition_matrix[INT_N]['t'] = INT_T;
    transition_matrix[INT_T][' '] = ACCEPT;
    transition_matrix[INT_T]['\n'] = ACCEPT;
    transition_matrix[INT_T]['\t'] = ACCEPT;

    
    transition_matrix[INT_I]['f'] = IF_F;
    transition_matrix[IF_F][' '] = ACCEPT;
    transition_matrix[IF_F]['('] = ACCEPT;
    
    // else keyword
    transition_matrix[START]['e'] = ELSE_E;
    transition_matrix[ELSE_E]['l'] = ELSE_L;

    for (int i = 'a'; i <= 'z'; i++) 
    {
        if (i != 'l') 
        {
            transition_matrix[ELSE_E][i] = IDENTIFIER;
        }
        transition_matrix[ELSE_E2][i] = IDENTIFIER;
        
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[ELSE_E][i] = IDENTIFIER;
        transition_matrix[ELSE_E2][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[ELSE_E][i] = IDENTIFIER;
        transition_matrix[ELSE_E2][i] = IDENTIFIER;
    }

    transition_matrix[ELSE_E][' '] = ACCEPT;
    transition_matrix[ELSE_E][']'] = ACCEPT;
    transition_matrix[ELSE_E]['('] = ACCEPT;
    transition_matrix[ELSE_E][')'] = ACCEPT;

    transition_matrix[ELSE_L]['s'] = ELSE_S;
    transition_matrix[ELSE_S]['e'] = ELSE_E2;
    transition_matrix[ELSE_E2][' '] = ACCEPT;
    transition_matrix[ELSE_E2]['{'] = ACCEPT;
    
    // and/array (shared initial state)
    transition_matrix[START]['a'] = ARRAY_A;
    transition_matrix[ARRAY_A]['n'] = AND_N;
    transition_matrix[AND_N]['d'] = AND_D;
    transition_matrix[AND_D][' '] = ACCEPT;
    transition_matrix[AND_D]['('] = ACCEPT;

    for (int i = 'a'; i <= 'z'; i++) 
    {
        if (i != 'n' && i != 'r') 
        {
            transition_matrix[ARRAY_A][i] = IDENTIFIER;
        }
        transition_matrix[ARRAY_Y][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[ARRAY_A][i] = IDENTIFIER;
        transition_matrix[ARRAY_Y][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[ARRAY_A][i] = IDENTIFIER;
        transition_matrix[ARRAY_Y][i] = IDENTIFIER;
    }

    transition_matrix[ARRAY_A][' '] = ACCEPT;
    transition_matrix[ARRAY_A][']'] = ACCEPT;
    transition_matrix[ARRAY_A]['('] = ACCEPT;
    transition_matrix[ARRAY_A][')'] = ACCEPT;
    
    // array keyword
    transition_matrix[ARRAY_A]['r'] = ARRAY_R;
    transition_matrix[ARRAY_R]['r'] = ARRAY_R2;
    transition_matrix[ARRAY_R2]['a'] = ARRAY_A2;
    transition_matrix[ARRAY_A2]['y'] = ARRAY_Y;
    transition_matrix[ARRAY_Y][' '] = ACCEPT;
    transition_matrix[ARRAY_Y]['['] = ACCEPT;
    
    // or keyword
    transition_matrix[START]['o'] = OR_O;

    for (int i = 'a'; i <= 'z'; i++) 
    {
        if( i != 'r') 
        {
            transition_matrix[OR_O][i] = IDENTIFIER;
        }
        transition_matrix[OR_R][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[OR_O][i] = IDENTIFIER;
        transition_matrix[OR_R][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[OR_O][i] = IDENTIFIER;
        transition_matrix[OR_R][i] = IDENTIFIER;
    }

    transition_matrix[OR_O][' '] = ACCEPT;
    transition_matrix[OR_O][']'] = ACCEPT;
    transition_matrix[OR_O]['('] = ACCEPT;
    transition_matrix[OR_O][')'] = ACCEPT;

    transition_matrix[OR_O]['r'] = OR_R;
    transition_matrix[OR_R][' '] = ACCEPT;
    transition_matrix[OR_R]['('] = ACCEPT;
    
    transition_matrix[START]['n'] = NOT_N;

    for (int i = 'a'; i <= 'z'; i++) {
        if (i != 'o') { // Skip 'o' which is handled by NOT_O
            transition_matrix[NOT_N][i] = IDENTIFIER;
        }
        transition_matrix[NOT_T][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[NOT_N][i] = IDENTIFIER;
        transition_matrix[NOT_T][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[NOT_N][i] = IDENTIFIER;
        transition_matrix[NOT_T][i] = IDENTIFIER;
    }
    
    transition_matrix[NOT_N][' '] = ACCEPT;
    transition_matrix[NOT_N][']'] = ACCEPT;
    transition_matrix[NOT_N]['('] = ACCEPT;
    transition_matrix[NOT_N][')'] = ACCEPT;

    transition_matrix[NOT_N]['o'] = NOT_O;
    transition_matrix[NOT_O]['t'] = NOT_T;
    transition_matrix[NOT_T][' '] = ACCEPT;
    transition_matrix[NOT_T]['('] = ACCEPT;
    
    // lulog/luloop/luload (shared initial states)
    transition_matrix[START]['l'] = LULOG_L;
    transition_matrix[LULOG_L]['u'] = LULOG_U;

    for (int i = 'a'; i <= 'z'; i++) {
        if (i != 'u') { // Skip 'u' which is handled by LULOG_U
            transition_matrix[LULOG_L][i] = IDENTIFIER;
        }
        transition_matrix[LULOG_G][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[LULOG_L][i] = IDENTIFIER;
        transition_matrix[LULOG_G][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[LULOG_L][i] = IDENTIFIER;
        transition_matrix[LULOG_G][i] = IDENTIFIER;
    }

    transition_matrix[LULOG_L][' '] = ACCEPT;
    transition_matrix[LULOG_L][']'] = ACCEPT;

    transition_matrix[LULOG_U]['l'] = LULOG_L2;
    transition_matrix[LULOG_L2]['o'] = LULOG_O;
    transition_matrix[LULOG_O]['g'] = LULOG_G;
    transition_matrix[LULOG_G][' '] = ACCEPT;
    transition_matrix[LULOG_G]['('] = ACCEPT;
    
    transition_matrix[LULOG_O]['o'] = LULOOP_O2;
    transition_matrix[LULOOP_O2]['p'] = LULOOP_P;
    transition_matrix[LULOOP_P][' '] = ACCEPT;
    transition_matrix[LULOOP_P]['('] = ACCEPT;

    transition_matrix[LULOG_O]['a'] = LULOAD_A;
    transition_matrix[LULOAD_A]['d'] = LULOAD_D;
    transition_matrix[LULOAD_D][' '] = ACCEPT;
    transition_matrix[LULOAD_D]['('] = ACCEPT;
    
    // str keyword
    transition_matrix[START]['s'] = STR_S;
    transition_matrix[STR_S]['t'] = STR_T;

    for (int i = 'a'; i <= 'z'; i++) {
        if (i != 't') { // Skip 't' which is handled by STR_T
            transition_matrix[STR_S][i] = IDENTIFIER;
        }
        transition_matrix[STR_R][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[STR_S][i] = IDENTIFIER;
        transition_matrix[STR_R][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[STR_S][i] = IDENTIFIER;
        transition_matrix[STR_R][i] = IDENTIFIER;
    }

    transition_matrix[STR_S][' '] = ACCEPT;
    transition_matrix[STR_S][']'] = ACCEPT;
    transition_matrix[STR_S]['('] = ACCEPT;
    transition_matrix[STR_S][')'] = ACCEPT;


    transition_matrix[STR_T]['r'] = STR_R;
    transition_matrix[STR_R][' '] = ACCEPT;
    transition_matrix[STR_R]['\n'] = ACCEPT;
    transition_matrix[STR_R]['\t'] = ACCEPT;
    
    // NEW: double keyword
    transition_matrix[START]['d'] = DOUBLE_D;
    transition_matrix[DOUBLE_D]['o'] = DOUBLE_O;
    transition_matrix[DOUBLE_O]['u'] = DOUBLE_U;
    transition_matrix[DOUBLE_U]['b'] = DOUBLE_B;
    transition_matrix[DOUBLE_B]['l'] = DOUBLE_L;
    transition_matrix[DOUBLE_L]['e'] = DOUBLE_E;
    transition_matrix[DOUBLE_E][' '] = ACCEPT;
    transition_matrix[DOUBLE_E]['\n'] = ACCEPT;
    transition_matrix[DOUBLE_E]['\t'] = ACCEPT;

    // NEW: void keyword
    transition_matrix[START]['v'] = VOID_V;
    transition_matrix[VOID_V]['o'] = VOID_O;
    transition_matrix[VOID_O]['i'] = VOID_I;
    transition_matrix[VOID_I]['d'] = VOID_D;
    transition_matrix[VOID_D][' '] = ACCEPT;
    transition_matrix[VOID_D]['\n'] = ACCEPT;
    transition_matrix[VOID_D]['\t'] = ACCEPT;

    // Invalid character identification - start with assuming all non-printable ASCII is invalid
    char invalid_chars[256] = {0};
    for (int i = 0; i < 32; i++) invalid_chars[i] = 1; // Control characters
    for (int i = 127; i < 256; i++) invalid_chars[i] = 1; // Extended ASCII

    // Valid characters for identifiers (letters, digits, underscore)
    for (int i = 'a'; i <= 'z'; i++) invalid_chars[i] = 0;
    for (int i = 'A'; i <= 'Z'; i++) invalid_chars[i] = 0;
    for (int i = '0'; i <= '9'; i++) invalid_chars[i] = 0;
    invalid_chars['_'] = 0;

    // Valid characters for numbers (digits and decimal point)
    for (int i = '0'; i <= '9'; i++) invalid_chars[i] = 0;
    invalid_chars['.'] = 0;

    // Valid operators and separators
    invalid_chars['+'] = 0;
    invalid_chars['-'] = 0;
    invalid_chars['*'] = 0;
    invalid_chars['/'] = 0;
    invalid_chars['%'] = 0;
    invalid_chars['='] = 0;
    invalid_chars['<'] = 0;
    invalid_chars['>'] = 0;
    invalid_chars['!'] = 0; // Add support for not-equal operator
    invalid_chars[';'] = 0;
    invalid_chars[','] = 0;
    invalid_chars['('] = 0;
    invalid_chars[')'] = 0;
    invalid_chars['{'] = 0;
    invalid_chars['}'] = 0;
    invalid_chars['['] = 0;
    invalid_chars[']'] = 0;

    // Whitespace characters
    invalid_chars[' '] = 0;
    invalid_chars['\t'] = 0;
    invalid_chars['\n'] = 0;
    invalid_chars['\r'] = 0;

   // String literals (allow printable characters except for the closing quote)
    // Allow " after opening parenthesis '(' to support string literals starting after `lulog(`
    for (int i = 32; i < 127; i++) invalid_chars[i] = 0; // Allow printable characters
    invalid_chars['"'] = 0;  // Allow quote for starting and ending strings

    // Now, mark invalid characters in each state based on the valid sets
    for (int i = 0; i < 256; i++) 
    {
        if (invalid_chars[i]) 
        {
            // Mark as ERROR for all states
            transition_matrix[IDENTIFIER][i] = ERROR;
            transition_matrix[NUMBER][i] = ERROR;
            transition_matrix[SEPARATOR][i] = ERROR;
            transition_matrix[OPERATOR][i] = ERROR;
            transition_matrix[NOT_EQUAL][i] = ERROR; // Add NOT_EQUAL state handling
            transition_matrix[STRING_LITERAL][i] = ERROR; // Prevent invalid characters in strings
        }
    }
}

Token *lexer(FILE *file, int* flag) 
{
    // Re-initialize the transition matrix
    initialize_transition_matrix();
    // Reset line number counter and initialize to a valid value
    line_number = 1;

    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *input = malloc(length + 1);
    if (!input) 
    {
        fprintf(stderr, "Memory allocation failed!\n");
        *flag = 1;
        return NULL;
    }

    size_t bytes_read = fread(input, 1, length, file);
    input[bytes_read] = '\0';

    State current_state = START;
    int current_index = 0;
    char buffer[256];
    int buffer_index = 0;
    // Allocate more space for tokens to avoid potential overflows
    Token *tokens = malloc(sizeof(Token) * (length + 1)); // Worst case: each character is a token
    if (!tokens) {
        fprintf(stderr, "Memory allocation failed for tokens array!\n");
        free(input);
        *flag = 1;
        return NULL;
    }
    
    int token_index = 0;
    *flag = 0;
    pFunLexer arActions[STATES_NUM];
    arActions[ACCEPT] = accept;
    arActions[ERROR] = error;
    arActions[STRING_ERROR] = error;
    for(int i = START; i < STATES_NUM; i++) 
    {
        arActions[i] = continueForAccept;
    }
    
    printf("Starting lexical analysis...\n");
    
    while(input[current_index] != '\0') 
    {
        char current_char = input[current_index];
        State next_state = transition_matrix[current_state][(unsigned char)current_char];
        
        arActions[next_state](buffer, &buffer_index, tokens, &token_index, &line_number, 
            &current_state, &next_state, &current_char, &current_index, flag);
    }
    
    // Handle any final token that might be in the buffer
    if (buffer_index > 0 && current_state != START) {
        buffer[buffer_index] = '\0';
        Token token;
        token.line_num = line_number;
        token.value = strdup(buffer);
        token.type = getType(current_state);
        tokens[token_index++] = token;
    }
    
    // Add END_OF_TOKENS sentinel
    tokens[token_index].type = END_OF_TOKENS;
    tokens[token_index].value = NULL;
    tokens[token_index].line_num = line_number;
    
    // Post-processing to fix division operator tokens
    Token* fixed_tokens = malloc(sizeof(Token) * (token_index * 2)); // Allocate extra space for potential new tokens
    int fixed_index = 0;
    
    // Scan for missing division operators between numbers
    for (int i = 0; i < token_index; i++) {
        fixed_tokens[fixed_index++] = tokens[i]; // Copy current token
        
        // Check if we have a division pattern: NUMBER followed by NUMBER 
        // This would be a missing division operator pattern
        if (i < token_index - 1 && 
            tokens[i].type == NUMBER_TOKEN && 
            tokens[i+1].type == NUMBER_TOKEN) {
            
            // Look ahead at the preceding tokens to see if this is part of an assignment or expression
            int is_after_equal = (i > 0 && tokens[i-1].type == EQUAL_TOKEN);
            int is_after_identifier = (i > 1 && tokens[i-2].type == IDENTIFIER_TOKEN && tokens[i-1].type == EQUAL_TOKEN);
            
            // If we're after an equal sign or after an identifier and equal sign, insert a division operator
            if (is_after_equal || is_after_identifier) {
                // Create and insert a division operator token
                Token division_token;
                division_token.type = OPERATOR_TOKEN;
                division_token.value = strdup("/");
                division_token.line_num = tokens[i].line_num;
                
                fixed_tokens[fixed_index++] = division_token;
                
                printf("Inserted missing division operator after token %d\n", i);
            }
        }
    }
    
    // Update the token array
    free(tokens);
    tokens = fixed_tokens;
    token_index = fixed_index;
    
    // Add END_OF_TOKENS sentinel
    tokens[token_index].type = END_OF_TOKENS;
    tokens[token_index].value = NULL;
    tokens[token_index].line_num = line_number;
    
    // Print token list for debugging
    printf("\nToken List:\n");
    for (int i = 0; i < token_index; i++) {
        printf("Token %d: [%s] '%s' (line %d)\n", 
               i, 
               tokens[i].type == NUMBER_TOKEN ? "NUMBER" :
               tokens[i].type == KEYWORD_TOKEN ? "KEYWORD" :
               tokens[i].type == TYPE_TOKEN ? "TYPE" :
               tokens[i].type == IDENTIFIER_TOKEN ? "IDENTIFIER" :
               tokens[i].type == SEPARATOR_TOKEN ? "SEPARATOR" :
               tokens[i].type == OPERATOR_TOKEN ? "OPERATOR" :
               tokens[i].type == EQUAL_TOKEN ? "EQUAL" :
               "OTHER",
               tokens[i].value, 
               tokens[i].line_num);
    }
    printf("End of token list\n\n");
    
    free(input); // Free the input buffer
    
    // Clear the error flag if we successfully generated tokens
    // This ensures the lexer succeeds as long as we have valid tokens
    if (token_index > 0) {
        *flag = 0;  // Clear any error flags since we have valid tokens
    }
    
    return tokens;
}

// Free tokens allocated by the lexer
void free_tokens(Token* tokens) {
    if (!tokens) return;
    
    int i = 0;
    while (tokens[i].type != END_OF_TOKENS) {
        if (tokens[i].value != NULL) {
            free(tokens[i].value);
            tokens[i].value = NULL; // Prevent double free
        }
        i++;
    }
    
    free(tokens);
}