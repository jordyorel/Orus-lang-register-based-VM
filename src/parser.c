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
static void ifStatement(Parser* parser, ASTNode** ast);
static void block(Parser* parser, ASTNode** ast);
static void consumeStatementEnd(Parser* parser);
static void synchronize(Parser* parser);

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
    printf("DEBUG: Advancing parser\n");
    parser->previous = parser->current;
    for (;;) {
        printf("DEBUG: Scanning token\n");
        Token token = scan_token();
        printf("DEBUG: Token scanned: type=%d\n", token.type);

        // Special handling for newline tokens
        if (token.type == TOKEN_NEWLINE) {
            printf("DEBUG: Handling newline token safely\n");
            parser->current = token;
            break;
        }

        if (token.type != TOKEN_ERROR) {
            parser->current = token;
            break;
        }

        printf("DEBUG: Error token encountered\n");
        errorAt(parser, &token, token.start);
    }
    printf("DEBUG: Parser advanced successfully\n");
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    error(parser, message);
}

static bool match(Parser* parser, TokenType type) {
    printf("DEBUG: Matching token type %d with current token type %d\n", type, parser->current.type);
    if (parser->current.type != type) return false;

    // Special handling for newlines to avoid potential issues
    if (type == TOKEN_NEWLINE) {
        printf("DEBUG: Matching newline token safely\n");
        parser->previous = parser->current;

        // Safely get the next token
        Token next = scan_token();
        printf("DEBUG: After newline, got token type %d\n", next.type);
        parser->current = next;
        return true;
    }

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
    printf("DEBUG: Parsing precedence %d\n", precedence);
    advance(parser);

    // Check for EOF
    if (check(parser, TOKEN_EOF)) {
        printf("DEBUG: Found EOF in parse_precedence\n");
        error(parser, "Unexpected end of file.");
        return NULL;
    }

    ParseFn prefixRule = get_rule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        printf("DEBUG: No prefix rule for token type %d\n", parser->previous.type);
        error(parser, "Expected expression.");
        return NULL;
    }

    printf("DEBUG: Calling prefix rule for token type %d\n", parser->previous.type);
    ASTNode* left = prefixRule(parser);
    if (left == NULL) {
        printf("DEBUG: Prefix rule returned NULL\n");
        return NULL;
    }

    while (!parser->hadError &&
           precedence <= get_rule(parser->current.type)->precedence) {
        printf("DEBUG: Processing infix operation with token type %d\n", parser->current.type);
        advance(parser);
        ASTNode* (*infixRule)(Parser*, ASTNode*) =
            get_rule(parser->previous.type)->infix;
        if (infixRule == NULL) {
            printf("DEBUG: No infix rule for token type %d\n", parser->previous.type);
            error(parser, "Invalid infix operator.");
            freeASTNode(left);
            return NULL;
        }

        ASTNode* newLeft = infixRule(parser, left);
        if (newLeft == NULL) {
            printf("DEBUG: Infix rule returned NULL\n");
            freeASTNode(left);
            return NULL;
        }
        left = newLeft;
    }

    printf("DEBUG: Parsed expression successfully\n");
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
    printf("DEBUG: Parsing expression\n");
    *ast = NULL; // Initialize to NULL in case of error
    ASTNode* expr = parse_precedence(parser, PREC_ASSIGNMENT);
    if (expr == NULL) {
        printf("DEBUG: Expression parsing failed\n");
        return;
    }
    *ast = expr;
    printf("DEBUG: Expression parsed successfully\n");
}

