#include "parser.h"
void init_lexeme_actions(Parser* parser) {
    parser->state_lexeme_actions = (TokenLexemeActions**)malloc(parser->num_states * sizeof(TokenLexemeActions*));
    
    if (!parser->state_lexeme_actions) {
        fprintf(stderr, "Memory allocation failed for lexeme action tables\n");
        exit(1);
    }

    for (int i = 0; i < parser->num_states; i++) {
        parser->state_lexeme_actions[i] = NULL;
    }
}

void add_lexeme_action(Parser* parser, int state, TokenType token_type, 
    const char* lexeme, ActionType action_type, int action_value) {

    if (parser->state_lexeme_actions[state] == NULL) {
        parser->state_lexeme_actions[state] = (TokenLexemeActions*)malloc(sizeof(TokenLexemeActions) * parser->num_terminals);
        
        if (parser->state_lexeme_actions[state] == NULL) {
            fprintf(stderr, "Memory allocation failed for parser->state_lexeme_actions[%d]\n", state);
            exit(1);
        }

        for (int i = 0; i < parser->num_terminals; i++) {
            parser->state_lexeme_actions[state][i].token_type = i;
            parser->state_lexeme_actions[state][i].num_actions = 0;
            parser->state_lexeme_actions[state][i].capacity = 4; // Initial capacity for actions
            parser->state_lexeme_actions[state][i].actions = (LexemeAction*)malloc(sizeof(LexemeAction) * parser->state_lexeme_actions[state][i].capacity);
            
            if (parser->state_lexeme_actions[state][i].actions == NULL) {
                fprintf(stderr, "Memory allocation failed for actions at state %d\n", state);
                exit(1);
            }
        }
    }

    TokenLexemeActions* tla = &parser->state_lexeme_actions[state][token_type];
    
    if (tla->num_actions >= tla->capacity) {
        int new_capacity = tla->capacity * 2;
        tla->actions = (LexemeAction*)realloc(tla->actions, new_capacity * sizeof(LexemeAction));
        
        if (tla->actions == NULL) {
            fprintf(stderr, "Memory reallocation failed for lexeme actions at state %d\n", state);
            exit(1);
        }

        tla->capacity = new_capacity;
    }

    LexemeAction* action = &tla->actions[tla->num_actions++];
    action->lexeme = strdup(lexeme);
    action->type = action_type;
    action->value = action_value;
}

Action get_action(Parser* parser, int state, Token token) {
    // Special case for END_OF_TOKENS to handle end of input correctly
    if (token.type == END_OF_TOKENS) {
        // Return the accept action when reaching the end of the input
        Action result;
        result.type = ACTION_ACCEPT;
        result.value = 0; // No additional value needed for accept
        return result;
    }
    
    // Check if lexeme actions exist for the current state and token type
    if (parser->state_lexeme_actions[state] != NULL) {
        TokenLexemeActions* tla = &parser->state_lexeme_actions[state][token.type];
        
        // First, check for a specific action for the exact lexeme (token value)
        for (int i = 0; i < tla->num_actions; i++) {
            // If an exact match of the lexeme is found, return the corresponding action
            if (strcmp(tla->actions[i].lexeme, token.value) == 0) {
                Action result;
                result.type = tla->actions[i].type;
                result.value = tla->actions[i].value;
                return result;
            }
        }
        
        // If no exact match is found, check for a wildcard "*" lexeme
        for (int i = 0; i < tla->num_actions; i++) {
            // If a wildcard "*" match is found, return the corresponding action
            if (strcmp(tla->actions[i].lexeme, "*") == 0) {
                Action result;
                result.type = tla->actions[i].type;
                result.value = tla->actions[i].value;
                return result;
            }
        }
    }
    
    // If no specific lexeme action is found, check the action table
    return parser->action_table[state][token.type];
}



void free_lexeme_actions(Parser* parser) {
    if (parser->state_lexeme_actions) {
        for (int i = 0; i < parser->num_states; i++) {
            if (parser->state_lexeme_actions[i]) {
                for (int j = 0; j < parser->num_terminals; j++) {
                    TokenLexemeActions* tla = &parser->state_lexeme_actions[i][j];
                    if (tla->actions) {
                        for (int k = 0; k < tla->num_actions; k++) {
                            free(tla->actions[k].lexeme);
                        }
                        free(tla->actions);
                    }
                }
                free(parser->state_lexeme_actions[i]);
            }
        }
        free(parser->state_lexeme_actions);
        parser->state_lexeme_actions = NULL;  // Set to NULL after freeing
    }
}

Parser* create_parser(Token* tokens) {
    // Allocate memory for the parser object
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) {
        fprintf(stderr, "Memory allocation failed for parser\n");
        exit(1);
    }
    
    // Initialize the parser fields
    parser->tokens = tokens;
    parser->current_token = 0;
    parser->has_error = false;
    parser->error_message = NULL;
    
    // Initialize parser settings (number of states, terminals, etc.)
    parser->num_states = 51;
    parser->num_terminals = 13;
    parser->num_non_terminals = 26;
    parser->num_rules = 40;
    
    // Initialize the stack
    init_stack(&parser->stack);

    // Initialize the lexeme actions table
    init_lexeme_actions(parser);
    
    // Initialize the parsing rules
    init_rules(parser);

    // Initialize the parsing tables (action and goto tables)
    init_parse_tables(parser);

    // Initialize the AST root to NULL
    parser->ast_root = NULL;

    // Create the initial stack item and push it onto the stack with the AST node set to NULL
    StackItem initial_item = {0, {END_OF_TOKENS, NULL, 0}, 0, false, NULL};
    push(&parser->stack, initial_item);

    return parser;
}

// Initialize the stack
void init_stack(Stack* stack) {
    stack->capacity = 100;  // Initial capacity
    stack->items = (StackItem*)malloc(stack->capacity * sizeof(StackItem));
    if (!stack->items) {
        fprintf(stderr, "Memory allocation failed for stack\n");
        exit(1);
    }
    stack->top = -1;
}

// Push an item onto the stack
void push(Stack* stack, StackItem item) {
    // Check if stack needs to be resized
    if (stack->top + 1 >= stack->capacity) {
        stack->capacity *= 2;
        stack->items = (StackItem*)realloc(stack->items, stack->capacity * sizeof(StackItem));
        if (!stack->items) {
            fprintf(stderr, "Memory reallocation failed for stack\n");
            exit(1);
        }
    }
    
    stack->items[++stack->top] = item;
}

// Pop an item from the stack
StackItem pop(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    
    return stack->items[stack->top--];
}

// Peek at the top item of the stack
StackItem peek(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Stack is empty\n");
        exit(1);
    }
    
    return stack->items[stack->top];
}

// Check if the stack is empty
bool is_stack_empty(Stack* stack) {
    return stack->top < 0;
}

// Free the stack memory
void free_stack(Stack* stack) {
    free(stack->items);
    stack->items = NULL;
    stack->capacity = 0;
    stack->top = -1;
}

