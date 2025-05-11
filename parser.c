#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new parser
Parser* create_parser(Token* tokens) {
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->tokens = tokens;
    parser->pos = 0;
    parser->root = NULL;
    
    return parser;
}

// Free a parser and its AST
void free_parser(Parser* parser) {
    if (!parser) return;
    
    if (parser->root) {
        free_node(parser->root);
    }
    
    free(parser);
}

// Create a new AST node
ASTNode* create_node(NodeType type, const char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->num_children = 0;
    node->capacity = 10;  // Initial capacity
    node->children = malloc(node->capacity * sizeof(ASTNode*));
    node->parent = NULL;
    
    if (!node->children) {
        free(node->value);
        free(node);
        return NULL;
    }
    
    return node;
}

// Add a child node to a parent node
void add_child(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) return;
    
    // Resize children array if needed
    if (parent->num_children >= parent->capacity) {
        parent->capacity *= 2;
        parent->children = realloc(parent->children, parent->capacity * sizeof(ASTNode*));
        if (!parent->children) return;
    }
    
    // Add child
    parent->children[parent->num_children++] = child;
    child->parent = parent;
}

// Free an AST node and all its children
void free_node(ASTNode* node) {
    if (!node) return;
    
    // Free children
    for (int i = 0; i < node->num_children; i++) {
        free_node(node->children[i]);
    }
    
    // Free node data
    free(node->value);
    free(node->children);
    free(node);
}

// Print an AST for debugging
void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    // Print node info
    switch (node->type) {
        case NODE_PROGRAM:
            printf("PROGRAM\n");
            break;
        case NODE_FUNCTION:
            printf("FUNCTION: %s\n", node->value);
            break;
        case NODE_TYPE:
            printf("TYPE: %s\n", node->value);
            break;
        case NODE_PARAM:
            printf("PARAM\n");
            break;
        case NODE_BLOCK:
            printf("BLOCK\n");
            break;
        case NODE_STATEMENT:
            printf("STATEMENT\n");
            break;
        case NODE_VAR_DECL:
            printf("VAR_DECL: %s\n", node->value);
            break;
        case NODE_EXPR:
            printf("EXPR: %s\n", node->value);
            break;
        case NODE_BINARY_OP:
            printf("BINARY_OP: %s\n", node->value);
            break;
        case NODE_RETURN:
            printf("RETURN\n");
            break;
        case NODE_IDENTIFIER:
            printf("IDENTIFIER: %s\n", node->value);
            break;
        case NODE_NUMBER:
            printf("NUMBER: %s\n", node->value);
            break;
        case NODE_STRING:
            printf("STRING: %s\n", node->value);
            break;
        case NODE_IF:
            printf("IF\n");
            break;
        case NODE_ELSE:
            printf("ELSE\n");
            break;
        case NODE_LULOOP:
            printf("LULOOP\n");
            break;
        case NODE_LULOG:
            printf("LULOG\n");
            break;
        case NODE_LULOAD:
            printf("LULOAD\n");
            break;
        case NODE_CONDITION:
            printf("CONDITION\n");
            break;
        default:
            printf("UNKNOWN NODE TYPE\n");
            break;
    }
    
    // Print children
    for (int i = 0; i < node->num_children; i++) {
        print_ast(node->children[i], indent + 1);
    }
}

// Helper functions for parsing
static bool is_token_type(Parser* parser, TokenType type) {
    if (parser->pos >= 0 && parser->tokens[parser->pos].type != END_OF_TOKENS) {
        return parser->tokens[parser->pos].type == type;
    }
    return false;
}

static Token* current_token(Parser* parser) {
    if (parser->pos >= 0 && parser->tokens[parser->pos].type != END_OF_TOKENS) {
        return &parser->tokens[parser->pos];
    }
    return NULL;
}

static void advance(Parser* parser) {
    if (parser->tokens[parser->pos].type != END_OF_TOKENS) {
        parser->pos++;
    }
}

