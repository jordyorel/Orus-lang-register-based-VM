#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "memory.h"

// Forward declaration for parse_precedence
static ASTNode* parse_precedence(Parser* parser, Precedence precedence);

// Add these forward declarations at the top with the others
static ASTNode* parseString(Parser* parser);
static ASTNode* parseNumber(Parser* parser);
static ASTNode* parseGrouping(Parser* parser);
static ASTNode* parseUnary(Parser* parser);
static ASTNode* parseBinary(Parser* parser, ASTNode* left);
static ASTNode* parseBoolean(Parser* parser);
static ASTNode* parseVariable(Parser* parser);
static Type* parseType(Parser* parser);
static void expression(Parser* parser, ASTNode** ast);
static void statement(Parser* parser, ASTNode** ast);
static void consumeStatementEnd(Parser* parser);

static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) return;
    parser->panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (token->type != TOKEN_ERROR)
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    fprintf(stderr, ": %s\n", message);
    parser->hadError = true;
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    for (;;) {
        parser->current = scan_token();
        if (parser->current.type != TOKEN_ERROR) break;
        errorAt(parser, &parser->current, parser->current.start);
    }
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    error(parser, message);
}

static bool match(Parser* parser, TokenType type) {
    if (parser->current.type != type) return false;
    advance(parser);
    return true;
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static ASTNode* parseString(Parser* parser) {
    const char* start = parser->previous.start + 1;  // Skip opening quote
    int length = parser->previous.length - 2;        // Exclude both quotes
    char* string = (char*)malloc(length + 1);
    memcpy(string, start, length);
    string[length] = '\0';
    Value value =
        STRING_VAL(string, length);  // Use both arguments: chars and len
    ASTNode* node = createLiteralNode(value);
    node->valueType =
        createPrimitiveType(TYPE_STRING);  // Assuming TYPE_STRING exists
    return node;
}

static ASTNode* parseNumber(Parser* parser) {
    const char* start = parser->previous.start;
    int length = parser->previous.length;
    char* endptr;
    bool isFloat = false;
    for (int i = 0; i < length; i++) {
        if (start[i] == '.') {
            isFloat = true;
            break;
        }
    }
    ASTNode* node;
    if (isFloat) {
        double value = strtod(start, &endptr);
        node = createLiteralNode(F64_VAL(value));
        node->valueType = createPrimitiveType(TYPE_F64);
    } else {
        long value = strtol(start, &endptr, 10);
        if (value >= INT32_MIN && value <= INT32_MAX) {
            node = createLiteralNode(I32_VAL((int32_t)value));
            node->valueType = createPrimitiveType(TYPE_I32);
        } else if (value >= 0 && value <= UINT32_MAX) {
            node = createLiteralNode(U32_VAL((uint32_t)value));
            node->valueType = createPrimitiveType(TYPE_U32);
        } else {
            double dvalue = strtod(start, &endptr);
            node = createLiteralNode(F64_VAL(dvalue));
            node->valueType = createPrimitiveType(TYPE_F64);
        }
    }
    return node;
}

static ASTNode* parseGrouping(Parser* parser) {
    ASTNode* expr = parse_precedence(parser, PREC_ASSIGNMENT);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    return expr;
}

static ASTNode* parseUnary(Parser* parser) {
    Token operator= parser->previous;
    ASTNode* operand = parse_precedence(parser, PREC_UNARY);
    return createUnaryNode(operator, operand);
}

static ASTNode* parseBinary(Parser* parser, ASTNode* left) {
    Token operator= parser->previous;
    ParseRule* rule = get_rule(operator.type);
    ASTNode* right =
        parse_precedence(parser, (Precedence)(rule->precedence + 1));
    return createBinaryNode(operator, left, right);
}

static ASTNode* parseVariable(Parser* parser) {
    Token name = parser->previous;
    return createVariableNode(name, 0);  // Index will be resolved by Compiler
}

static ASTNode* parse_precedence(Parser* parser, Precedence precedence) {
    advance(parser);
    ParseFn prefixRule = get_rule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(parser, "Expected expression.");
        return NULL;
    }
    ASTNode* left = prefixRule(parser);
    while (!parser->hadError &&
           precedence <= get_rule(parser->current.type)->precedence) {
        advance(parser);
        ASTNode* (*infixRule)(Parser*, ASTNode*) =
            get_rule(parser->previous.type)->infix;
        left = infixRule(parser, left);
    }
    return left;
}

// static ASTNode* parsePrimary(Parser* parser) {
//     if (match(parser, TOKEN_LEFT_PAREN)) {
//         return parseGrouping(parser);
//     }
//     if (match(parser, TOKEN_NUMBER)) {
//         return parseNumber(parser);
//     }
//     if (match(parser, TOKEN_STRING)) {
//         return parseString(parser);
//     }
//     if (match(parser, TOKEN_IDENTIFIER)) {
//         return parseVariable(parser);
//     }
//     if (match(parser, TOKEN_TRUE) || match(parser, TOKEN_FALSE)) {
//         return parseBoolean(parser);
//     }
//     error(parser, "Expected expression.");
//     return NULL;
// }