void init_rules(Parser* parser) {
    int index = 0;
    parser->rules = (Rule*)malloc(parser->num_rules * sizeof(Rule));
    if (!parser->rules) {
        fprintf(stderr, "Memory allocation failed for rules\n");
        exit(1);
    }

    // Rule 0: <program> ::= <function>*
    parser->rules[index].lhs = NT_PROGRAM;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_FUNCTION;
    index++;

// Rule 1: <function> ::= <type> <identifier> "(" <parameters> ")" "{" <statements> "}"
parser->rules[index].lhs = NT_FUNCTION;
parser->rules[index].num_symbols = 8;
parser->rules[index].is_terminal[0] = false;
parser->rules[index].symbols[0] = NT_TYPE;
parser->rules[index].is_terminal[1] = true;
parser->rules[index].symbols[1] = IDENTIFIER_TOKEN;
parser->rules[index].is_terminal[2] = true;
parser->rules[index].symbols[2] = SEPARATOR_TOKEN;
parser->rules[index].is_terminal[3] = false;
parser->rules[index].symbols[3] = NT_PARAMETERS;
parser->rules[index].is_terminal[4] = true;
parser->rules[index].symbols[4] = SEPARATOR_TOKEN;
parser->rules[index].is_terminal[5] = true;
parser->rules[index].symbols[5] = SEPARATOR_TOKEN;
parser->rules[index].is_terminal[6] = false;
parser->rules[index].symbols[6] = NT_STATEMENTS;
parser->rules[index].is_terminal[7] = true;
parser->rules[index].symbols[7] = SEPARATOR_TOKEN;  // semicolon can appear after function declaration
index++;

    // Rule 2: <parameters> ::= <parameter> ("," <parameter>)*
    parser->rules[index].lhs = NT_PARAMETERS;
    parser->rules[index].num_symbols = 3;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_PARAMETER;
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[2] = false;
    parser->rules[index].symbols[2] = NT_PARAMETERS;
    index++;

    // Rule 3: <parameters> ::= <parameter>
    parser->rules[index].lhs = NT_PARAMETERS;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_PARAMETER;
    index++;

    // Rule 4: <parameters> ::= ε (empty)
    parser->rules[index].lhs = NT_PARAMETERS;
    parser->rules[index].num_symbols = 0;
    index++;

    // Rule 5: <parameter> ::= <type> <identifier>
    parser->rules[index].lhs = NT_PARAMETER;
    parser->rules[index].num_symbols = 2;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_TYPE;
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = IDENTIFIER_TOKEN;
    index++;

    // Rule 6: <statements> ::= <statement>*
    parser->rules[index].lhs = NT_STATEMENTS;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_STATEMENT;
    index++;

    // Rule 7: <statements> ::= ε (empty)
    parser->rules[index].lhs = NT_STATEMENTS;
    parser->rules[index].num_symbols = 0;
    index++;

    // Rule 8: <statement> ::= <variable_declaration>
    parser->rules[index].lhs = NT_STATEMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_VARIABLE_DECLARATION;
    index++;

    // Rule 9: <statement> ::= <assignment>
    parser->rules[index].lhs = NT_STATEMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_ASSIGNMENT;
    index++;

    // Rule 10: <statement> ::= <if_statement>
    parser->rules[index].lhs = NT_STATEMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_IF_STATEMENT;
    index++;

    // Rule 11: <statement> ::= <loop_statement>
    parser->rules[index].lhs = NT_STATEMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_LOOP_STATEMENT;
    index++;

    // Rule 12: <statement> ::= <function_call>
    parser->rules[index].lhs = NT_STATEMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_FUNCTION_CALL;
    index++;

    // Rule 13: <statement> ::= <return_statement>
    parser->rules[index].lhs = NT_STATEMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_RETURN_STATEMENT;
    index++;

    // Rule 14: <variable_declaration> ::= <type> <identifier> ("=" <expression>)? ";"
parser->rules[index].lhs = NT_VARIABLE_DECLARATION;
parser->rules[index].num_symbols = 5;
parser->rules[index].is_terminal[0] = false;
parser->rules[index].symbols[0] = NT_TYPE;
parser->rules[index].is_terminal[1] = true;
parser->rules[index].symbols[1] = IDENTIFIER_TOKEN;
parser->rules[index].is_terminal[2] = true;
parser->rules[index].symbols[2] = EQUAL_TOKEN;
parser->rules[index].is_terminal[3] = false;
parser->rules[index].symbols[3] = NT_EXPRESSION;
parser->rules[index].is_terminal[4] = true;
parser->rules[index].symbols[4] = SEPARATOR_TOKEN;  // semicolon expected at the end
index++;

    // Rule 15: <variable_declaration> ::= <type> <identifier> ";"
    parser->rules[index].lhs = NT_VARIABLE_DECLARATION;
    parser->rules[index].num_symbols = 3;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_TYPE;
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = IDENTIFIER_TOKEN;
    parser->rules[index].is_terminal[2] = true;
    parser->rules[index].symbols[2] = SEPARATOR_TOKEN;
    index++;

    // Rule 16: <assignment> ::= <identifier> "=" <expression> ";"
    parser->rules[index].lhs = NT_ASSIGNMENT;
    parser->rules[index].num_symbols = 4;
    parser->rules[index].is_terminal[0] = true;  // IDENTIFIER_TOKEN
    parser->rules[index].symbols[0] = IDENTIFIER_TOKEN;
    parser->rules[index].is_terminal[1] = true;  // EQUAL_TOKEN
    parser->rules[index].symbols[1] = EQUAL_TOKEN;
    parser->rules[index].is_terminal[2] = false; // NT_EXPRESSION
    parser->rules[index].symbols[2] = NT_EXPRESSION;
    parser->rules[index].is_terminal[3] = true;  // SEPARATOR_TOKEN (semicolon)
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;
    index++;

    // Rule 17: <if_statement> ::= "if" "(" <expression> ")"
    parser->rules[index].lhs = NT_IF_STATEMENT;
    parser->rules[index].num_symbols = 4;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = KEYWORD_TOKEN;
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[2] = false;
    parser->rules[index].symbols[2] = NT_EXPRESSION;
    parser->rules[index].is_terminal[3] = true;
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;
    index++;

    // Rule 18: <if_statement> ::= "if" "(" <expression> ")" "{" <statements> "}" "else" "{" <statements> "}"
    parser->rules[index].lhs = NT_IF_STATEMENT;
    parser->rules[index].num_symbols = 11;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = KEYWORD_TOKEN;
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[2] = false;
    parser->rules[index].symbols[2] = NT_EXPRESSION;
    parser->rules[index].is_terminal[3] = true;
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[4] = true;
    parser->rules[index].symbols[4] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[5] = false;
    parser->rules[index].symbols[5] = NT_STATEMENTS;
    parser->rules[index].is_terminal[6] = true;
    parser->rules[index].symbols[6] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[7] = true;
    parser->rules[index].symbols[7] = KEYWORD_TOKEN;
    parser->rules[index].is_terminal[8] = true;
    parser->rules[index].symbols[8] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[9] = false;
    parser->rules[index].symbols[9] = NT_STATEMENTS;
    parser->rules[index].is_terminal[10] = true;
    parser->rules[index].symbols[10] = SEPARATOR_TOKEN;
    index++;

    // Rule 19: <loop_statement> ::= "luloop" "(" <expression> ")"
    parser->rules[index].lhs = NT_LOOP_STATEMENT;
    parser->rules[index].num_symbols = 4;  // Reduced to 4 symbols
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = KEYWORD_TOKEN;   // "luloop"
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;  // "("
    parser->rules[index].is_terminal[2] = false;
    parser->rules[index].symbols[2] = NT_EXPRESSION;  // <expression>
    parser->rules[index].is_terminal[3] = true;
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;  // ")"
    index++;

// Rule 20: <function_call> ::= <identifier> "(" <arguments> ")" ";"
parser->rules[index].lhs = NT_FUNCTION_CALL;
parser->rules[index].num_symbols = 5;
parser->rules[index].is_terminal[0] = true;
parser->rules[index].symbols[0] = IDENTIFIER_TOKEN;
parser->rules[index].is_terminal[1] = true;
parser->rules[index].symbols[1] = SEPARATOR_TOKEN;
parser->rules[index].is_terminal[2] = false;
parser->rules[index].symbols[2] = NT_ARGUMENTS;
parser->rules[index].is_terminal[3] = true;
parser->rules[index].symbols[3] = SEPARATOR_TOKEN;
parser->rules[index].is_terminal[4] = true;
parser->rules[index].symbols[4] = SEPARATOR_TOKEN;  // semicolon expected at the end
index++;

// Rule 21: <return_statement> ::= "return" <expression> ";"
parser->rules[index].lhs = NT_RETURN_STATEMENT;
parser->rules[index].num_symbols = 3;
parser->rules[index].is_terminal[0] = true;
parser->rules[index].symbols[0] = KEYWORD_TOKEN;
parser->rules[index].is_terminal[1] = false;
parser->rules[index].symbols[1] = NT_EXPRESSION;
parser->rules[index].is_terminal[2] = true;
parser->rules[index].symbols[2] = SEPARATOR_TOKEN;  // semicolon expected at the end
index++;

    // Rule 22: <expression> ::= <term> (<operator> <term>)*
    parser->rules[index].lhs = NT_EXPRESSION;
    parser->rules[index].num_symbols = 2;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_TERM;
    parser->rules[index].is_terminal[1] = false;
    parser->rules[index].symbols[1] = NT_OPERATOR;
    index++;

    // Rule 23: <term> ::= <identifier>
    parser->rules[index].lhs = NT_TERM;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = IDENTIFIER_TOKEN;
    index++;

    // Rule 24: <term> ::= <number>
    parser->rules[index].lhs = NT_TERM;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = NUMBER_TOKEN;
    index++;

    // Rule 25: <term> ::= <string_literal>
    parser->rules[index].lhs = NT_TERM;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = STRING_LITERAL_TOKEN;
    index++;

    // Rule 26: <term> ::= "(" <expression> ")"
    parser->rules[index].lhs = NT_TERM;
    parser->rules[index].num_symbols = 3;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[1] = false;
    parser->rules[index].symbols[1] = NT_EXPRESSION;
    parser->rules[index].is_terminal[2] = true;
    parser->rules[index].symbols[2] = SEPARATOR_TOKEN;
    index++;

    // Rule 27: <operator> ::= "+" | "-" | "*" | "/" | "%"
    parser->rules[index].lhs = NT_OPERATOR;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = OPERATOR_TOKEN;
    index++;

   // Rule 28: <operator> ::= ">" | "<" | ">=" | "<=" | "==" | "!="
    parser->rules[index].lhs = NT_OPERATOR;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = OPERATOR_TOKEN;
    index++;

    // Rule 29: <operator> ::= "and" | "or" | "not"
    parser->rules[index].lhs = NT_OPERATOR;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = LOGICAL_OP_TOKEN;
    index++;

    // Rule 30: <type> ::= "int" | "double" | "array" | "void" | "str"
    parser->rules[index].lhs = NT_TYPE;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = TYPE_TOKEN;
    index++;

    // Rule 31: <identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*
    parser->rules[index].lhs = NT_IDENTIFIER;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = IDENTIFIER_TOKEN;
    index++;

    // Rule 32: <number> ::= [0-9]+ ("." [0-9]+)?
    parser->rules[index].lhs = NT_NUMBER;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = NUMBER_TOKEN;
    index++;

    // Rule 33: <string_literal> ::= '"' (any_character_except_quote)* '"'
    parser->rules[index].lhs = NT_STRING_LITERAL;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = STRING_LITERAL_TOKEN;
    index++;

    // Rule 34: <separator> ::= ";" | "," | "(" | ")" | "{" | "}" | "[" | "]"
    parser->rules[index].lhs = NT_SEPARATOR;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = SEPARATOR_TOKEN;
    index++;

    // Rule 35: <comment> ::= "//" (any_character_except_newline)* "\n"
    parser->rules[index].lhs = NT_COMMENT;
    parser->rules[index].num_symbols = 1;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = COMMENT_TOKEN;
    index++;

    // Rule 36: <array_index> ::= <identifier> "[" <expression> "]" | <number> "[" <expression> "]"
    parser->rules[index].lhs = NT_ARRAY_INDEX;
    parser->rules[index].num_symbols = 4;
    parser->rules[index].is_terminal[0] = true;   // IDENTIFIER_TOKEN or NUMBER_TOKEN
    parser->rules[index].symbols[0] = IDENTIFIER_TOKEN; // Could be IDENTIFIER or NUMBER
    parser->rules[index].is_terminal[1] = true;   
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;  // '['
    parser->rules[index].is_terminal[2] = false;  
    parser->rules[index].symbols[2] = NT_EXPRESSION;    // <expression>
    parser->rules[index].is_terminal[3] = true;   
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;  // ']'
    index++;

    // Rule 37: <element_list> ::= <expression> ("," <expression>)*
    parser->rules[index].lhs = NT_ELEMENT_LIST;
    parser->rules[index].num_symbols = 3;
    parser->rules[index].is_terminal[0] = false;
    parser->rules[index].symbols[0] = NT_EXPRESSION;  // First element
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;  // ","
    parser->rules[index].is_terminal[2] = false;
    parser->rules[index].symbols[2] = NT_EXPRESSION;  // Next element
    index++;

    // Rule 38: <array_init> ::= "{" <element_list> "}" ";"
    parser->rules[index].lhs = NT_ARRAY_INIT;
    parser->rules[index].num_symbols = 4;
    parser->rules[index].is_terminal[0] = true;   // '{'
    parser->rules[index].symbols[0] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[1] = false;  // <element_list>
    parser->rules[index].symbols[1] = NT_ELEMENT_LIST;
    parser->rules[index].is_terminal[2] = true;   // '}'
    parser->rules[index].symbols[2] = SEPARATOR_TOKEN;
    parser->rules[index].is_terminal[3] = true;   // ';'
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;
    index++;

    // Rule 39: <identifier> "[" (<identifier> | <number>) "]" "=" <expression> ";"
    parser->rules[index].lhs = NT_ASSIGNMENT;
    parser->rules[index].num_symbols = 6;
    parser->rules[index].is_terminal[0] = true;
    parser->rules[index].symbols[0] = IDENTIFIER_TOKEN;  // array name
    parser->rules[index].is_terminal[1] = true;
    parser->rules[index].symbols[1] = SEPARATOR_TOKEN;   // '['
    parser->rules[index].is_terminal[2] = true;
    parser->rules[index].symbols[2] = IDENTIFIER_TOKEN | NUMBER_TOKEN;  // index can only be identifier or number
    parser->rules[index].is_terminal[3] = true;
    parser->rules[index].symbols[3] = SEPARATOR_TOKEN;   // ']'
    parser->rules[index].is_terminal[4] = true;
    parser->rules[index].symbols[4] = EQUAL_TOKEN;       // '='
    parser->rules[index].is_terminal[5] = false;
    parser->rules[index].symbols[5] = NT_EXPRESSION;     // Value expression
    parser->rules[index].is_terminal[6] = true;
    parser->rules[index].symbols[6] = SEPARATOR_TOKEN;   // ';'
    index++;
}