// Debug function to print the current token
static void print_current_token(Parser* parser) {
    Token* token = current_token(parser);
    if (token) {
        printf("DEBUG: Current token: type=%d, value='%s'\n", 
            token->type, token->value ? token->value : "NULL");
    } else {
        printf("DEBUG: Current token: NULL or end of tokens\n");
    }
}

// Forward declarations
ASTNode* parse_program(Parser* parser);
ASTNode* parse_function(Parser* parser);
ASTNode* parse_parameter_list(Parser* parser);
ASTNode* parse_block(Parser* parser);
ASTNode* parse_statement(Parser* parser);
ASTNode* parse_variable_decl(Parser* parser);
ASTNode* parse_return(Parser* parser);
ASTNode* parse_expression(Parser* parser);
ASTNode* parse_condition(Parser* parser);
ASTNode* parse_if_statement(Parser* parser);
ASTNode* parse_luloop_statement(Parser* parser);
ASTNode* parse_lulog_statement(Parser* parser);
ASTNode* parse_luload_statement(Parser* parser);

// Parse a program
ASTNode* parse_program(Parser* parser) {
    ASTNode* program = create_node(NODE_PROGRAM, NULL);
    
    // Parse function definitions
    while (parser->tokens[parser->pos].type != END_OF_TOKENS) {
        ASTNode* function = parse_function(parser);
        if (function) {
            add_child(program, function);
        } else {
            // Error recovery: advance to next potential function declaration
            fprintf(stderr, "Error parsing function declaration. Attempting to recover...\n");
            
            // Advance until we find another potential function declaration (TYPE_TOKEN)
            // or end of tokens
            while (parser->tokens[parser->pos].type != END_OF_TOKENS &&
                   parser->tokens[parser->pos].type != TYPE_TOKEN) {
                advance(parser);
            }
            
            // If we're at the end of tokens, break the loop
            if (parser->tokens[parser->pos].type == END_OF_TOKENS) {
                break;
            }
        }
    }
    
    return program;
}

// Parse a function
ASTNode* parse_function(Parser* parser) {
    // Function return type
    if (!is_token_type(parser, TYPE_TOKEN)) {
        fprintf(stderr, "Expected function return type\n");
        return NULL;
    }
    
    ASTNode* type = create_node(NODE_TYPE, current_token(parser)->value);
    advance(parser);
    
    // Function name
    if (!is_token_type(parser, IDENTIFIER_TOKEN)) {
        fprintf(stderr, "Expected function name\n");
        free_node(type);
        return NULL;
    }
    
    ASTNode* function = create_node(NODE_FUNCTION, current_token(parser)->value);
    add_child(function, type);
    advance(parser);
    
    // Parameter list
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "(") != 0) {
        fprintf(stderr, "Expected '(' after function name\n");
        free_node(function);
        return NULL;
    }
    advance(parser);
    
    ASTNode* params = parse_parameter_list(parser);
    if (!params) {
        fprintf(stderr, "Failed to parse parameter list\n");
        free_node(function);
        return NULL;
    }
    add_child(function, params);
    
    // Function body
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "{") != 0) {
        fprintf(stderr, "Expected '{' after function parameters\n");
        free_node(function);
        return NULL;
    }
    advance(parser);
    
    ASTNode* body = parse_block(parser);
    if (!body) {
        fprintf(stderr, "Failed to parse function body\n");
        free_node(function);
        return NULL;
    }
    add_child(function, body);
    
    // Final closing brace
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "}") != 0) {
        fprintf(stderr, "Expected '}' after function body\n");
        free_node(function);
        return NULL;
    }
    advance(parser);
    
    return function;
}