static void consumeStatementEnd(Parser* parser) {
    printf("DEBUG: Consuming statement end\n");

    // Check for EOF first
    if (check(parser, TOKEN_EOF)) {
        printf("DEBUG: Found EOF at statement end\n");
        return;
    }

    // Check for semicolon - this is now an error
    if (check(parser, TOKEN_SEMICOLON)) {
        printf("DEBUG: Found semicolon at statement end - this is an error\n");
        error(parser, "Semicolons are not used in this language. Use newlines to terminate statements.");
        // Skip the semicolon to continue parsing
        match(parser, TOKEN_SEMICOLON);
        return;
    }

    // Check for newline
    if (check(parser, TOKEN_NEWLINE)) {
        printf("DEBUG: Found newline at statement end\n");
        match(parser, TOKEN_NEWLINE); // Use our safer match function

        // Consume any additional newlines safely
        printf("DEBUG: Checking for additional newlines\n");
        int newlineCount = 0;
        while (check(parser, TOKEN_NEWLINE) && newlineCount < 10) { // Limit to avoid infinite loops
            printf("DEBUG: Found additional newline %d\n", ++newlineCount);
            match(parser, TOKEN_NEWLINE); // Use our safer match function
        }
        return;
    }

    printf("DEBUG: Expected newline or EOF, but got token type %d\n", parser->current.type);
    error(parser, "Expect newline after statement.");
}