void init_parse_tables(Parser* parser) {
    // Initialize the action table and set default error actions
    parser->action_table = (Action**)malloc(parser->num_states * sizeof(Action*));
    if (!parser->action_table) {
        fprintf(stderr, "Memory allocation failed for action table\n");
        exit(1);
    }

    for (int i = 0; i < parser->num_states; i++) {
        parser->action_table[i] = (Action*)malloc(parser->num_terminals * sizeof(Action));
        if (!parser->action_table[i]) {
            fprintf(stderr, "Memory allocation failed for action table row\n");
            exit(1);
        }

        for (int j = 0; j < parser->num_terminals; j++) {
            parser->action_table[i][j].type = ACTION_ERROR;
            parser->action_table[i][j].value = 0;
        }
    }

    // Initialize the goto table
    parser->goto_table = (int**)malloc(parser->num_states * sizeof(int*));
    if (!parser->goto_table) {
        fprintf(stderr, "Memory allocation failed for goto table\n");
        exit(1);
    }

    for (int i = 0; i < parser->num_states; i++) {
        parser->goto_table[i] = (int*)malloc(parser->num_non_terminals * sizeof(int));
        if (!parser->goto_table[i]) {
            fprintf(stderr, "Memory allocation failed for goto table row\n");
            exit(1);
        }

        for (int j = 0; j < parser->num_non_terminals; j++) {
            parser->goto_table[i][j] = -1;
        }
    }

    // -------------------------------------------------------
    // State 0: Starting state
    add_lexeme_action(parser, 0, TYPE_TOKEN, "int", ACTION_SHIFT, 1);
    add_lexeme_action(parser, 0, TYPE_TOKEN, "double", ACTION_SHIFT, 1);
    add_lexeme_action(parser, 0, TYPE_TOKEN, "array", ACTION_SHIFT, 1);
    add_lexeme_action(parser, 0, TYPE_TOKEN, "void", ACTION_SHIFT, 1);
    add_lexeme_action(parser, 0, TYPE_TOKEN, "str", ACTION_SHIFT, 1);
    add_lexeme_action(parser, 0, SEPARATOR_TOKEN, "(", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 0, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 0, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 0);  // End of function declaration
    // -------------------------------------------------------
    // State 1: After reading <type> in a function, accept any identifier
    add_lexeme_action(parser, 1, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 2);
    add_lexeme_action(parser, 1, SEPARATOR_TOKEN, "(", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, SEPARATOR_TOKEN, "{", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, SEPARATOR_TOKEN, "(", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, IDENTIFIER_TOKEN, "*", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, KEYWORD_TOKEN, "if", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, KEYWORD_TOKEN, "return", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 1, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 0);  // End of function declaration, reduce

    // -------------------------------------------------------
    // State 2: After reading <type> <identifier>
    add_lexeme_action(parser, 2, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 3);
    add_lexeme_action(parser, 2, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, SEPARATOR_TOKEN, "{", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, SEPARATOR_TOKEN, "(", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, IDENTIFIER_TOKEN, "*", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, KEYWORD_TOKEN, "if", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, KEYWORD_TOKEN, "return", ACTION_REDUCE, 0);  // End of function declaration
    add_lexeme_action(parser, 2, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 0);  // End of function declaration

    // -------------------------------------------------------
    // State 3: Inside function parameter list
    add_lexeme_action(parser, 3, TYPE_TOKEN, "int", ACTION_SHIFT, 6);
    add_lexeme_action(parser, 3, TYPE_TOKEN, "double", ACTION_SHIFT, 6);
    add_lexeme_action(parser, 3, TYPE_TOKEN, "array", ACTION_SHIFT, 6);
    add_lexeme_action(parser, 3, TYPE_TOKEN, "void", ACTION_SHIFT, 6);
    add_lexeme_action(parser, 3, TYPE_TOKEN, "str", ACTION_SHIFT, 6);
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, ")", ACTION_SHIFT, 4);
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, ",", ACTION_SHIFT, 13);  // Continue parsing parameters
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 2);  // End of parameter list
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, "{", ACTION_REDUCE, 2);  // End of parameter list
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 2);  // End of parameter list
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, "(", ACTION_REDUCE, 2);  // End of parameter list
    add_lexeme_action(parser, 3, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 6);  // Accept any identifier
    add_lexeme_action(parser, 3, KEYWORD_TOKEN, "if", ACTION_SHIFT, 10);  // Expect an if statement
    add_lexeme_action(parser, 3, KEYWORD_TOKEN, "return", ACTION_SHIFT, 11);  // Expect a return statement
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 2);  // End of statement, reduce
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, "{", ACTION_SHIFT, 7);  // Expect a new function body
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 2);  // End of function body, reduce
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 20);  // Expect an expression
    add_lexeme_action(parser, 3, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 2);  // End of parameter list, reduce


    // -------------------------------------------------------
    // State 4: After the parameter list (after ")")
    add_lexeme_action(parser, 4, SEPARATOR_TOKEN, "{", ACTION_SHIFT, 7);
    add_lexeme_action(parser, 4, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1);  // End of function declaration
    add_lexeme_action(parser, 4, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 1);  // End of function declaration


    // -------------------------------------------------------
    // State 6: Parsing a parameter, accept any identifier
    add_lexeme_action(parser, 6, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 12);
    add_lexeme_action(parser, 6, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 3);  // End of parameter list
    add_lexeme_action(parser, 6, SEPARATOR_TOKEN, ",", ACTION_SHIFT, 13);  // Continue parsing parameters
    add_lexeme_action(parser, 6, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 3);  // End of parameter list

    // -------------------------------------------------------
    // State 7: Inside function body after "{", begin parsing <statements>
    add_lexeme_action(parser, 7, TYPE_TOKEN, "*", ACTION_SHIFT, 8);
    add_lexeme_action(parser, 7, ARRAY_TOKEN, "array", ACTION_SHIFT, 8);
    add_lexeme_action(parser, 7, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 9);  // Accept any identifier
    add_lexeme_action(parser, 7, KEYWORD_TOKEN, "if", ACTION_SHIFT, 10);
    add_lexeme_action(parser, 7, SEPARATOR_TOKEN, "}", ACTION_SHIFT, 11);
    add_lexeme_action(parser, 7, KEYWORD_TOKEN, "return", ACTION_SHIFT, 8);  // 8 should be your expression-handling state
    add_lexeme_action(parser, 7, SEPARATOR_TOKEN, ";", ACTION_SHIFT, 7);  // Shift to state 7 to continue parsing inside the function
    add_lexeme_action(parser, 7, KEYWORD_TOKEN, "lulog", ACTION_SHIFT, 33);
    add_lexeme_action(parser, 7, KEYWORD_TOKEN, "luloop", ACTION_SHIFT, 37);  

    // -------------------------------------------------------
    // State 8: Expect a <variable_declaration>
    add_lexeme_action(parser, 8, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 16);  // Expect an identifier for variable declaration
    add_lexeme_action(parser, 8, KEYWORD_TOKEN, "if", ACTION_SHIFT, 10);  // Expect an if statement
    add_lexeme_action(parser, 8, KEYWORD_TOKEN, "return", ACTION_SHIFT, 11);  // Expect a return statement
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1);  // End of statement, reduce
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, "{", ACTION_SHIFT, 7);  // Expect a new function body
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, "}", ACTION_SHIFT, 16);
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 20);  // Expect an expression
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 22);  // End of expression
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, ",", ACTION_REDUCE, 22);  // End of expression
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 22);  // End of expression
    add_lexeme_action(parser, 8, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 8, NUMBER_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 8, STRING_LITERAL_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 22);
    add_lexeme_action(parser, 8, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1);  // End of variable declaration, reduce

    // -------------------------------------------------------
    // State 9: Expect an <assignment>
    add_lexeme_action(parser, 9, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 16);  // Expect an identifier for assignment
    add_lexeme_action(parser, 9, KEYWORD_TOKEN, "if", ACTION_SHIFT, 10);  // Expect an if statement
    add_lexeme_action(parser, 9, KEYWORD_TOKEN, "return", ACTION_SHIFT, 11);  // Expect a return statement
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1);  // End of statement, reduce
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, "{", ACTION_SHIFT, 7);  // Expect a new function body
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 1);  // End of function body, reduce
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 20);  // Expect an expression
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 22);  // End of expression
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, ",", ACTION_REDUCE, 22);  // End of expression
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 22);  // End of expression
    add_lexeme_action(parser, 9, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1);  // End of return statement, reduce
    add_lexeme_action(parser, 9, EQUAL_TOKEN, "=", ACTION_SHIFT, 10);

    // -------------------------------------------------------
    // State 10: Inside an <if_statement>, expect "("
    add_lexeme_action(parser, 10, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 14);
    add_lexeme_action(parser, 10, SEPARATOR_TOKEN, "{", ACTION_SHIFT, 7);  // Expect a new function body
    add_lexeme_action(parser, 10, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 1);  // End of function body, reduce
    add_lexeme_action(parser, 10, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 10, NUMBER_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 10, STRING_LITERAL_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 10, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 20);


    // -------------------------------------------------------
    // State 11: End of function body
    add_lexeme_action(parser, 11, TYPE_TOKEN, "*", ACTION_SHIFT, 1);  // Expect new function (type) to follow after closing }
    add_lexeme_action(parser, 11, END_OF_TOKENS, "*", ACTION_ACCEPT, 0);  // Accept if we hit end of tokens
    add_lexeme_action(parser, 11, KEYWORD_TOKEN, "else", ACTION_SHIFT, 10);  // Expect else statement
    add_lexeme_action(parser, 11, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1);  // End of function body, reduce
    add_lexeme_action(parser, 11, KEYWORD_TOKEN, "luloop", ACTION_SHIFT, 37);
    add_lexeme_action(parser, 11, SEPARATOR_TOKEN, "}", ACTION_SHIFT, 11);  // End of function body, reduce

    
    // -------------------------------------------------------
    // State 12: After completing one parameter (<type> <identifier>)
    add_lexeme_action(parser, 12, SEPARATOR_TOKEN, ",", ACTION_SHIFT, 13);
    add_lexeme_action(parser, 12, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 3);
    add_lexeme_action(parser, 12, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 3);  // End of parameter list
    add_lexeme_action(parser, 12, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 3);  // End of parameter list

    // -------------------------------------------------------
    // State 13: After comma in parameter list, expect any identifier
    add_lexeme_action(parser, 13, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 6);
    add_lexeme_action(parser, 13, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 3);  // End of parameter list
    add_lexeme_action(parser, 13, SEPARATOR_TOKEN, ",", ACTION_SHIFT, 13);  // Continue parsing parameters

    // -------------------------------------------------------
    // State 14: Inside an if statement, expect an expression
    add_lexeme_action(parser, 14, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 15);  // Expect an identifier after '('
    add_lexeme_action(parser, 14, NUMBER_TOKEN, "*", ACTION_SHIFT, 15);  // Expect a number after '('

    // -------------------------------------------------------
    // State 15:
    add_lexeme_action(parser, 15, OPERATOR_TOKEN, ">", ACTION_SHIFT, 17);
    add_lexeme_action(parser, 15, OPERATOR_TOKEN, "<", ACTION_SHIFT, 17);
    add_lexeme_action(parser, 15, OPERATOR_TOKEN, "<=", ACTION_SHIFT, 17);
    add_lexeme_action(parser, 15, OPERATOR_TOKEN, ">=", ACTION_SHIFT, 17);
    add_lexeme_action(parser, 15, OPERATOR_TOKEN, "==", ACTION_SHIFT, 17);
    add_lexeme_action(parser, 15, OPERATOR_TOKEN, "!=", ACTION_SHIFT, 17);

    // -------------------------------------------------------
    // State 16: After reducing an assignment or statement, start parsing again
    add_lexeme_action(parser, 16, TYPE_TOKEN, "int", ACTION_SHIFT, 8);       // Start a new variable declaration
    add_lexeme_action(parser, 16, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 9);  // Start a new assignment or function call
    add_lexeme_action(parser, 16, OPERATOR_TOKEN, "*", ACTION_SHIFT, 20);  // Start a new expression
    add_lexeme_action(parser, 16, KEYWORD_TOKEN, "if", ACTION_SHIFT, 10);   // Start a new if statement
    add_lexeme_action(parser, 16, KEYWORD_TOKEN, "return", ACTION_SHIFT, 11); // Start a new return statement
    add_lexeme_action(parser, 16, KEYWORD_TOKEN, "luloop", ACTION_SHIFT, 12); // Start a new loop statement
    add_lexeme_action(parser, 16, SEPARATOR_TOKEN, "}", ACTION_SHIFT, 11);  // End of function body
    add_lexeme_action(parser, 16, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 16);  // End of statement, reduce
    add_lexeme_action(parser, 16, EQUAL_TOKEN, "=", ACTION_SHIFT, 20);  // Expect an expression after assignment
    add_lexeme_action(parser, 16, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 20);  // Expect an expression after assignment
    add_lexeme_action(parser, 16, SEPARATOR_TOKEN, "[", ACTION_SHIFT, 22);  // Expect an expression after assignment
    add_lexeme_action(parser, 16, IDENTIFIER_TOKEN, "{", ACTION_SHIFT, 7);  // Expect a new function body
    add_lexeme_action(parser, 16, KEYWORD_TOKEN, "lulog", ACTION_SHIFT, 30); // Start a new loop statement

    // -------------------------------------------------------
    // State 17:
    add_lexeme_action(parser, 17, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 18);  // Expect an identifier after the operator
    add_lexeme_action(parser, 17, NUMBER_TOKEN, "*", ACTION_SHIFT, 18);  // Expect a number after the operator

    //State 18:
    add_lexeme_action(parser, 18, LOGICAL_OP_TOKEN, "*", ACTION_SHIFT, 14);
    add_lexeme_action(parser, 18, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 17);

    

    

    // -------------------------------------------------------
    // State 20 : Inside an expression, expect a term
    add_lexeme_action(parser, 20, OPERATOR_TOKEN, "+", ACTION_SHIFT, 21);
    add_lexeme_action(parser, 20, OPERATOR_TOKEN, "-", ACTION_SHIFT, 21);  // Expect an operator after the term
    add_lexeme_action(parser, 20, OPERATOR_TOKEN, "*", ACTION_SHIFT, 21); 
    add_lexeme_action(parser, 20, OPERATOR_TOKEN, "/", ACTION_SHIFT, 21);  // Expect an operator after the term
    add_lexeme_action(parser, 20, OPERATOR_TOKEN, "%", ACTION_SHIFT, 21);  // Expect an operator after the term
    add_lexeme_action(parser, 20, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 20);  // Expect an identifier after the operator
    add_lexeme_action(parser, 20, NUMBER_TOKEN, "*", ACTION_SHIFT, 20);  // Expect a number after the operator
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 22);  // End of function body
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 16); 
    add_lexeme_action(parser, 20, STRING_LITERAL_TOKEN, "*", ACTION_SHIFT, 21);  // Expect a string literal after the operator '*'
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 21);  // Expect a '(' for a sub-expression after '*'
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, ")", ACTION_SHIFT, 40);  // End of expression, reduce
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, "]", ACTION_REDUCE, 36);  // End of array index, reduce
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, "{", ACTION_SHIFT, 23); 
    add_lexeme_action(parser, 20, SEPARATOR_TOKEN, "[", ACTION_SHIFT, 25); 

    // -------------------------------------------------------
    // State 21: After an operator in an expression, expect a term
    add_lexeme_action(parser, 21, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 21, NUMBER_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 21, STRING_LITERAL_TOKEN, "*", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 21, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 21, SEPARATOR_TOKEN, ")", ACTION_SHIFT, 20);
    add_lexeme_action(parser, 21, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 16); // Rule 16: <assignment> ::= <identifier> "=" <expression> ";"
    add_lexeme_action(parser, 16, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 1); // End of assignment, reduce

    // -------------------------------------------------------
    // State 22: Inside an array, expect an identifier or number
    add_lexeme_action(parser, 22, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 20);  // Expect an identifier after the '['
    add_lexeme_action(parser, 22, NUMBER_TOKEN, "*", ACTION_SHIFT, 20);  // Expect a number after the '['
    add_lexeme_action(parser, 22, SEPARATOR_TOKEN, "]", ACTION_SHIFT, 16);
    add_lexeme_action(parser, 22, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 38);  // End of array index, reduce

    // -------------------------------------------------------
    // State 23:
    add_lexeme_action(parser, 23, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 23);
    add_lexeme_action(parser, 23, NUMBER_TOKEN, "*", ACTION_SHIFT, 23);
    add_lexeme_action(parser, 23, SEPARATOR_TOKEN, ",", ACTION_SHIFT, 23);
    add_lexeme_action(parser, 23, SEPARATOR_TOKEN, "}", ACTION_REDUCE, 37);
    
    // -------------------------------------------------------
    //State 24:
    add_lexeme_action(parser, 24, SEPARATOR_TOKEN, "}", ACTION_SHIFT, 22);
    add_lexeme_action(parser, 24, SEPARATOR_TOKEN, ";", ACTION_SHIFT, 16);
    // -------------------------------------------------------
    //State 25:
    add_lexeme_action(parser, 25, NUMBER_TOKEN, "*", ACTION_SHIFT, 26);
    add_lexeme_action(parser, 25, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 26);

    // -------------------------------------------------------
    //State 26:
    add_lexeme_action(parser, 26, SEPARATOR_TOKEN, "]", ACTION_SHIFT, 27);

    // -------------------------------------------------------
    //State 27:
    add_lexeme_action(parser, 27, EQUAL_TOKEN, "=", ACTION_SHIFT, 28);
    
    // -------------------------------------------------------
    //State 28:
    add_lexeme_action(parser, 28, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 29);
    add_lexeme_action(parser, 28, NUMBER_TOKEN, "*", ACTION_SHIFT, 29);

    // -------------------------------------------------------
    //State 29:
    add_lexeme_action(parser, 29, OPERATOR_TOKEN, "*", ACTION_SHIFT, 28);
    add_lexeme_action(parser, 29, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 39); // End of assignment, reduce

    //State 30:
    add_lexeme_action(parser, 30, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 31);

    //State 31:
    add_lexeme_action(parser, 31, NUMBER_TOKEN, "*", ACTION_SHIFT, 32);
    add_lexeme_action(parser, 31, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 32);

    //State 32:
    add_lexeme_action(parser, 32, SEPARATOR_TOKEN, ")", ACTION_SHIFT, 8);

    //State 33:
    add_lexeme_action(parser, 33, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 34);

    //State 34:
    add_lexeme_action(parser, 34, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 35);
    add_lexeme_action(parser, 34, NUMBER_TOKEN, "*", ACTION_SHIFT, 35);
    add_lexeme_action(parser, 34, STRING_LITERAL_TOKEN, "*", ACTION_SHIFT, 35);

    //State 35:
    add_lexeme_action(parser, 35, SEPARATOR_TOKEN, ")", ACTION_SHIFT, 36);



    //State 36:
    add_lexeme_action(parser, 36, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 16);

    //State 37:
    add_lexeme_action(parser, 37, SEPARATOR_TOKEN, "(", ACTION_SHIFT, 38);

    //State 38:
    add_lexeme_action(parser, 38, IDENTIFIER_TOKEN, "*", ACTION_SHIFT, 39);
    add_lexeme_action(parser, 38, NUMBER_TOKEN, "*", ACTION_SHIFT, 39);

    //State 39:
    add_lexeme_action(parser, 39, OPERATOR_TOKEN, "<", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, OPERATOR_TOKEN, ">", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, OPERATOR_TOKEN, "<=", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, OPERATOR_TOKEN, ">=", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, OPERATOR_TOKEN, "==", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, OPERATOR_TOKEN, "!=", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, LOGICAL_OP_TOKEN, "*", ACTION_SHIFT, 38);
    add_lexeme_action(parser, 39, SEPARATOR_TOKEN, ")", ACTION_REDUCE, 19);
    

    //State 40:
    add_lexeme_action(parser, 40, SEPARATOR_TOKEN, ")", ACTION_SHIFT, 9);
    add_lexeme_action(parser, 40, SEPARATOR_TOKEN, ";", ACTION_REDUCE, 16); // End of assignment, reduce


    
    





    // -------------------------------------------------------
    // Final accepting state (state 50):
    add_lexeme_action(parser, 50, END_OF_TOKENS, "*", ACTION_ACCEPT, 0);
    // -------------------------------------------------------
    // Correct the goto transitions for non-terminals

    // After reducing NT_PROGRAM, it should go to the program handling state.
    parser->goto_table[0][NT_PROGRAM] = 50;  // Ensure this is correct; typically, this would be a valid state where program ends.
    parser->goto_table[0][NT_FUNCTION] = 49; // State where we expect a function after starting to parse program
    parser->goto_table[0][NT_TYPE] = 1;     // After starting to parse, move to type handling

    // After reducing NT_PARAMETERS in state 3, we need to go to the correct state (e.g., parsing a function or parameter).
    parser->goto_table[3][NT_PARAMETERS] = 4;  // Parsing parameters from function definition
    parser->goto_table[3][NT_PARAMETER] = 5;   // Parsing individual parameters in function definition

    // In state 7, we are parsing statements, so after reducing NT_STATEMENTS, it should go to the right statement handling state.
    parser->goto_table[7][NT_STATEMENTS] = 8;  // We expect to continue parsing statements in state 8
    parser->goto_table[7][NT_STATEMENT] = 9;   // After parsing a statement, we move to parsing the next statement

    // After parsing an expression (in state 10), the parser should proceed to another expression (for example in state 14).
    parser->goto_table[10][NT_EXPRESSION] = 14; // Continue expression parsing in state 14

    // If we have a reduced expression, move to handling a new term or expression.
    parser->goto_table[14][NT_EXPRESSION] = 20; // Continue with more expressions if necessary

    // States for function call or return statements handling
    parser->goto_table[7][NT_FUNCTION_CALL] = 10; // After handling a function call in state 7, move to state 10
    parser->goto_table[7][NT_RETURN_STATEMENT] = 11;  // After handling a return statement in state 7, go to state 11

    // Variable declaration handling (state 8 and 9)
    parser->goto_table[8][NT_VARIABLE_DECLARATION] = 16;  // Handle variable declaration after parsing type in state 8
    parser->goto_table[9][NT_ASSIGNMENT] = 16;  // Handle assignment after parsing an identifier in state 9

    // After parsing an IF statement in state 10, go to 11 for statement handling
    parser->goto_table[10][NT_IF_STATEMENT] = 11;  // After parsing an if statement, go to state 11 for further processing

    // Parameters in state 6 - these are similar to the ones in state 3
    parser->goto_table[6][NT_PARAMETERS] = 3;   // Return to parameters state
    parser->goto_table[6][NT_PARAMETER] = 4;    // After parsing one parameter, go to the next state

    // General statements handling for state 7
    parser->goto_table[7][NT_STATEMENTS] = 8;  // After parsing the statements, go to state 8 for further statement handling
    
    parser->goto_table[12][NT_PARAMETERS] = 3;   // Return to parameters state

    // State 9: After parsing an identifier, transition to NT_ASSIGNMENT
    parser->goto_table[9][NT_ASSIGNMENT] = 16;

    // State 10: After parsing an expression, transition to NT_ASSIGNMENT
    parser->goto_table[10][NT_ASSIGNMENT] = 16;

    // State 20: After reducing an assignment, transition to state 16
    parser->goto_table[20][NT_ASSIGNMENT] = 16;
    
    // State 16: After reducing a statement, transition to state 7 to continue parsing statements
    parser->goto_table[16][NT_STATEMENT] = 7;
    parser->goto_table[16][NT_STATEMENTS] = 7;

    // Fixing the state transition after reducing the ASSIGNMENT rule
    parser->goto_table[9][NT_ASSIGNMENT] = 7;  // After reducing the assignment, go to state 7 (valid next state for function body or next statement)

    // We should also ensure that after completing the assignment, it doesn't mistakenly go back to an invalid state (-1).
    parser->goto_table[16][NT_ASSIGNMENT] = 7;  // After reducing the assignment, go to state 7 (valid state for further parsing)

    parser->goto_table[7][NT_ASSIGNMENT] = 7;
    parser->goto_table[7][NT_STATEMENT] = 7;

    parser->goto_table[7][NT_ARRAY_INDEX] = 22;

    parser->goto_table[23][NT_ELEMENT_LIST] = 24;

    parser->goto_table[23][NT_ARRAY_INIT] = 24;

    parser->goto_table[22][NT_ASSIGNMENT] = 24;

    parser->goto_table[18][NT_IF_STATEMENT] = 32;

    parser->goto_table[37][NT_LOOP_STATEMENT] = 40;

}

