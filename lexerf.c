#include "lexerf.h"

// Global Variables
State transition_matrix[STATES_NUM][ASCI_CHARS];
int line_number = 1;

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
    transition_matrix[NUMBER]['+'] = ACCEPT;
    transition_matrix[NUMBER]['-'] = ACCEPT;
    transition_matrix[NUMBER]['*'] = ACCEPT;
    transition_matrix[NUMBER]['/'] = ACCEPT;
    transition_matrix[NUMBER]['%'] = ACCEPT;
    transition_matrix[NUMBER]['>'] = ACCEPT;
    transition_matrix[NUMBER]['<'] = ACCEPT;
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
    transition_matrix[DECIMAL_NUMBER]['+'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['-'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['*'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['/'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['%'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['>'] = ACCEPT;
    transition_matrix[DECIMAL_NUMBER]['<'] = ACCEPT;
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
        if (i != 'u') { // Skip 'o' which is handled by NOT_O
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

    for (int i = 'a'; i <= 'z'; i++) {
        if (i != 'o') { // Skip 'o' which is handled by double_O
            transition_matrix[DOUBLE_D][i] = IDENTIFIER;
        }
        transition_matrix[DOUBLE_E][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[DOUBLE_D][i] = IDENTIFIER;
        transition_matrix[DOUBLE_E][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[DOUBLE_D][i] = IDENTIFIER;
        transition_matrix[DOUBLE_E][i] = IDENTIFIER;
    }

    transition_matrix[DOUBLE_D][' '] = ACCEPT;
    transition_matrix[DOUBLE_D][']'] = ACCEPT;
    transition_matrix[DOUBLE_D]['('] = ACCEPT;
    transition_matrix[DOUBLE_D][')'] = ACCEPT;

    // String literals
    transition_matrix[START]['"'] = STRING_LITERAL;
    
    for (int i = 32; i < 127; i++) {
        if (i != '"') 
        {
            transition_matrix[STRING_LITERAL][i] = STRING_LITERAL;
        }
    }
    
    transition_matrix[STRING_LITERAL]['"'] = STRING_END;
    transition_matrix[STRING_END][' '] = ACCEPT;
    transition_matrix[STRING_END]['\n'] = ACCEPT;
    transition_matrix[STRING_END]['\t'] = ACCEPT;
    transition_matrix[STRING_END][';'] = ACCEPT;
    transition_matrix[STRING_END][','] = ACCEPT;
    transition_matrix[STRING_END][')'] = ACCEPT;
    transition_matrix[STRING_END][']'] = ACCEPT;
    
    // String errors
    transition_matrix[STRING_LITERAL]['\0'] = STRING_ERROR;
    transition_matrix[STRING_END]['"'] = STRING_ERROR;
    transition_matrix[IDENTIFIER]['"'] = STRING_ERROR;
    transition_matrix[NUMBER]['"'] = STRING_ERROR;
    transition_matrix[EQUAL]['"'] = STRING_ERROR;
    transition_matrix[OPERATOR]['"'] = STRING_ERROR;
    transition_matrix[STRING_LITERAL]['\0'] = STRING_ERROR;
    transition_matrix[STRING_LITERAL][';'] = STRING_ERROR;


    // Void keyword
    transition_matrix[START]['v'] = VOID_V;
    transition_matrix[VOID_V]['o'] = VOID_O;
    transition_matrix[VOID_O]['i'] = VOID_I;
    transition_matrix[VOID_I]['d'] = VOID_D;
    transition_matrix[VOID_D][' '] = ACCEPT;
    transition_matrix[VOID_D]['\n'] = ACCEPT;
    transition_matrix[VOID_D]['\t'] = ACCEPT;

    for (int i = 'a'; i <= 'z'; i++) {
        if (i != 'o') { //  Skip 'v' which is handled by VOID_O
            transition_matrix[VOID_V][i] = IDENTIFIER;
        }
        transition_matrix[VOID_D][i] = IDENTIFIER;
    }
    
    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[VOID_V][i] = IDENTIFIER;
        transition_matrix[VOID_D][i] = IDENTIFIER;
    }
    
    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[VOID_V][i] = IDENTIFIER;
        transition_matrix[VOID_D][i] = IDENTIFIER;
    }

    transition_matrix[VOID_V][' '] = ACCEPT;
    transition_matrix[VOID_V][']'] = ACCEPT;
    transition_matrix[VOID_V]['('] = ACCEPT;
    transition_matrix[VOID_V][')'] = ACCEPT;

    // Return keyword
    transition_matrix[START]['r'] = RETURN_R;
    transition_matrix[RETURN_R]['e'] = RETURN_E;
    transition_matrix[RETURN_E]['t'] = RETURN_T;
    transition_matrix[RETURN_T]['u'] = RETURN_U;
    transition_matrix[RETURN_U]['r'] = RETURN_R2;
    transition_matrix[RETURN_R2]['n'] = RETURN_N;
    transition_matrix[RETURN_N][' '] = ACCEPT;
    transition_matrix[RETURN_N][';'] = ACCEPT;

    for (int i = 'a'; i <= 'z'; i++) {
        if (i != 'r') { //  Skip 'r' which is handled by RETURN_R
            transition_matrix[RETURN_R][i] = IDENTIFIER;
        }
        transition_matrix[RETURN_N][i] = IDENTIFIER;
    }

    for (int i = 'A'; i <= 'Z'; i++) 
    {
        transition_matrix[RETURN_R][i] = IDENTIFIER;
        transition_matrix[RETURN_N][i] = IDENTIFIER;
    }

    for (int i = '0'; i <= '9'; i++) 
    {
        transition_matrix[RETURN_R][i] = IDENTIFIER;
        transition_matrix[RETURN_N][i] = IDENTIFIER;
    }

    transition_matrix[RETURN_R][' '] = ACCEPT;
    transition_matrix[RETURN_R][']'] = ACCEPT;
    transition_matrix[RETURN_R]['('] = ACCEPT;
    transition_matrix[RETURN_R][')'] = ACCEPT;

    // Invalid character array to mark invalid characters for all states
    int invalid_chars[256] = {0};

    // Mark valid characters for specific token types, leave invalid characters as 1
    // Identifiers (letters, digits, underscore)
    for (int i = 'a'; i <= 'z'; i++) invalid_chars[i] = 0;
    for (int i = 'A'; i <= 'Z'; i++) invalid_chars[i] = 0;
    invalid_chars['_'] = 0;  // underscore is valid in identifiers
    for (int i = '0'; i <= '9'; i++) invalid_chars[i] = 0;

    // Numbers (digits and decimal point)
    for (int i = '0'; i <= '9'; i++) invalid_chars[i] = 0;
    invalid_chars['.'] = 0;  // valid for numbers

    // Operators (arithmetic and comparison)
    invalid_chars['+'] = 0;
    invalid_chars['-'] = 0;
    invalid_chars['*'] = 0;
    invalid_chars['/'] = 0;
    invalid_chars['%'] = 0;
    invalid_chars['>'] = 0;
    invalid_chars['<'] = 0;
    invalid_chars['='] = 0;

    // Separators (like parentheses, braces, commas, semicolons)
    invalid_chars[';'] = 0;
    invalid_chars[','] = 0;
    invalid_chars['('] = 0;
    invalid_chars[')'] = 0;
    invalid_chars['{'] = 0;
    invalid_chars['}'] = 0;
    invalid_chars['['] = 0;
    invalid_chars[']'] = 0;

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
            transition_matrix[STRING_LITERAL][i] = ERROR; // Prevent invalid characters in strings
        }
    }

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
    types_arr[NOT_N] = IDENTIFIER;
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
    types_arr[LULOAD_L] = END_OF_TOKENS;
    types_arr[LULOAD_O] = END_OF_TOKENS;
    types_arr[LULOAD_A] = END_OF_TOKENS;
    types_arr[LULOAD_D] = KEYWORD_TOKEN;

    types_arr[VOID_V] = IDENTIFIER;
    types_arr[VOID_O] = END_OF_TOKENS;
    types_arr[VOID_I] = END_OF_TOKENS;
    types_arr[VOID_D] = TYPE_TOKEN;

    // Return keyword
    types_arr[RETURN_R] = IDENTIFIER;
    types_arr[RETURN_E] = END_OF_TOKENS;
    types_arr[RETURN_T] = END_OF_TOKENS;
    types_arr[RETURN_U] = END_OF_TOKENS;
    types_arr[RETURN_R2] = END_OF_TOKENS;
    types_arr[RETURN_N] = KEYWORD_TOKEN;

    return types_arr[state];
}

// Function to handle accepting state actions
void accept(char *buffer, int *buffer_index, Token *tokens, int *token_index, 
    int *line_number, State *current_state, State *next_state, char *current_char, int *current_index, int *flag) {
        buffer[*buffer_index] = '\0';
        Token token;
        token.line_num = *line_number;
        token.value = strdup(buffer);
        token.type = getType(*current_state);
        
        tokens[*token_index] = token;
        (*token_index)++;
        (*buffer_index) = 0;
        *current_state = START;
}

// Function to handle error state actions
void error(char *buffer, int *buffer_index, Token *tokens, int *token_index, 
    int *line_number, State *current_state, State *next_state, char *current_char, int *current_index, int *flag) {
    // Print the lexical error
    if(*current_char != '\n' && *current_char != '\t' && *current_char != ' ') 
    {
        printf("Lexical error at line %d \n", *line_number);
        *flag = 1;
    }
    
    // Reset for the next token
    *buffer_index = 0;
    *current_state = START;
    (*current_index)++;

    if(*current_char == '\n') 
    {
        (*line_number)++;
    }
}

void continueForAccept(char *buffer, int *buffer_index, Token *tokens, int *token_index, 
    int *line_number, State *current_state, State *next_state, char *current_char, int *current_index, int *flag) {
        buffer[*buffer_index] = *current_char;
        (*buffer_index)++;
        *current_state = *next_state;
        (*current_index)++;

        if(*current_char == '\n') 
        {
            (*line_number)++;
        }
}
Token *lexer(FILE *file, int* flag) 
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
    Token *tokens = malloc(sizeof(Token) * 1000);
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
    
    while(input[current_index] != '\0') 
    {
        char current_char = input[current_index];
        State next_state = transition_matrix[current_state][(unsigned char)current_char];
        
        arActions[next_state](buffer, &buffer_index, tokens, &token_index, &line_number, 
            &current_state, &next_state, &current_char, &current_index, flag);
    }

    tokens[token_index].type = END_OF_TOKENS;
    tokens[token_index].value = NULL;
    tokens[token_index].line_num = line_number;

    free(input);
    return tokens;
}

void print_token(Token token) 
{
    char* token_types[] = {
        "NUMBER", "KEYWORD", "TYPE", "STRING_LITERAL", "STRING_ERROR",
        "IDENTIFIER", "SEPARATOR", "OPERATOR", "EQUAL", "LOGICAL_OP", 
        "ARRAY", "COMMENT", "END_OF_TOKENS"
    };
    
    if (token.type >= 0 && token.type <= END_OF_TOKENS) 
    {
        printf("TOKEN VALUE: '%s', LINE: %d, TYPE: %s\n",
    token.value, token.line_num, token_types[token.type]);
    } 
    else 
    {
        printf("TOKEN VALUE: '%s', LINE: %d, TYPE: UNKNOWN\n",
        token.value, token.line_num);
    }
}

void free_tokens(Token *tokens) 
{
    for (int i = 0; tokens[i].type != END_OF_TOKENS; i++) 
    {
        free(tokens[i].value);
    }
    free(tokens);
}