// Parse a parameter list
ASTNode* parse_parameter_list(Parser* parser) {
    ASTNode* params = create_node(NODE_PARAM, NULL);
    
    // Check if there are no parameters
    if (is_token_type(parser, SEPARATOR_TOKEN) && 
        strcmp(current_token(parser)->value, ")") == 0) {
        advance(parser);
        return params;
    }
    
    // Parse parameters
    while (true) {
        // Parameter type
        if (!is_token_type(parser, TYPE_TOKEN)) {
            fprintf(stderr, "Expected parameter type\n");
            free_node(params);
            return NULL;
        }
        
        ASTNode* type = create_node(NODE_TYPE, current_token(parser)->value);
        advance(parser);
        
        // Parameter name
        if (!is_token_type(parser, IDENTIFIER_TOKEN)) {
            fprintf(stderr, "Expected parameter name\n");
            free_node(type);
            free_node(params);
            return NULL;
        }
        
        ASTNode* param = create_node(NODE_VAR_DECL, current_token(parser)->value);
        add_child(param, type);
        add_child(params, param);
        advance(parser);
        
        // Check for more parameters
        if (is_token_type(parser, SEPARATOR_TOKEN) && 
            strcmp(current_token(parser)->value, ",") == 0) {
            advance(parser);
            continue;
        }
        
        // End of parameter list
        if (is_token_type(parser, SEPARATOR_TOKEN) && 
            strcmp(current_token(parser)->value, ")") == 0) {
            advance(parser);
            break;
        }
        
        fprintf(stderr, "Expected ',' or ')' in parameter list\n");
        free_node(params);
        return NULL;
    }
    
    return params;
}

// Parse a block of statements
ASTNode* parse_block(Parser* parser) {
    ASTNode* block = create_node(NODE_BLOCK, NULL);
    
    // Parse statements until we hit a closing brace
    while (!is_token_type(parser, SEPARATOR_TOKEN) || 
           strcmp(current_token(parser)->value, "}") != 0) {
           
        // Check for 'else' keyword which should be handled by the if statement parser
        // and not as a standalone statement in a block
        if (is_token_type(parser, KEYWORD_TOKEN) && 
            strcmp(current_token(parser)->value, "else") == 0) {
            // Found an 'else' without a matching 'if', which is a syntax error
            // But we'll break out to avoid infinite loops
            fprintf(stderr, "Error: 'else' without matching 'if'\n");
            break;
        }
        
        ASTNode* statement = parse_statement(parser);
        if (statement) {
            add_child(block, statement);
        } else {
            // Skip invalid statement
            fprintf(stderr, "Skipping invalid statement\n");
            
            // Try to resynchronize by finding the next semicolon or brace
            while (parser->tokens[parser->pos].type != END_OF_TOKENS) {
                if (is_token_type(parser, SEPARATOR_TOKEN) && 
                    (strcmp(current_token(parser)->value, ";") == 0 ||
                     strcmp(current_token(parser)->value, "}") == 0)) {
                    break;
                }
                // Also break on 'else' keyword to prevent infinite loops
                if (is_token_type(parser, KEYWORD_TOKEN) && 
                    strcmp(current_token(parser)->value, "else") == 0) {
                    break;
                }
                advance(parser);
            }
            
            // Skip the semicolon but not the closing brace or else keyword
            if (is_token_type(parser, SEPARATOR_TOKEN) && 
                strcmp(current_token(parser)->value, ";") == 0) {
                advance(parser);
            }
        }
        
        // Check for end of tokens
        if (parser->tokens[parser->pos].type == END_OF_TOKENS) {
            fprintf(stderr, "Unexpected end of tokens in block\n");
            break;
        }
    }
    
    return block;
}