ASTNode* create_ast_node(NonTerminal type, char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for AST node\n");
        exit(1);
    }
    
    node->type = type;
    node->children = NULL;
    node->num_children = 0;
    node->value = value ? strdup(value) : NULL;
    
    return node;
}

// Add a child to an AST node
// Add a child to an AST node with debug output
void add_child_to_ast_node(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) {
        printf("Warning: Attempt to add null node! parent: %p, child: %p\n", parent, child);
        return;
    }
    
    parent->children = (ASTNode**)realloc(parent->children, 
                                       (parent->num_children + 1) * sizeof(ASTNode*));
    if (!parent->children) {
        fprintf(stderr, "Memory reallocation failed for AST node children\n");
        exit(1);
    }
    
    parent->children[parent->num_children++] = child;
    printf("Added child to parent: parent=%s, child=%s, total children=%d\n", 
            parent->value ? parent->value : "NULL", 
            child->value ? child->value : "NULL", 
            parent->num_children);
}


// Define the token handler functions
void handle_type_token(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    // Variable declaration
    ASTNode* var_decl = create_ast_node(NT_VARIABLE_DECLARATION, "VarDecl");
    ASTNode* type_node = create_ast_node(NT_TYPE, parser->tokens[*stmt_start].value);
    ASTNode* var_name = create_ast_node(NT_IDENTIFIER, parser->tokens[*stmt_start+1].value);
    
    add_child_to_ast_node(var_decl, type_node);
    add_child_to_ast_node(var_decl, var_name);
    
    // Check if it has an initializer
    if (*stmt_start + 2 < *j && parser->tokens[*stmt_start+2].type == EQUAL_TOKEN) {
        ASTNode* init_expr = create_ast_node(NT_EXPRESSION, "Initializer");
        add_child_to_ast_node(var_decl, init_expr);
    }
    
    add_child_to_ast_node(body, var_decl);
}
void handle_identifier_token(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    // For identifiers, check if assignment or function call
    if (*stmt_start + 1 < *j && parser->tokens[*stmt_start+1].type == EQUAL_TOKEN) {
        // Assignment
        ASTNode* assign = create_ast_node(NT_ASSIGNMENT, "Assignment");
        ASTNode* var_name = create_ast_node(NT_IDENTIFIER, parser->tokens[*stmt_start].value);
        ASTNode* value = create_ast_node(NT_EXPRESSION, "Value");
        
        add_child_to_ast_node(assign, var_name);
        add_child_to_ast_node(assign, value);
        add_child_to_ast_node(body, assign);
    } else if (*stmt_start + 1 < *j && parser->tokens[*stmt_start+1].type == SEPARATOR_TOKEN && 
              strcmp(parser->tokens[*stmt_start+1].value, "(") == 0) {
        // Function call
        ASTNode* func_call = create_ast_node(NT_FUNCTION_CALL, "FunctionCall");
        ASTNode* func_name = create_ast_node(NT_IDENTIFIER, parser->tokens[*stmt_start].value);
        ASTNode* args = create_ast_node(NT_ARGUMENTS, "Arguments");
        
        add_child_to_ast_node(func_call, func_name);
        add_child_to_ast_node(func_call, args);
        add_child_to_ast_node(body, func_call);
    } else {
        // Generic identifier statement
        char debug_value[100];
        snprintf(debug_value, 100, "Statement [%s]", 
                parser->tokens[*stmt_start].value ? 
                parser->tokens[*stmt_start].value : "unknown");
        ASTNode* stmt = create_ast_node(NT_STATEMENT, debug_value);
        add_child_to_ast_node(body, stmt);
    }
}
void handle_keyword_token(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    char* keyword = parser->tokens[*stmt_start].value;
    
    if (strcmp(keyword, "if") == 0) {
        // If statement
        ASTNode* if_stmt = create_ast_node(NT_IF_STATEMENT, "If");
        ASTNode* condition = create_ast_node(NT_EXPRESSION, "Condition");
        add_child_to_ast_node(if_stmt, condition);
        add_child_to_ast_node(body, if_stmt);
    } else if (strcmp(keyword, "luloop") == 0) {
        // Loop statement
        ASTNode* loop_stmt = create_ast_node(NT_LOOP_STATEMENT, "Loop");
        ASTNode* condition = create_ast_node(NT_EXPRESSION, "Condition");
        add_child_to_ast_node(loop_stmt, condition);
        add_child_to_ast_node(body, loop_stmt);
    } else if (strcmp(keyword, "return") == 0) {
        // Return statement
        ASTNode* return_stmt = create_ast_node(NT_RETURN_STATEMENT, "Return");
        if (*stmt_start + 1 < *j) {
            ASTNode* return_val = create_ast_node(NT_EXPRESSION, "ReturnValue");
            add_child_to_ast_node(return_stmt, return_val);
        }
        add_child_to_ast_node(body, return_stmt);
    } else {
        // Unknown keyword
        char debug_value[100];
        snprintf(debug_value, 100, "Statement [%s]", keyword ? keyword : "unknown");
        ASTNode* stmt = create_ast_node(NT_STATEMENT, debug_value);
        add_child_to_ast_node(body, stmt);
    }
}
void handle_default_token(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    // Generic statement with debug info
    if (parser->tokens[*stmt_start].type == SEPARATOR_TOKEN && 
        strcmp(parser->tokens[*stmt_start].value, ";") == 0) {
        // Skip empty statements
        return;
    }
    
    char debug_value[100] = "Statement";
    if (parser->tokens[*stmt_start].type != END_OF_TOKENS) {
        snprintf(debug_value, 100, "Statement [%s]", 
                parser->tokens[*stmt_start].value ? 
                parser->tokens[*stmt_start].value : "unknown");
    }
    ASTNode* stmt = create_ast_node(NT_STATEMENT, debug_value);
    add_child_to_ast_node(body, stmt);
}
// Improved build_ast_from_tokens function using a simpler approach
// Simple token matcher
int token_matches(Token token, TokenType type, const char* value) {
    return (token.type == type) && 
           (!value || (token.value && strcmp(token.value, value) == 0));
}