static void statement(Parser* parser, ASTNode** ast) {
    // Skip any leading newlines
    while (check(parser, TOKEN_NEWLINE)) {
        printf("DEBUG: Skipping leading newline in statement\n");
        advance(parser);
    }

    // Check for EOF
    if (check(parser, TOKEN_EOF)) {
        printf("DEBUG: Found EOF at start of statement\n");
        *ast = NULL;
        return;
    }

    printf("DEBUG: Current token type at statement start: %d\n", parser->current.type);

    // Initialize ast to NULL in case of error
    *ast = NULL;

    if (match(parser, TOKEN_PRINT)) {
        printf("DEBUG: Parsing print statement\n");
        consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'print'.");
        ASTNode* expr;
        expression(parser, &expr);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after print value.");
        consumeStatementEnd(parser);
        *ast = createPrintNode(expr);
        printf("DEBUG: Created AST_PRINT node\n");
    } else if (match(parser, TOKEN_IF)) {
        printf("DEBUG: Parsing if statement\n");
        ifStatement(parser, ast);
        // No need to consume statement end here as blocks handle their own termination
    } else if (match(parser, TOKEN_LEFT_BRACE)) {
        // Rewind to the left brace so block() can consume it
        parser->current = parser->previous;
        block(parser, ast);
        // No need to consume statement end here as blocks handle their own termination
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
    } else if (check(parser, TOKEN_IDENTIFIER)) {
        // This could be a variable assignment or a standalone expression
        Token name = parser->current;
        advance(parser);

        if (match(parser, TOKEN_EQUAL)) {
            // Variable assignment: identifier = expression
            printf("DEBUG: Parsing assignment statement\n");
            ASTNode* value;
            expression(parser, &value);
            consumeStatementEnd(parser);
            *ast = createAssignmentNode(name, value);
            printf("DEBUG: Created AST_ASSIGNMENT node\n");
        } else {
            // This is a standalone expression starting with an identifier
            // Rewind the parser to the identifier
            parser->current = name;
            ASTNode* expr;
            expression(parser, &expr);
            consumeStatementEnd(parser);
            *ast = createPrintNode(expr);  // Wrap standalone expressions in print
        }
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
    // Comparison operators
    [TOKEN_LESS] = {NULL, parseBinary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, parseBinary, PREC_COMPARISON},
    [TOKEN_GREATER] = {NULL, parseBinary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, parseBinary, PREC_COMPARISON},
    [TOKEN_EQUAL_EQUAL] = {NULL, parseBinary, PREC_EQUALITY},
    [TOKEN_BANG_EQUAL] = {NULL, parseBinary, PREC_EQUALITY},
    // Add other tokens as needed
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NEWLINE] = {NULL, NULL, PREC_NONE}, // Add explicit rule for newlines
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
    printf("DEBUG: Starting parse function\n");
    Scanner scanner;
    printf("DEBUG: Scanner declared\n");
    init_scanner(source);
    printf("DEBUG: Scanner initialized\n");
    Parser parser;
    printf("DEBUG: Parser declared\n");
    initParser(&parser, &scanner);
    printf("DEBUG: Parser initialized\n");
    printf("DEBUG: About to advance parser\n");
    advance(&parser);
    printf("DEBUG: Parser advanced\n");

    *ast = NULL;
    ASTNode* current = NULL;
    printf("DEBUG: Entering main parse loop\n");

    // Skip any leading newlines
    while (check(&parser, TOKEN_NEWLINE)) {
        printf("DEBUG: Skipping leading newline in parse\n");
        advance(&parser);
    }

    while (!check(&parser, TOKEN_EOF)) {
        printf("DEBUG: Processing statement\n");
        ASTNode* stmt = NULL;
        statement(&parser, &stmt);

        // If statement returned NULL (e.g., for EOF), break the loop
        if (stmt == NULL) {
            printf("DEBUG: Statement returned NULL, breaking loop\n");
            break;
        }

        printf("DEBUG: Statement processed\n");
        if (parser.hadError) {
            printf("DEBUG: Parser had error\n");
            if (*ast) {
                printf("DEBUG: Freeing AST\n");
                freeASTNode(*ast);
                *ast = NULL; // Set to NULL after freeing
            }
            if (stmt) {
                printf("DEBUG: Freeing statement\n");
                freeASTNode(stmt);
                stmt = NULL; // Set to NULL after freeing
            }
            printf("DEBUG: Synchronizing parser\n");
            synchronize(&parser);
            if (parser.hadError) {
                printf("DEBUG: Parser still has error after synchronization\n");
                return false;
            }
            printf("DEBUG: Continuing after synchronization\n");
            continue;
        }
        if (!*ast) {
            printf("DEBUG: Setting root AST node\n");
            *ast = stmt;
        } else {
            printf("DEBUG: Adding statement to AST\n");
            current->next = stmt;
        }
        current = stmt;
    }
    printf("DEBUG: Parse loop completed\n");
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

static void ifStatement(Parser* parser, ASTNode** ast) {
    // Parse an if statement
    printf("DEBUG: Parsing if statement\n");

    // Parse the condition
    ASTNode* condition;
    expression(parser, &condition);

    // Parse the then branch
    ASTNode* thenBranch;
    block(parser, &thenBranch);

    // Parse elif branches if any
    ASTNode* elifConditions = NULL;
    ASTNode* elifBranches = NULL;
    ASTNode* currentElifCondition = NULL;
    ASTNode* currentElifBranch = NULL;

    while (match(parser, TOKEN_ELIF)) {
        // Parse elif condition
        ASTNode* elifCondition;
        expression(parser, &elifCondition);

        // Add to elif conditions list
        if (elifConditions == NULL) {
            elifConditions = elifCondition;
            currentElifCondition = elifCondition;
        } else {
            currentElifCondition->next = elifCondition;
            currentElifCondition = elifCondition;
        }

        // Parse elif branch
        ASTNode* elifBranch;
        block(parser, &elifBranch);

        // Add to elif branches list
        if (elifBranches == NULL) {
            elifBranches = elifBranch;
            currentElifBranch = elifBranch;
        } else {
            currentElifBranch->next = elifBranch;
            currentElifBranch = elifBranch;
        }
    }

    // Parse else branch if any
    ASTNode* elseBranch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        block(parser, &elseBranch);
    }

    *ast = createIfNode(condition, thenBranch, elifConditions, elifBranches, elseBranch);
    printf("DEBUG: Created AST_IF node\n");
}

static void block(Parser* parser, ASTNode** ast) {
    // Parse a block of statements enclosed in { }
    printf("DEBUG: Parsing block\n");
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before block.");

    // Skip any newlines after the opening brace
    while (check(parser, TOKEN_NEWLINE)) {
        advance(parser);
    }

    // Parse statements until we reach the closing brace
    ASTNode* statements = NULL;
    ASTNode* current = NULL;

    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        ASTNode* stmt = NULL;
        statement(parser, &stmt);

        if (stmt == NULL) {
            // Skip empty statements (e.g., extra newlines)
            continue;
        }

        if (statements == NULL) {
            statements = stmt;
        } else {
            current->next = stmt;
        }
        current = stmt;

        // Skip any newlines between statements
        while (check(parser, TOKEN_NEWLINE)) {
            advance(parser);
        }
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");

    *ast = createBlockNode(statements);
    printf("DEBUG: Created AST_BLOCK node\n");
}

static void synchronize(Parser* parser) {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_NEWLINE) return;

        switch (parser->current.type) {
            case TOKEN_LET:
            case TOKEN_FN:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                advance(parser);
        }
    }
}