// Parse a statement
ASTNode* parse_statement(Parser* parser) {
    // Variable declaration
    if (is_token_type(parser, TYPE_TOKEN)) {
        return parse_variable_decl(parser);
    }
    
    // Return statement
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "return") == 0) {
        return parse_return(parser);
    }
    
    // If statement
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "if") == 0) {
        return parse_if_statement(parser);
    }
    
    // luloop statement
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "luloop") == 0) {
        return parse_luloop_statement(parser);
    }
    
    // lulog statement
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "lulog") == 0) {
        return parse_lulog_statement(parser);
    }
    
    // luload statement
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "luload") == 0) {
        return parse_luload_statement(parser);
    }
    
    // Assignment statement
    if (is_token_type(parser, IDENTIFIER_TOKEN)) {
        ASTNode* id = create_node(NODE_IDENTIFIER, current_token(parser)->value);
        advance(parser);
        
        if (is_token_type(parser, EQUAL_TOKEN)) {
            #ifdef DEBUG_PARSER
            printf("DEBUG: Found assignment with '=' token\n");
            #endif
            advance(parser);
            
            // Parse right-side expression
            #ifdef DEBUG_PARSER
            printf("DEBUG: Parsing expression on right side of assignment\n");
            print_current_token(parser);
            #endif
            
            ASTNode* expr = parse_expression(parser);
            if (!expr) {
                free_node(id);
                fprintf(stderr, "Failed to parse expression in assignment\n");
                return NULL;
            }
            
            // Expect semicolon
            if (!is_token_type(parser, SEPARATOR_TOKEN) ||
                strcmp(current_token(parser)->value, ";") != 0) {
                fprintf(stderr, "Expected ';' after assignment\n");
                free_node(id);
                free_node(expr);
                return NULL;
            }
            advance(parser);
            
            // Create assignment node
            ASTNode* assign = create_node(NODE_EXPR, "=");
            add_child(assign, id);
            add_child(assign, expr);
            
            #ifdef DEBUG_PARSER
            printf("DEBUG: Successfully created assignment node\n");
            #endif
            
            return assign;
        } else {
            free_node(id);
            fprintf(stderr, "Expected '=' in assignment\n");
            return NULL;
        }
    }
    
    fprintf(stderr, "Unrecognized statement\n");
    return NULL;
}

// Parse a variable declaration
ASTNode* parse_variable_decl(Parser* parser) {
    // Variable type
    ASTNode* type = create_node(NODE_TYPE, current_token(parser)->value);
    advance(parser);
    
    // Variable name
    if (!is_token_type(parser, IDENTIFIER_TOKEN)) {
        fprintf(stderr, "Expected variable name\n");
        free_node(type);
        return NULL;
    }
    
    ASTNode* var_decl = create_node(NODE_VAR_DECL, current_token(parser)->value);
    add_child(var_decl, type);
    advance(parser);
    
    // Optional initialization
    if (is_token_type(parser, EQUAL_TOKEN)) {
        advance(parser);
        
        // Parse the expression after the equals sign
        ASTNode* expr = NULL;
        
        // We might have a simple expression (number/id) or a binary operation
        // First, parse the left side
        if (is_token_type(parser, NUMBER_TOKEN)) {
            expr = create_node(NODE_NUMBER, current_token(parser)->value);
            advance(parser);
            
            // Check if an operator follows the number
            if (is_token_type(parser, OPERATOR_TOKEN)) {
                const char* op_value = current_token(parser)->value;
                advance(parser);
                
                // Create binary operation node
                ASTNode* binary_op = create_node(NODE_BINARY_OP, op_value);
                add_child(binary_op, expr);
                
                // Parse right side
                if (is_token_type(parser, NUMBER_TOKEN)) {
                    ASTNode* right = create_node(NODE_NUMBER, current_token(parser)->value);
                    add_child(binary_op, right);
                    advance(parser);
                    expr = binary_op;
                } else if (is_token_type(parser, IDENTIFIER_TOKEN)) {
                    ASTNode* right = create_node(NODE_IDENTIFIER, current_token(parser)->value);
                    add_child(binary_op, right);
                    advance(parser);
                    expr = binary_op;
                } else {
                    fprintf(stderr, "Expected number or identifier on right side of operator\n");
                    free_node(binary_op);
                    free_node(var_decl);
                    return NULL;
                }
            }
        } else if (is_token_type(parser, IDENTIFIER_TOKEN)) {
            expr = create_node(NODE_IDENTIFIER, current_token(parser)->value);
            advance(parser);
            
            // Check if an operator follows the identifier
            if (is_token_type(parser, OPERATOR_TOKEN)) {
                const char* op_value = current_token(parser)->value;
                advance(parser);
                
                // Create binary operation node
                ASTNode* binary_op = create_node(NODE_BINARY_OP, op_value);
                add_child(binary_op, expr);
                
                // Parse right side
                if (is_token_type(parser, NUMBER_TOKEN)) {
                    ASTNode* right = create_node(NODE_NUMBER, current_token(parser)->value);
                    add_child(binary_op, right);
                    advance(parser);
                    expr = binary_op;
                } else if (is_token_type(parser, IDENTIFIER_TOKEN)) {
                    ASTNode* right = create_node(NODE_IDENTIFIER, current_token(parser)->value);
                    add_child(binary_op, right);
                    advance(parser);
                    expr = binary_op;
                } else {
                    fprintf(stderr, "Expected number or identifier on right side of operator\n");
                    free_node(binary_op);
                    free_node(var_decl);
                    return NULL;
                }
            }
        } else {
            // If not a direct expression, try the regular expression parser
            expr = parse_expression(parser);
        }
        
        if (!expr) {
            fprintf(stderr, "Failed to parse initialization expression\n");
            free_node(var_decl);
            return NULL;
        }
        
        add_child(var_decl, expr);
    }
    
    // Semicolon required
    if (!is_token_type(parser, SEPARATOR_TOKEN) ||
        strcmp(current_token(parser)->value, ";") != 0) {
        fprintf(stderr, "Expected ';' after variable declaration\n");
        free_node(var_decl);
        return NULL;
    }
    advance(parser);
    
    return var_decl;
}