// Function to build AST directly from tokens - fixed version
void build_ast_from_tokens(Parser* parser) {
    parser->ast_root = create_ast_node(NT_PROGRAM, "Program");
    
    // Debug token count
    printf("Analyzing tokens: ");
    int token_count = 0;
    while (parser->tokens[token_count].type != END_OF_TOKENS) token_count++;
    printf("%d tokens found\n", token_count);
    
    for (int i = 0; parser->tokens[i].type != END_OF_TOKENS; i++) {
        // Debug current position
        printf("Looking at token %d: %s (%s)\n", i, 
               parser->tokens[i].value ? parser->tokens[i].value : "NULL",
               get_token_type_name(parser->tokens[i].type));
        
        // Function detection pattern
        if (parser->tokens[i].type == TYPE_TOKEN &&
            i+1 < token_count && parser->tokens[i+1].type == IDENTIFIER_TOKEN &&
            i+2 < token_count && token_matches(parser->tokens[i+2], SEPARATOR_TOKEN, "(")) {
            
            printf("Found function: %s\n", parser->tokens[i+1].value);
            
            // Create function node
            ASTNode* function = create_ast_node(NT_FUNCTION, parser->tokens[i+1].value);
            add_child_to_ast_node(function, create_ast_node(NT_TYPE, parser->tokens[i].value));
            
            // Parse parameters
            ASTNode* params = create_ast_node(NT_PARAMETERS, "Parameters");
            int j = i + 3; // Skip to first parameter
            
            // Collect parameters until closing parenthesis
            while (j < token_count && !token_matches(parser->tokens[j], SEPARATOR_TOKEN, ")")) {
                if (parser->tokens[j].type == TYPE_TOKEN && 
                    j+1 < token_count && parser->tokens[j+1].type == IDENTIFIER_TOKEN) {
                    ASTNode* param = create_ast_node(NT_PARAMETER, "Parameter");
                    add_child_to_ast_node(param, create_ast_node(NT_TYPE, parser->tokens[j].value));
                    add_child_to_ast_node(param, create_ast_node(NT_IDENTIFIER, parser->tokens[j+1].value));
                    add_child_to_ast_node(params, param);
                    j += 2; // Skip type and identifier
                    
                    // Skip comma if present
                    if (j < token_count && token_matches(parser->tokens[j], SEPARATOR_TOKEN, ",")) {
                        j++;
                    }
                } else {
                    j++; // Skip non-parameter tokens
                }
            }
            
            add_child_to_ast_node(function, params);
            j++; // Skip closing parenthesis
            
            // Find function body start
            while (j < token_count && !token_matches(parser->tokens[j], SEPARATOR_TOKEN, "{")) {
                j++;
            }
            
            if (j < token_count) {
                printf("Found function body at token %d\n", j);
                
                // Create statements node for function body
                ASTNode* body = create_ast_node(NT_STATEMENTS, "Statements");
                int brace_count = 1;
                j++; // Skip opening brace
                
                // Process statements in function body
                while (j < token_count && brace_count > 0) {
                    // Track braces for nesting
                    if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "{")) {
                        brace_count++;
                    } else if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "}")) {
                        brace_count--;
                        if (brace_count == 0) {
                            break; // Exit at closing brace
                        }
                    }
                    
                    // Skip empty statements
                    if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, ";")) {
                        j++;
                        continue;
                    }
                    
                    // Handle statement based on first token
                    if (brace_count > 0) {
                        int stmt_start = j;
                        
                        // Process specific statement types
                        if (parser->tokens[j].type == TYPE_TOKEN) {
                            // Variable declaration
                            ASTNode* var_decl = create_ast_node(NT_VARIABLE_DECLARATION, "VarDecl");
                            add_child_to_ast_node(var_decl, create_ast_node(NT_TYPE, parser->tokens[j].value));
                            
                            if (j+1 < token_count && parser->tokens[j+1].type == IDENTIFIER_TOKEN) {
                                add_child_to_ast_node(var_decl, create_ast_node(NT_IDENTIFIER, parser->tokens[j+1].value));
                            }
                            
                            add_child_to_ast_node(body, var_decl);
                            
                            // Skip to end of statement
                            while (j < token_count && !token_matches(parser->tokens[j], SEPARATOR_TOKEN, ";") && brace_count > 0) {
                                if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "{")) {
                                    brace_count++;
                                } else if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "}")) {
                                    brace_count--;
                                    if (brace_count == 0) break;
                                }
                                j++;
                            }
                            j++; // Skip semicolon
                        } 
                        else if (parser->tokens[j].type == KEYWORD_TOKEN) {
                            // Handle keywords (if, loop, return)
                            if (strcmp(parser->tokens[j].value, "if") == 0) {
                                ASTNode* if_stmt = create_ast_node(NT_IF_STATEMENT, "If");
                                add_child_to_ast_node(if_stmt, create_ast_node(NT_EXPRESSION, "Condition"));
                                add_child_to_ast_node(body, if_stmt);
                            } 
                            else if (strcmp(parser->tokens[j].value, "luloop") == 0) {
                                ASTNode* loop = create_ast_node(NT_LOOP_STATEMENT, "Loop");
                                add_child_to_ast_node(loop, create_ast_node(NT_EXPRESSION, "Condition"));
                                add_child_to_ast_node(body, loop);
                            } 
                            else if (strcmp(parser->tokens[j].value, "return") == 0) {
                                ASTNode* ret = create_ast_node(NT_RETURN_STATEMENT, "Return");
                                add_child_to_ast_node(ret, create_ast_node(NT_EXPRESSION, "ReturnValue"));
                                add_child_to_ast_node(body, ret);
                            }
                            
                            // Skip to end of statement
                            while (j < token_count && !token_matches(parser->tokens[j], SEPARATOR_TOKEN, ";") && brace_count > 0) {
                                if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "{")) {
                                    brace_count++;
                                } else if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "}")) {
                                    brace_count--;
                                    if (brace_count == 0) break;
                                }
                                j++;
                            }
                            j++; // Skip semicolon
                        } 
                        else if (parser->tokens[j].type == IDENTIFIER_TOKEN) {
                            // Handle assignment or function call
                            if (j+1 < token_count && parser->tokens[j+1].type == EQUAL_TOKEN) {
                                ASTNode* assign = create_ast_node(NT_ASSIGNMENT, "Assignment");
                                add_child_to_ast_node(assign, create_ast_node(NT_IDENTIFIER, parser->tokens[j].value));
                                add_child_to_ast_node(assign, create_ast_node(NT_EXPRESSION, "Value"));
                                add_child_to_ast_node(body, assign);
                            } 
                            else if (j+1 < token_count && token_matches(parser->tokens[j+1], SEPARATOR_TOKEN, "(")) {
                                ASTNode* call = create_ast_node(NT_FUNCTION_CALL, "FunctionCall");
                                add_child_to_ast_node(call, create_ast_node(NT_IDENTIFIER, parser->tokens[j].value));
                                add_child_to_ast_node(call, create_ast_node(NT_ARGUMENTS, "Arguments"));
                                add_child_to_ast_node(body, call);
                            } 
                            else {
                                // Generic statement
                                ASTNode* stmt = create_ast_node(NT_STATEMENT, "Statement");
                                add_child_to_ast_node(body, stmt);
                            }
                            
                            // Skip to end of statement
                            while (j < token_count && !token_matches(parser->tokens[j], SEPARATOR_TOKEN, ";") && brace_count > 0) {
                                if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "{")) {
                                    brace_count++;
                                } else if (token_matches(parser->tokens[j], SEPARATOR_TOKEN, "}")) {
                                    brace_count--;
                                    if (brace_count == 0) break;
                                }
                                j++;
                            }
                            j++; // Skip semicolon
                        } 
                        else {
                            j++; // Skip other tokens
                        }
                    } 
                    else {
                        j++; // Skip tokens if not in a function body
                    }
                }
                
                add_child_to_ast_node(function, body);
            }
            
            // Add function to program
            add_child_to_ast_node(parser->ast_root, function);
            printf("Added function to AST: %s\n", parser->tokens[i+1].value);
            
            // Skip to end of function
            i = j;
        }
    }
    
    printf("AST building complete. Functions found: %d\n", parser->ast_root->num_children);
}
// Print AST with indentation
void print_ast(ASTNode* node, int depth) {
    if (!node) {
        printf("NULL node encountered in print_ast\n");
        return;
    }
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    // Print the node info
    printf("%s: %s\n", get_non_terminal_name(node->type), 
           node->value ? node->value : "NULL");
    
    // Print the number of children
    if (node->num_children > 0) {
        for (int i = 0; i < depth; i++) {
            printf("  ");
        }
        printf("  (Children: %d)\n", node->num_children);
    }
    
    // Print all children recursively
    for (int i = 0; i < node->num_children; i++) {
        print_ast(node->children[i], depth + 1);
    }
}
// Implementation for free_ast to avoid memory leaks
void free_ast(ASTNode* node) {
    if (!node) return;
    
    // Free all children first
    for (int i = 0; i < node->num_children; i++) {
        free_ast(node->children[i]);
    }
    
    // Free the children array
    if (node->children) {
        free(node->children);
    }
    
    // Free the value if it exists
    if (node->value) {
        free(node->value);
    }
    
    // Free the node itself
    free(node);
}