static void expression(Parser* parser, ASTNode** ast) {
    *ast = parse_precedence(parser, PREC_ASSIGNMENT);
}

static void consumeStatementEnd(Parser* parser) {
    if (match(parser, TOKEN_SEMICOLON)) {
        while (match(parser, TOKEN_NEWLINE));  // Consume optional newlines after semicolon
    } else if (match(parser, TOKEN_NEWLINE) || check(parser, TOKEN_EOF)) {
        // Accept newline or EOF as statement end
    } else {
        error(parser, "Expect newline or ';' after statement.");
    }
}

static void statement(Parser* parser, ASTNode** ast) {
    if (match(parser, TOKEN_PRINT)) {
        printf("DEBUG: Parsing print statement\n");
        consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'print'.");
        ASTNode* expr;
        expression(parser, &expr);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after print value.");
        consumeStatementEnd(parser);
        *ast = createPrintNode(expr);
        printf("DEBUG: Created AST_PRINT node\n");
    } else if (match(parser, TOKEN_LET)) {
#ifdef DEBUG_PARSER
        printf("DEBUG: Parsing let statement\n");
#endif
        consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
        Token name = parser->previous;
        Type* type = NULL;
        
        if (match(parser, TOKEN_COLON)) {
            type = parseType(parser);
            printf("DEBUG: Variable type annotation: %s\n", 
                   type ? getTypeName(type->kind) : "null");
            if (parser->hadError) return;
        }
        
        consume(parser, TOKEN_EQUAL, "Expect '=' after variable name.");
        ASTNode* initializer;
        expression(parser, &initializer);
        printf("DEBUG: Initializer type: %s\n", 
               initializer->valueType ? getTypeName(initializer->valueType->kind) : "null");
        
        consumeStatementEnd(parser);
        *ast = createLetNode(name, type, initializer);
    } else {
        ASTNode* expr;
        expression(parser, &expr);
        consumeStatementEnd(parser);
        *ast = createPrintNode(expr);  // Wrap standalone expressions in print
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {parseGrouping, NULL, PREC_NONE},
    [TOKEN_MINUS] = {parseUnary, parseBinary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, parseBinary, PREC_TERM},
    [TOKEN_SLASH] = {NULL, parseBinary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, parseBinary, PREC_FACTOR},
    [TOKEN_MODULO] = {NULL, parseBinary, PREC_FACTOR},
    [TOKEN_NUMBER] = {parseNumber, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {parseVariable, NULL, PREC_NONE},
    [TOKEN_STRING] = {parseString, NULL, PREC_NONE},
    [TOKEN_TRUE] = {parseBoolean, NULL, PREC_NONE},
    [TOKEN_FALSE] = {parseBoolean, NULL, PREC_NONE},
    // Add other tokens as needed
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

ParseRule* get_rule(TokenType type) { return &rules[type]; }

void initParser(Parser* parser, Scanner* scanner) {
    parser->current = (Token){0};
    parser->previous = (Token){0};
    parser->hadError = false;
    parser->panicMode = false;
    parser->scanner = scanner;
}

bool parse(const char* source, ASTNode** ast) {
    Scanner scanner;
    init_scanner(source);
    Parser parser;
    initParser(&parser, &scanner);
    advance(&parser);

    *ast = NULL;
    ASTNode* current = NULL;
    while (!match(&parser, TOKEN_EOF)) {
        ASTNode* stmt;
        statement(&parser, &stmt);
        if (parser.hadError) {
            if (*ast) freeASTNode(*ast);  // Free the entire AST
            return false;
        }
        if (!*ast) {
            *ast = stmt;  // First statement
        } else {
            current->next = stmt;  // Chain statements as siblings
            // Don't set parent - they're siblings
        }
        current = stmt;
    }
    return !parser.hadError;
}

static Type* parseType(Parser* parser) {
    Type* type = NULL;
    if (match(parser, TOKEN_INT)) {
        type = getPrimitiveType(TYPE_I32);
    } else if (match(parser, TOKEN_U32)) {
        type = getPrimitiveType(TYPE_U32);
    } else if (match(parser, TOKEN_F64)) {
        type = getPrimitiveType(TYPE_F64);
    } else if (match(parser, TOKEN_BOOL)) {
        type = getPrimitiveType(TYPE_BOOL);
        printf("DEBUG: Parsed bool type\n");
    } else {
        error(parser, "Expected type name (int, u32, f64, or bool).");
        return NULL;
    }
    
    if (type == NULL) {
        printf("DEBUG: Failed to get primitive type\n");
        error(parser, "Failed to get primitive type");
        return NULL;
    }
    
    return type;
}

static ASTNode* parseBoolean(Parser* parser) {
    bool value = parser->previous.type == TOKEN_TRUE;
    printf("DEBUG: Creating boolean node with value: %s\n", value ? "true" : "false");
    ASTNode* node = createLiteralNode(BOOL_VAL(value));
    node->valueType = getPrimitiveType(TYPE_BOOL);
    printf("DEBUG: Boolean node type set to: %s\n", getTypeName(node->valueType->kind));
    return node;
}