// Parse a return statement
ASTNode* parse_return(Parser* parser) {
    ASTNode* ret = create_node(NODE_RETURN, NULL);
    advance(parser); // Consume 'return'
    
    // Optional return expression
    if (!is_token_type(parser, SEPARATOR_TOKEN) ||
        strcmp(current_token(parser)->value, ";") != 0) {
        ASTNode* expr = parse_expression(parser);
        if (expr) {
            add_child(ret, expr);
        } else {
            fprintf(stderr, "Failed to parse return expression\n");
            free_node(ret);
            return NULL;
        }
    }
    
    // Semicolon required
    if (!is_token_type(parser, SEPARATOR_TOKEN) ||
        strcmp(current_token(parser)->value, ";") != 0) {
        fprintf(stderr, "Expected ';' after return statement\n");
        free_node(ret);
        return NULL;
    }
    advance(parser);
    
    return ret;
}

// Parse a condition expression (for if statements and luloop)
ASTNode* parse_condition(Parser* parser) {
    // Open parenthesis
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "(") != 0) {
        fprintf(stderr, "Expected '(' after if/luloop\n");
        return NULL;
    }
    advance(parser);
    
    // Create condition node
    ASTNode* condition = create_node(NODE_CONDITION, NULL);
    
    // Left side of condition
    if (is_token_type(parser, IDENTIFIER_TOKEN)) {
        ASTNode* left = create_node(NODE_IDENTIFIER, current_token(parser)->value);
        add_child(condition, left);
        advance(parser);
        
        // Comparison operator
        if (is_token_type(parser, EQUAL_TOKEN) || is_token_type(parser, OPERATOR_TOKEN)) {
            // Create a binary op node for the comparison
            ASTNode* op = create_node(NODE_BINARY_OP, current_token(parser)->value);
            add_child(op, left);
            advance(parser);
            
            // Right side of condition
            ASTNode* right;
            if (is_token_type(parser, NUMBER_TOKEN)) {
                right = create_node(NODE_NUMBER, current_token(parser)->value);
                advance(parser);
            } else if (is_token_type(parser, IDENTIFIER_TOKEN)) {
                right = create_node(NODE_IDENTIFIER, current_token(parser)->value);
                advance(parser);
            } else {
                fprintf(stderr, "Expected expression after comparison operator\n");
                free_node(condition);
                return NULL;
            }
            
            add_child(op, right);
            // Replace the direct left child with the operator node that contains both operands
            condition->children[0] = op;
        } else {
            fprintf(stderr, "Expected comparison operator in condition\n");
            free_node(condition);
            return NULL;
        }
    } else {
        fprintf(stderr, "Expected identifier as first part of condition\n");
        free_node(condition);
        return NULL;
    }
    
    // Close parenthesis
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, ")") != 0) {
        fprintf(stderr, "Expected ')' after condition\n");
        free_node(condition);
        return NULL;
    }
    advance(parser);
    
    return condition;
}