// Handle special cases (placeholder implementation)
void handle_special_cases(Rule rule, ASTNode* node) {
    // For now, this is a placeholder implementation
    // This function can be expanded in the future to handle specific rules
}
int action_shift(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item) {
    StackItem new_item = {action.value, current_token, true, 0, NULL};
    
    // Create AST node based on token type
    if (current_token.type == IDENTIFIER_TOKEN) new_item.ast_node = create_ast_node(NT_IDENTIFIER, current_token.value);
    else if (current_token.type == NUMBER_TOKEN) new_item.ast_node = create_ast_node(NT_NUMBER, current_token.value);
    else if (current_token.type == TYPE_TOKEN) new_item.ast_node = create_ast_node(NT_TYPE, current_token.value);
    else if (current_token.type == STRING_LITERAL_TOKEN) new_item.ast_node = create_ast_node(NT_STRING_LITERAL, current_token.value);
    
    push(&parser->stack, new_item);
    (*token_index)++;
    return 0;
}
int action_reduce(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item) {
    Rule rule = parser->rules[action.value];
    ASTNode* node = create_ast_node(rule.lhs, get_non_terminal_name(rule.lhs));
    ASTNode* children[10];
    int num_children = 0;
    
    // Pop symbols and collect children
    for (int i = 0; i < rule.num_symbols; i++) {
        StackItem item = pop(&parser->stack);
        if (item.ast_node) children[num_children++] = item.ast_node;
    }
    
    // Add children in reverse order
    for (int i = num_children - 1; i >= 0; i--) add_child_to_ast_node(node, children[i]);
    
    handle_special_cases(rule, node);
    
    // Create new item and push
    int state = peek(&parser->stack).state;
    StackItem new_item = {parser->goto_table[state][rule.lhs], {0}, false, rule.lhs, node};
    
    // Add function to program root
    if (rule.lhs == NT_FUNCTION) add_child_to_ast_node(parser->ast_root, node);
    
    push(&parser->stack, new_item);
    return 0;
}