// Parse an expression (improved)
ASTNode* parse_expression(Parser* parser) {
    #ifdef DEBUG_PARSER
    printf("DEBUG: Starting parse_expression\n");
    print_current_token(parser);
    #endif
    
    // Handle string literals
    if (is_token_type(parser, STRING_LITERAL_TOKEN)) {
        ASTNode* str = create_node(NODE_STRING, current_token(parser)->value);
        advance(parser);
        #ifdef DEBUG_PARSER
        printf("DEBUG: Parsed string literal\n");
        #endif
        return str;
    }
    
    // Handle parenthesized expressions
    if (is_token_type(parser, SEPARATOR_TOKEN) && strcmp(current_token(parser)->value, "(") == 0) {
        advance(parser); // Consume '('
        
        ASTNode* expr = parse_expression(parser);
        if (!expr) {
            fprintf(stderr, "Failed to parse expression inside parentheses\n");
            return NULL;
        }
        
        if (!is_token_type(parser, SEPARATOR_TOKEN) || strcmp(current_token(parser)->value, ")") != 0) {
            fprintf(stderr, "Expected closing parenthesis ')'\n");
            free_node(expr);
            return NULL;
        }
        advance(parser); // Consume ')'
        
        // Check for operator after parenthesized expression
        if (is_token_type(parser, OPERATOR_TOKEN)) {
            ASTNode* op = create_node(NODE_BINARY_OP, current_token(parser)->value);
            add_child(op, expr);
            advance(parser);
            
            ASTNode* right = parse_expression(parser);
            if (!right) {
                fprintf(stderr, "Expected right operand after operator\n");
                free_node(op);
                return NULL;
            }
            
            add_child(op, right);
            return op;
        }
        
        return expr;
    }
    
    // Handle numbers
    if (is_token_type(parser, NUMBER_TOKEN)) {
        ASTNode* num = create_node(NODE_NUMBER, current_token(parser)->value);
        advance(parser);
        
        // Check for operator after number
        if (is_token_type(parser, OPERATOR_TOKEN)) {
            #ifdef DEBUG_PARSER
            printf("DEBUG: Found binary operator after number: %s\n", current_token(parser)->value);
            #endif
            
            ASTNode* op = create_node(NODE_BINARY_OP, current_token(parser)->value);
            add_child(op, num);
            advance(parser);
            
            ASTNode* right = parse_expression(parser);
            if (!right) {
                fprintf(stderr, "Expected right operand after operator\n");
                free_node(op);
                return NULL;
            }
            
            add_child(op, right);
            return op;
        }
        
        #ifdef DEBUG_PARSER
        printf("DEBUG: Parsed number without operator\n");
        #endif
        return num;
    }
    
    // Handle luload keyword
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "luload") == 0) {
        ASTNode* luload_node = create_node(NODE_LULOAD, NULL);
        advance(parser); // Consume 'luload'
        
        // Check for opening parenthesis
        if (!is_token_type(parser, SEPARATOR_TOKEN) || 
            strcmp(current_token(parser)->value, "(") != 0) {
            fprintf(stderr, "Expected '(' after luload\n");
            free_node(luload_node);
            return NULL;
        }
        advance(parser);
        
        // Check for closing parenthesis - luload doesn't take arguments
        if (!is_token_type(parser, SEPARATOR_TOKEN) || 
            strcmp(current_token(parser)->value, ")") != 0) {
            fprintf(stderr, "Expected ')' for luload\n");
            free_node(luload_node);
            return NULL;
        }
        advance(parser);
        
        return luload_node;
    }
    
    // Handle identifiers
    if (is_token_type(parser, IDENTIFIER_TOKEN)) {
        #ifdef DEBUG_PARSER
        printf("DEBUG: Found identifier in expression: %s\n", current_token(parser)->value);
        #endif
        
        // Save the identifier value for potential function call
        const char* identifier_value = current_token(parser)->value;
        ASTNode* id = create_node(NODE_IDENTIFIER, identifier_value);
        advance(parser);
        
        // Not handling function calls in this version

        if (is_token_type(parser, OPERATOR_TOKEN)) {
            #ifdef DEBUG_PARSER
            printf("DEBUG: Found binary operator after identifier: %s\n", current_token(parser)->value);
            #endif
            
            ASTNode* op = create_node(NODE_BINARY_OP, current_token(parser)->value);
            add_child(op, id);
            advance(parser);
            
            #ifdef DEBUG_PARSER
            printf("DEBUG: Parsing right operand of binary operation\n");
            print_current_token(parser);
            #endif
            
            ASTNode* right = parse_expression(parser);
            if (!right) {
                fprintf(stderr, "Expected right operand after operator\n");
                free_node(op);
                return NULL;
            }
            
            add_child(op, right);
            #ifdef DEBUG_PARSER
            printf("DEBUG: Successfully created binary operation node\n");
            #endif
            return op;
        }
        
        #ifdef DEBUG_PARSER
        printf("DEBUG: Parsed identifier without operator\n");
        #endif
        return id;
    }
    
    fprintf(stderr, "Expected expression\n");
    return NULL;
}