int action_accept(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item) {
    // Add remaining functions to program root
    for (int i = 0; i <= parser->stack.top; i++) {
        if (!parser->stack.items[i].is_terminal && parser->stack.items[i].nt == NT_FUNCTION && parser->stack.items[i].ast_node) {
            add_child_to_ast_node(parser->ast_root, parser->stack.items[i].ast_node);
        }
    }
    return 1;
}
int action_error(Parser* parser, Action action, Token current_token, int* token_index, StackItem* top_item) {
    char error_msg[100];
    snprintf(error_msg, 100, "Syntax error at token '%s' (type: %s)", 
             current_token.value ? current_token.value : "NULL", 
             get_token_type_name(current_token.type));
    parser_error(parser, error_msg);
    return -1;
}


// Statement handlers
void handle_type_stmt(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    ASTNode* var_decl = create_ast_node(NT_VARIABLE_DECLARATION, "VarDecl");
    add_child_to_ast_node(var_decl, create_ast_node(NT_TYPE, parser->tokens[*stmt_start].value));
    add_child_to_ast_node(var_decl, create_ast_node(NT_IDENTIFIER, parser->tokens[*stmt_start+1].value));
    
    if (*stmt_start + 2 < *j && parser->tokens[*stmt_start+2].type == EQUAL_TOKEN)
        add_child_to_ast_node(var_decl, create_ast_node(NT_EXPRESSION, "Initializer"));
    
    add_child_to_ast_node(body, var_decl);
}
void handle_if_stmt(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    ASTNode* if_stmt = create_ast_node(NT_IF_STATEMENT, "If");
    add_child_to_ast_node(if_stmt, create_ast_node(NT_EXPRESSION, "Condition"));
    add_child_to_ast_node(body, if_stmt);
}
void handle_loop_stmt(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    ASTNode* loop_stmt = create_ast_node(NT_LOOP_STATEMENT, "Loop");
    add_child_to_ast_node(loop_stmt, create_ast_node(NT_EXPRESSION, "Condition"));
    add_child_to_ast_node(body, loop_stmt);
}
void handle_return_stmt(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    ASTNode* return_stmt = create_ast_node(NT_RETURN_STATEMENT, "Return");
    if (*stmt_start + 1 < *j) add_child_to_ast_node(return_stmt, create_ast_node(NT_EXPRESSION, "ReturnValue"));
    add_child_to_ast_node(body, return_stmt);
}
void handle_identifier_stmt(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    if (*stmt_start + 1 < *j && parser->tokens[*stmt_start+1].type == EQUAL_TOKEN) {
        // Assignment
        ASTNode* assign = create_ast_node(NT_ASSIGNMENT, "Assignment");
        add_child_to_ast_node(assign, create_ast_node(NT_IDENTIFIER, parser->tokens[*stmt_start].value));
        add_child_to_ast_node(assign, create_ast_node(NT_EXPRESSION, "Value"));
        add_child_to_ast_node(body, assign);
    } else if (*stmt_start + 1 < *j && parser->tokens[*stmt_start+1].type == SEPARATOR_TOKEN && 
               strcmp(parser->tokens[*stmt_start+1].value, "(") == 0) {
        // Function call
        ASTNode* func_call = create_ast_node(NT_FUNCTION_CALL, "FunctionCall");
        add_child_to_ast_node(func_call, create_ast_node(NT_IDENTIFIER, parser->tokens[*stmt_start].value));
        add_child_to_ast_node(func_call, create_ast_node(NT_ARGUMENTS, "Arguments"));
        add_child_to_ast_node(body, func_call);
    } else {
        // Generic statement
        char debug_value[100];
        snprintf(debug_value, 100, "Statement [%s]", parser->tokens[*stmt_start].value);
        add_child_to_ast_node(body, create_ast_node(NT_STATEMENT, debug_value));
    }
}
void handle_default_stmt(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    if (parser->tokens[*stmt_start].type == SEPARATOR_TOKEN && strcmp(parser->tokens[*stmt_start].value, ";") == 0) return;
    
    char debug_value[100] = "Statement";
    if (parser->tokens[*stmt_start].type != END_OF_TOKENS)
        snprintf(debug_value, 100, "Statement [%s]", parser->tokens[*stmt_start].value);
    
    add_child_to_ast_node(body, create_ast_node(NT_STATEMENT, debug_value));
}
// Process statement based on token type
void process_statement(Parser* parser, ASTNode* body, int* j, int* stmt_start) {
    TokenType type = parser->tokens[*stmt_start].type;
    
    if (type == KEYWORD_TOKEN) {
        char* keyword = parser->tokens[*stmt_start].value;
        if (strcmp(keyword, "if") == 0) handle_if_stmt(parser, body, j, stmt_start);
        else if (strcmp(keyword, "luloop") == 0) handle_loop_stmt(parser, body, j, stmt_start);
        else if (strcmp(keyword, "return") == 0) handle_return_stmt(parser, body, j, stmt_start);
        else handle_default_stmt(parser, body, j, stmt_start);
        return;
    }
    
    switch (type) {
        case TYPE_TOKEN: handle_type_stmt(parser, body, j, stmt_start); break;
        case IDENTIFIER_TOKEN: handle_identifier_stmt(parser, body, j, stmt_start); break;
        default: handle_default_stmt(parser, body, j, stmt_start); break;
    }
}
// Helper to parse a statement
int parse_statement(Parser* parser, int j, ASTNode* body, int brace_count) {
    int stmt_start = j;
    
    // Find statement end
    while (j < 1000 && parser->tokens[j].type != END_OF_TOKENS && brace_count > 0 && 
          !(parser->tokens[j].type == SEPARATOR_TOKEN && strcmp(parser->tokens[j].value, ";") == 0)) {
        
        if (parser->tokens[j].type == SEPARATOR_TOKEN) {
            if (strcmp(parser->tokens[j].value, "{") == 0) brace_count++;
            else if (strcmp(parser->tokens[j].value, "}") == 0) {
                brace_count--;
                if (brace_count == 0) break;
            }
        }
        j++;
    }
    
    // Process the statement
    process_statement(parser, body, &j, &stmt_start);
    
    // Skip semicolon
    if (j < 1000 && parser->tokens[j].type != END_OF_TOKENS && 
        parser->tokens[j].type == SEPARATOR_TOKEN && strcmp(parser->tokens[j].value, ";") == 0) {
        j++;
    }
    
    return j;
}
// Helper to parse parameters
int parse_parameters(Parser* parser, int j, ASTNode* function) {
    ASTNode* params = create_ast_node(NT_PARAMETERS, "Parameters");
    
    // Add parameters until closing parenthesis
    while (parser->tokens[j].type != SEPARATOR_TOKEN || strcmp(parser->tokens[j].value, ")") != 0) {
        if (parser->tokens[j].type == TYPE_TOKEN && parser->tokens[j+1].type == IDENTIFIER_TOKEN) {
            ASTNode* param = create_ast_node(NT_PARAMETER, "Parameter");
            add_child_to_ast_node(param, create_ast_node(NT_TYPE, parser->tokens[j].value));
            add_child_to_ast_node(param, create_ast_node(NT_IDENTIFIER, parser->tokens[j+1].value));
            add_child_to_ast_node(params, param);
            j += 2;
        } else {
            j++;
        }
    }
    
    add_child_to_ast_node(function, params);
    
    // Skip to opening brace
    while (parser->tokens[j].type != SEPARATOR_TOKEN || strcmp(parser->tokens[j].value, "{") != 0) j++;
    
    return j;
}
// Helper to parse function body
int parse_function_body(Parser* parser, int j, ASTNode* function) {
    ASTNode* body = create_ast_node(NT_STATEMENTS, "Statements");
    int brace_count = 1;
    j++; // Skip opening brace
    
    while (brace_count > 0 && parser->tokens[j].type != END_OF_TOKENS) {
        // Track braces
        if (parser->tokens[j].type == SEPARATOR_TOKEN) {
            if (strcmp(parser->tokens[j].value, "{") == 0) brace_count++;
            else if (strcmp(parser->tokens[j].value, "}") == 0) brace_count--;
            
            if (brace_count == 0) break;
            if (strcmp(parser->tokens[j].value, ";") == 0) { j++; continue; }
        }
        
        if (brace_count > 0) {
            j = parse_statement(parser, j, body, brace_count);
        } else {
            j++;
        }
    }
    
    add_child_to_ast_node(function, body);
    return j;
}
// Helper to parse a function definition
int parse_function(Parser* parser, int i) {
    ASTNode* function = create_ast_node(NT_FUNCTION, parser->tokens[i+1].value);
    add_child_to_ast_node(function, create_ast_node(NT_TYPE, parser->tokens[i].value));
    
    int j = parse_parameters(parser, i+3, function); // Parse parameters, starting after '('
    j = parse_function_body(parser, j, function);    // Parse function body
    
    add_child_to_ast_node(parser->ast_root, function);
    return j;
}
// Ultra-short parse function using function pointers
void parse(Parser* parser) {
    // Initialize action handlers
    pFun action_handlers[4] = {
        action_shift,
        action_reduce,
        action_accept,
        action_error
    };
    
    parser->ast_root = create_ast_node(NT_PROGRAM, "Program");
    int token_index = 0;
    
    while (1) {
        StackItem top_item = peek(&parser->stack);
        Token current_token = parser->tokens[token_index];
        Action action = get_action(parser, top_item.state, current_token);
        
        int result = action_handlers[action.type](parser, action, current_token, &token_index, &top_item);
        if (result != 0) return; // Exit on accept or error
    }
}
// Function to print the AST with indentation for readability


char* get_token_type_name(TokenType type) {
    static char* names[] = {"NUMBER", "KEYWORD", "TYPE", "STRING_LITERAL", "STRING_ERROR", 
                            "IDENTIFIER", "SEPARATOR", "OPERATOR", "EQUAL", "LOGICAL_OP", 
                            "ARRAY", "COMMENT", "END_OF_TOKENS"};
    return (type >= 0 && type <= END_OF_TOKENS) ? names[type] : "UNKNOWN";
}
// Non-terminal name function - simple array lookup 
char* get_non_terminal_name(NonTerminal nt) {
    static char* names[] = {"Program", "Function", "Parameters", "Parameter", "Statements", 
                            "Statement", "VariableDeclaration", "Assignment", "IfStatement", 
                            "LoopStatement", "FunctionCall", "ReturnStatement", "Expression", 
                            "Term", "Operator", "Type", "Arguments", "ArgumentList", 
                           "Identifier", "Number", "StringLiteral", "Separator", "Comment", 
                           "ArrayIndex", "ElementList", "ArrayInit"};
    return (nt >= 0 && nt < NT_ARRAY_INIT + 1) ? names[nt] : "Unknown";
}


// Function to handle parser errors
void parser_error(Parser* parser, const char* message) {
    parser->has_error = true;
    if (parser->error_message) {
        free(parser->error_message);
    }
    parser->error_message = strdup(message);
    printf("Parser Error: %s\n", message);
}
// Function to free the parser
void free_parser(Parser* parser) {
    if (!parser) return;
    
    // Free rules
    if (parser->rules) {
        free(parser->rules);
    }
    
    // Free action table
    if (parser->action_table) {
        for (int i = 0; i < parser->num_states; i++) {
            if (parser->action_table[i]) {
                free(parser->action_table[i]);
            }
        }
        free(parser->action_table);
    }
    
    // Free goto table
    if (parser->goto_table) {
        for (int i = 0; i < parser->num_states; i++) {
            if (parser->goto_table[i]) {
                free(parser->goto_table[i]);
            }
        }
        free(parser->goto_table);
    }
    
    // Free lexeme actions
    free_lexeme_actions(parser);
    
    // Free error message
    if (parser->error_message) {
        free(parser->error_message);
    }
    
    // Free AST
    if (parser->ast_root) {
        free_ast(parser->ast_root);
    }
    
    // Free stack
    free_stack(&parser->stack);
    
    // Free the parser itself
    free(parser);
}