// Parse an if statement
ASTNode* parse_if_statement(Parser* parser) {
    ASTNode* if_node = create_node(NODE_IF, NULL);
    advance(parser); // Consume 'if'
    
    // Parse condition
    ASTNode* condition = parse_condition(parser);
    if (!condition) {
        free_node(if_node);
        return NULL;
    }
    add_child(if_node, condition);
    
    // Parse if block
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "{") != 0) {
        fprintf(stderr, "Expected '{' after if condition\n");
        free_node(if_node);
        return NULL;
    }
    advance(parser);
    
    ASTNode* if_body = parse_block(parser);
    if (!if_body) {
        fprintf(stderr, "Failed to parse if body\n");
        free_node(if_node);
        return NULL;
    }
    add_child(if_node, if_body);
    
    // Check for closing brace
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "}") != 0) {
        fprintf(stderr, "Expected '}' after if body\n");
        free_node(if_node);
        return NULL;
    }
    advance(parser);
    
    // Check for else
    if (is_token_type(parser, KEYWORD_TOKEN) && 
        strcmp(current_token(parser)->value, "else") == 0) {
        ASTNode* else_node = create_node(NODE_ELSE, NULL);
        add_child(if_node, else_node);
        advance(parser);
        
        // Parse else block
        if (!is_token_type(parser, SEPARATOR_TOKEN) || 
            strcmp(current_token(parser)->value, "{") != 0) {
            fprintf(stderr, "Expected '{' after else\n");
            free_node(if_node);
            return NULL;
        }
        advance(parser);
        
        ASTNode* else_body = parse_block(parser);
        if (!else_body) {
            fprintf(stderr, "Failed to parse else body\n");
            free_node(if_node);
            return NULL;
        }
        add_child(else_node, else_body);
        
        // Check for closing brace
        if (!is_token_type(parser, SEPARATOR_TOKEN) || 
            strcmp(current_token(parser)->value, "}") != 0) {
            fprintf(stderr, "Expected '}' after else body\n");
            free_node(if_node);
            return NULL;
        }
        advance(parser);
    }
    
    return if_node;
}

// Parse a luloop statement
ASTNode* parse_luloop_statement(Parser* parser) {
    ASTNode* luloop_node = create_node(NODE_LULOOP, NULL);
    advance(parser); // Consume 'luloop'
    
    // Parse condition
    ASTNode* condition = parse_condition(parser);
    if (!condition) {
        free_node(luloop_node);
        return NULL;
    }
    add_child(luloop_node, condition);
    
    // Parse loop block
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "{") != 0) {
        fprintf(stderr, "Expected '{' after luloop condition\n");
        free_node(luloop_node);
        return NULL;
    }
    advance(parser);
    
    ASTNode* loop_body = parse_block(parser);
    if (!loop_body) {
        fprintf(stderr, "Failed to parse luloop body\n");
        free_node(luloop_node);
        return NULL;
    }
    add_child(luloop_node, loop_body);
    
    // Check for closing brace
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "}") != 0) {
        fprintf(stderr, "Expected '}' after luloop body\n");
        free_node(luloop_node);
        return NULL;
    }
    advance(parser);
    
    return luloop_node;
}

// Parse a lulog statement
ASTNode* parse_lulog_statement(Parser* parser) {
    ASTNode* lulog_node = create_node(NODE_LULOG, NULL);
    advance(parser); // Consume 'lulog'
    
    // Check for opening parenthesis (optional)
    bool has_parentheses = false;
    if (is_token_type(parser, SEPARATOR_TOKEN) && 
        strcmp(current_token(parser)->value, "(") == 0) {
        has_parentheses = true;
        advance(parser);
    }
    
    // Parse argument
    ASTNode* arg = NULL;
    if (is_token_type(parser, STRING_LITERAL_TOKEN)) {
        arg = create_node(NODE_STRING, current_token(parser)->value);
        advance(parser);
    } else if (is_token_type(parser, IDENTIFIER_TOKEN)) {
        arg = create_node(NODE_IDENTIFIER, current_token(parser)->value);
        advance(parser);
    } else if (is_token_type(parser, NUMBER_TOKEN)) {
        arg = create_node(NODE_NUMBER, current_token(parser)->value);
        advance(parser);
    } else {
        fprintf(stderr, "Expected argument in lulog\n");
        free_node(lulog_node);
        return NULL;
    }
    add_child(lulog_node, arg);
    
    // Parse closing parenthesis if we had an opening one
    if (has_parentheses) {
        if (!is_token_type(parser, SEPARATOR_TOKEN) || 
            strcmp(current_token(parser)->value, ")") != 0) {
            fprintf(stderr, "Expected ')' after lulog argument\n");
            free_node(lulog_node);
            return NULL;
        }
        advance(parser);
    }
    
    // Parse semicolon
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, ";") != 0) {
        fprintf(stderr, "Expected ';' after lulog statement\n");
        free_node(lulog_node);
        return NULL;
    }
    advance(parser);
    
    return lulog_node;
}

// Parse a luload statement
ASTNode* parse_luload_statement(Parser* parser) {
    ASTNode* luload_node = create_node(NODE_LULOAD, NULL);
    advance(parser); // Consume 'luload'
    
    // Check for opening parenthesis
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, "(") != 0) {
        fprintf(stderr, "Expected '(' after luload\n");
        free_node(luload_node);
        return NULL;
    }
    advance(parser);
    
    // Check for closing parenthesis - luload doesn't take arguments
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, ")") != 0) {
        fprintf(stderr, "Expected ')' for luload\n");
        free_node(luload_node);
        return NULL;
    }
    advance(parser);
    
    // Parse semicolon
    if (!is_token_type(parser, SEPARATOR_TOKEN) || 
        strcmp(current_token(parser)->value, ";") != 0) {
        fprintf(stderr, "Expected ';' after luload()\n");
        free_node(luload_node);
        return NULL;
    }
    advance(parser);
    
    return luload_node;
}

// Main parsing function
void parse(Parser* parser) {
    parser->root = parse_program(parser);
    
    if (parser->root) {
        printf("AST built successfully.\n");
        print_ast(parser->root, 0);
    } else {
        fprintf(stderr, "Failed to build AST.\n");
    }
}