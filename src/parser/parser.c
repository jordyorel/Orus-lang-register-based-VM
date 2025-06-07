#include "../../include/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/type.h"

#include "../../include/common.h"
#include "../../include/memory.h"

// Forward declaration for parse_precedence
static ASTNode* parse_precedence(Parser* parser, Precedence precedence);

// Add these forward declarations at the top with the others
static ASTNode* parseString(Parser* parser);
static ASTNode* parseNumber(Parser* parser);
static ASTNode* parseGrouping(Parser* parser);
static ASTNode* parseUnary(Parser* parser);
static ASTNode* parseBinary(Parser* parser, ASTNode* left);
static ASTNode* parseLogical(Parser* parser, ASTNode* left);  // New logical operator function
static ASTNode* parseCall(Parser* parser, ASTNode* left);
static ASTNode* parseIndex(Parser* parser, ASTNode* left);
static ASTNode* parseDot(Parser* parser, ASTNode* left);
static ASTNode* parseStructLiteral(Parser* parser, Token structName,
                                   Type** genericArgs, int genericArgCount);
static Type* findStructTypeToken(Token token);
static ASTNode* parseBoolean(Parser* parser);
static ASTNode* parseVariable(Parser* parser);
static ASTNode* parseNil(Parser* parser);
static ASTNode* parseArray(Parser* parser);
static void structDeclaration(Parser* parser, ASTNode** ast);
static void implBlock(Parser* parser, ASTNode** ast);
static Type* parseType(Parser* parser);
static void expression(Parser* parser, ASTNode** ast);
static void statement(Parser* parser, ASTNode** ast);
static void ifStatement(Parser* parser, ASTNode** ast);
static void tryStatement(Parser* parser, ASTNode** ast);
static void functionDeclaration(Parser* parser, ASTNode** ast);
static void returnStatement(Parser* parser, ASTNode** ast);
static void importStatement(Parser* parser, ASTNode** ast);
static void block(Parser* parser, ASTNode** ast);
static void consumeStatementEnd(Parser* parser);
static void synchronize(Parser* parser);

static void printErrorContext(Token* token) {
    // Determine the start of the line containing the token
    const char* lineStart = token->start;
    while (lineStart > scanner.source && lineStart[-1] != '\n') {
        lineStart--;
    }

    // Determine the end of the line
    const char* lineEnd = token->start;
    while (*lineEnd != '\n' && *lineEnd != '\0') {
        lineEnd++;
    }

    int lineLength = (int)(lineEnd - lineStart);
    fprintf(stderr, "%.*s\n", lineLength, lineStart);

    // Print caret under the offending token
    int column = (int)(token->start - lineStart);
    for (int i = 0; i < column; i++) fputc(' ', stderr);
    int caretLen = token->length > 0 ? token->length : 1;
    for (int i = 0; i < caretLen; i++) fputc('^', stderr);
    fputc('\n', stderr);
}

static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) return;
    parser->panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (token->type != TOKEN_ERROR)
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    fprintf(stderr, ": %s\n", message);
    printErrorContext(token);
    parser->hadError = true;
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    for (;;) {
        Token token = scan_token();

        // Special handling for newline tokens
        if (token.type == TOKEN_NEWLINE) {
            parser->current = token;
            break;
        }

        if (token.type != TOKEN_ERROR) {
            parser->current = token;
            break;
        }

        errorAt(parser, &token, token.start);
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

    // Special handling for newlines to avoid potential issues
    if (type == TOKEN_NEWLINE) {
        parser->previous = parser->current;

        // Safely get the next token
        Token next = scan_token();
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
    // Comment out debug statement
    // fprintf(stderr, "DEBUG: parseString processing token: type=%d, '%.*s'\n",
    //         parser->previous.type, parser->previous.length, parser->previous.start);

    const char* start = parser->previous.start + 1;  // Skip opening quote
    int length = parser->previous.length - 2;        // Exclude both quotes

    char* buffer = (char*)malloc(length + 1);  // temporary buffer
    int outLen = 0;
    for (int i = 0; i < length; i++) {
        char c = start[i];
        if (c == '\\') {
            i++;
            if (i >= length) break;  // malformed but scanner should catch
            char next = start[i];
            switch (next) {
                case 'n': buffer[outLen++] = '\n'; break;
                case 't': buffer[outLen++] = '\t'; break;
                case '\\': buffer[outLen++] = '\\'; break;
                case '"': buffer[outLen++] = '"'; break;
                default: buffer[outLen++] = next; break;
            }
        } else {
            buffer[outLen++] = c;
        }
    }
    ObjString* string = allocateString(buffer, outLen);
    free(buffer);
    Value value = STRING_VAL(string);
    ASTNode* node = createLiteralNode(value);
    node->valueType = createPrimitiveType(TYPE_STRING);
    return node;
}

static ASTNode* parseNumber(Parser* parser) {
    // Comment out debug output
    // fprintf(stderr, "DEBUG: parseNumber processing token: type=%d, '%.*s'\n",
    //        parser->previous.type, parser->previous.length, parser->previous.start);

    const char* start = parser->previous.start;
    int length = parser->previous.length;
    char* endptr;
    bool isFloat = false;
    
    // Check if the number contains a decimal point
    for (int i = 0; i < length; i++) {
        if (start[i] == '.') {
            isFloat = true;
            break;
        }
    }
    
    // Create a null-terminated copy of the token text for conversion
    char* numStr = (char*)malloc(length + 1);
    memcpy(numStr, start, length);
    numStr[length] = '\0';
    
    ASTNode* node;
    if (isFloat) {
        double value = strtod(numStr, &endptr);
        // fprintf(stderr, "DEBUG: Created F64 literal with value: %f\n", value);
        node = createLiteralNode(F64_VAL(value));
        node->valueType = createPrimitiveType(TYPE_F64);
    } else {
        long value = strtol(numStr, &endptr, 10);
        if (value >= INT32_MIN && value <= INT32_MAX) {
            // fprintf(stderr, "DEBUG: Created I32 literal with value: %ld\n", value);
            node = createLiteralNode(I32_VAL((int32_t)value));
            node->valueType = createPrimitiveType(TYPE_I32);
        } else if (value >= 0 && value <= UINT32_MAX) {
            // fprintf(stderr, "DEBUG: Created U32 literal with value: %lu\n", (unsigned long)value);
            node = createLiteralNode(U32_VAL((uint32_t)value));
            node->valueType = createPrimitiveType(TYPE_U32);
        } else {
            double dvalue = strtod(numStr, &endptr);
            // fprintf(stderr, "DEBUG: Created F64 literal (from large int) with value: %f\n", dvalue);
            node = createLiteralNode(F64_VAL(dvalue));
            node->valueType = createPrimitiveType(TYPE_F64);
        }
    }
    
    free(numStr);
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

static ASTNode* parseLogical(Parser* parser, ASTNode* left) {
    // Similar to parseBinary, but creates a logical node instead
    Token operator= parser->previous;
    
    // Get the precedence rule for the current operator
    ParseRule* rule = get_rule(operator.type);
    
    // Parse the right operand with higher precedence to ensure proper nesting
    ASTNode* right = parse_precedence(parser, (Precedence)(rule->precedence + 1));
    
    // Create binary node - we're using the existing binary node structure 
    // since logical operations can be represented by binary operations
    return createBinaryNode(operator, left, right);
}

static ASTNode* parseCall(Parser* parser, ASTNode* left) {
    // Function calls are only valid for identifiers
    if (left->type != AST_VARIABLE) {
        error(parser, "Can only call functions.");
        return NULL;
    }

    // Get the function name and any generic arguments
    Token name = left->data.variable.name;
    Type** genArgs = left->data.variable.genericArgs;
    int genCount = left->data.variable.genericArgCount;
    left->data.variable.genericArgs = NULL;
    left->data.variable.genericArgCount = 0;

    // Parse arguments
    ASTNode* arguments = NULL;
    ASTNode* lastArg = NULL;
    int argCount = 0;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            ASTNode* arg;
            expression(parser, &arg);

            // Add to argument list
            if (arguments == NULL) {
                arguments = arg;
            } else {
                lastArg->next = arg;
            }
            lastArg = arg;
            argCount++;
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");



    return createCallNode(name, arguments, argCount, NULL, genArgs, genCount);
}

static ASTNode* parseIndex(Parser* parser, ASTNode* left) {
    Token bracket = parser->previous; // '[' token
    ASTNode* indexExpr = NULL;
    expression(parser, &indexExpr);
    consume(parser, TOKEN_RIGHT_BRACKET, "Expect ']' after index expression.");
    return createBinaryNode(bracket, left, indexExpr);
}

static ASTNode* parseDot(Parser* parser, ASTNode* left) {
    consume(parser, TOKEN_IDENTIFIER, "Expect property or method name after '.'.");
    Token name = parser->previous;

    if (match(parser, TOKEN_LEFT_PAREN)) {
        bool useReceiver = true;
        Type* staticType = NULL;
        if (left->type == AST_VARIABLE) {
            staticType = findStructTypeToken(left->data.variable.name);
            if (staticType) {
                useReceiver = false;
            }
        }

        ASTNode* arguments = NULL;
        ASTNode* lastArg = NULL;
        int argCount = 0;
        if (useReceiver) {
            arguments = left;
            left->next = NULL;
            lastArg = left;
            argCount = 1;
        } else {
        }

        if (!check(parser, TOKEN_RIGHT_PAREN)) {
            do {
                ASTNode* arg;
                expression(parser, &arg);
                if (arguments == NULL) {
                    arguments = arg;
                } else {
                    lastArg->next = arg;
                }
                lastArg = arg;
                argCount++;
            } while (match(parser, TOKEN_COMMA));
        }

        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

        return createCallNode(name, arguments, argCount, staticType, NULL, 0);
    }

    return createFieldAccessNode(left, name);
}

// Look ahead to determine if a `<` after an identifier starts a generic
// argument list or is simply the less-than operator. We scan forward until the
// matching `>` and then inspect the token that follows. If it is a `{` (struct
// literal) or `(` (generic function call) we treat the angle brackets as
// generic arguments.
static bool looksLikeGeneric() {
    Scanner backup = scanner;
    int depth = 1;
    while (depth > 0) {
        Token t = scan_token();
        if (t.type == TOKEN_EOF || t.type == TOKEN_NEWLINE) {
            scanner = backup;
            return false;
        }
        if (t.type == TOKEN_LESS) depth++;
        else if (t.type == TOKEN_GREATER) depth--;
    }
    Token after = scan_token();
    scanner = backup;
    return after.type == TOKEN_LEFT_BRACE || after.type == TOKEN_LEFT_PAREN;
}

static ASTNode* parseVariable(Parser* parser) {
    Token name = parser->previous;
    Type** genericArgs = NULL;
    int genericCount = 0;

    if (check(parser, TOKEN_LESS) && looksLikeGeneric()) {
        advance(parser); // consume '<'
        do {
            Type* argType = parseType(parser);
            if (parser->hadError) return NULL;
            genericArgs = realloc(genericArgs, sizeof(Type*) * (genericCount + 1));
            genericArgs[genericCount++] = argType;
        } while (match(parser, TOKEN_COMMA));
        consume(parser, TOKEN_GREATER, "Expect '>' after generic arguments.");
    }

    ASTNode* var = createVariableNode(name, 0);  // Index resolved later

    if (check(parser, TOKEN_LEFT_BRACE)) {
        if (findStructTypeToken(name)) {
            advance(parser); // consume '{'
            return parseStructLiteral(parser, name, genericArgs, genericCount);
        }
    }

    var->data.variable.genericArgs = genericArgs;
    var->data.variable.genericArgCount = genericCount;
    return var;
}

static ASTNode* parseNil(Parser* parser) {
    (void)parser;
    ASTNode* node = createLiteralNode(NIL_VAL);
    node->valueType = getPrimitiveType(TYPE_NIL);
    return node;
}

static ASTNode* parseArray(Parser* parser) {
    ASTNode* elements = NULL;
    ASTNode* last = NULL;
    int count = 0;

    if (!check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            ASTNode* value;
            expression(parser, &value);
            if (parser->hadError) return NULL;

            if (elements == NULL) {
                elements = value;
            } else {
                last->next = value;
            }
            last = value;
            count++;
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_BRACKET, "Expect ']' after array elements.");

    return createArrayNode(elements, count);
}

static Type* findStructTypeToken(Token token) {
    char name[token.length + 1];
    memcpy(name, token.start, token.length);
    name[token.length] = '\0';
    return findStructType(name);
}

static ASTNode* parseStructLiteral(Parser* parser, Token structName,
                                   Type** genericArgs, int genericArgCount) {


    ASTNode* values = NULL;
    ASTNode* last = NULL;
    int count = 0;

    if (!check(parser, TOKEN_RIGHT_BRACE)) {
        while (match(parser, TOKEN_NEWLINE)) {}
        do {
            while (match(parser, TOKEN_NEWLINE)) {}
            consume(parser, TOKEN_IDENTIFIER, "Expect field name.");
            consume(parser, TOKEN_COLON, "Expect ':' after field name.");
            ASTNode* value = NULL;
            expression(parser, &value);
            if (parser->hadError) return NULL;
            if (values == NULL)
                values = value;
            else
                last->next = value;
            last = value;
            count++;
            if (match(parser, TOKEN_COMMA) || match(parser, TOKEN_NEWLINE)) {
                while (match(parser, TOKEN_NEWLINE)) {}
            }
        } while (!check(parser, TOKEN_RIGHT_BRACE));
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after struct literal.");

    return createStructLiteralNode(structName, values, count, genericArgs, genericArgCount);
}

static ASTNode* parse_precedence(Parser* parser, Precedence precedence) {
    advance(parser);

    // Check for EOF
    if (check(parser, TOKEN_EOF)) {
        error(parser, "Unexpected end of file.");
        return NULL;
    }

    ParseFn prefixRule = get_rule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(parser, "Expected expression.");
        return NULL;
    }

    ASTNode* left = prefixRule(parser);
    if (left == NULL) {
        return NULL;
    }

    while (!parser->hadError &&
           precedence <= get_rule(parser->current.type)->precedence) {
        advance(parser);
        ASTNode* (*infixRule)(Parser*, ASTNode*) =
            get_rule(parser->previous.type)->infix;
        if (infixRule == NULL) {
            error(parser, "Invalid infix operator.");
            return NULL;
        }

        ASTNode* newLeft = infixRule(parser, left);
        if (newLeft == NULL) {
            return NULL;
        }
        left = newLeft;
    }

    return left;
}

static void expression(Parser* parser, ASTNode** ast) {
    *ast = NULL; // Initialize to NULL in case of error
    ASTNode* expr = parse_precedence(parser, PREC_ASSIGNMENT);
    if (expr == NULL) {
        return;
    }
    *ast = expr;
}

static void consumeStatementEnd(Parser* parser) {
    // Check for EOF first
    if (check(parser, TOKEN_EOF)) {
        return;
    }

    // Check for semicolon - this is now an error
    if (check(parser, TOKEN_SEMICOLON)) {
        error(parser, "Semicolons are not used in this language. Use newlines to terminate statements.");
        // Skip the semicolon to continue parsing
        match(parser, TOKEN_SEMICOLON);
        return;
    }

    // Check for newline
    if (check(parser, TOKEN_NEWLINE)) {
        match(parser, TOKEN_NEWLINE); // Use our safer match function

        // Consume any additional newlines safely
        int newlineCount = 0;
        while (check(parser, TOKEN_NEWLINE) && newlineCount < 10) { // Limit to avoid infinite loops
            newlineCount++;
            match(parser, TOKEN_NEWLINE); // Use our safer match function
        }
        return;
    }

    // Check for end of file
    if (check(parser, TOKEN_EOF)) {
        return;
    }

    // Check for right parenthesis (for function calls as statements)
    if (parser->previous.type == TOKEN_RIGHT_PAREN) {
        return;
    }

    error(parser, "Expect newline after statement.");
}

static void whileStatement(Parser* parser, ASTNode** ast) {
    // Parse a while statement

    // Parse the condition
    ASTNode* condition;
    expression(parser, &condition);

    // Parse the body
    ASTNode* body;
    block(parser, &body);

    *ast = createWhileNode(condition, body);
}

static void forStatement(Parser* parser, ASTNode** ast) {
    // Parse a for statement

    // Parse the iterator variable
    consume(parser, TOKEN_IDENTIFIER, "Expect iterator variable name.");
    Token iteratorName = parser->previous;

    // Parse the 'in' keyword
    consume(parser, TOKEN_IN, "Expect 'in' after iterator variable.");

    // Parse the range start expression
    ASTNode* startExpr;
    expression(parser, &startExpr);

    // Parse the range operator
    consume(parser, TOKEN_DOT_DOT, "Expect '..' in range expression.");

    // Parse the range end expression
    ASTNode* endExpr;
    expression(parser, &endExpr);

    // Check for optional step value
    ASTNode* stepExpr = NULL;
    if (match(parser, TOKEN_DOT_DOT)) {
        // Parse the step expression
        expression(parser, &stepExpr);
    } else {
        // Default step is 1
        stepExpr = createLiteralNode(I32_VAL(1));
        stepExpr->valueType = getPrimitiveType(TYPE_I32);
    }

    // Parse the body
    ASTNode* body;
    block(parser, &body);

    *ast = createForNode(iteratorName, startExpr, endExpr, stepExpr, body);
}

static void tryStatement(Parser* parser, ASTNode** ast) {
    ASTNode* tryBlock;
    block(parser, &tryBlock);

    consume(parser, TOKEN_CATCH, "Expect 'catch' after try block.");
    consume(parser, TOKEN_IDENTIFIER, "Expect identifier after 'catch'.");
    Token errName = parser->previous;

    ASTNode* catchBlock;
    block(parser, &catchBlock);

    *ast = createTryNode(tryBlock, errName, catchBlock);
}

static void functionDeclaration(Parser* parser, ASTNode** ast) {
    // Parse function name
    consume(parser, TOKEN_IDENTIFIER, "Expect function name.");
    Token name = parser->previous;

    bool hasSelf = false;

    // Generic parameters
    ObjString** generics = NULL;
    int genericCount = 0;
    int prevGenericCount = parser->genericCount;
    if (match(parser, TOKEN_LESS)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expect generic parameter name.");
            Token gnameTok = parser->previous;
            ObjString* gname = allocateString(gnameTok.start, gnameTok.length);
            if (parser->genericCount >= parser->genericCapacity) {
                parser->genericCapacity = parser->genericCapacity < 8 ? 8 : parser->genericCapacity * 2;
                parser->genericParams = realloc(parser->genericParams, sizeof(ObjString*) * parser->genericCapacity);
            }
            parser->genericParams[parser->genericCount++] = gname;
            generics = realloc(generics, sizeof(ObjString*) * (genericCount + 1));
            generics[genericCount++] = gname;
        } while (match(parser, TOKEN_COMMA));
        consume(parser, TOKEN_GREATER, "Expect '>' after generic parameters.");
    }

    // Enter function scope
    parser->functionDepth++;

    // Parse parameter list
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    // Parse parameters
    ASTNode* parameters = NULL;
    ASTNode* lastParam = NULL;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            // Parse parameter name
            consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
            Token paramName = parser->previous;

            Type* paramType = NULL;
            if (match(parser, TOKEN_COLON)) {
                paramType = parseType(parser);
                if (parser->hadError) return;
            } else if (paramName.length == 4 &&
                       strncmp(paramName.start, "self", 4) == 0 &&
                       parser->currentImplType != NULL) {
                paramType = parser->currentImplType;
                if (parameters == NULL) {
                    hasSelf = true;
                }
            } else {
                error(parser, "Expect ':' after parameter name.");
                return;
            }

            // Create parameter node (using let node for now)
            ASTNode* param = createLetNode(paramName, paramType, NULL);

            // Add to parameter list
            if (parameters == NULL) {
                parameters = param;
            } else {
                lastParam->next = param;
            }
            lastParam = param;
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    // Parse return type
    Type* returnType = NULL;
    if (match(parser, TOKEN_ARROW)) {
        returnType = parseType(parser);
        if (parser->hadError) return;
    } else {
        returnType = getPrimitiveType(TYPE_NIL);
    }

    if (hasSelf && parser->currentImplType != NULL) {
        const char* structName = parser->currentImplType->info.structure.name->chars;
        size_t structLen = strlen(structName);
        size_t funcLen = name.length;
        char* full = (char*)malloc(structLen + 1 + funcLen + 1);
        memcpy(full, structName, structLen);
        full[structLen] = '_';
        memcpy(full + structLen + 1, name.start, funcLen);
        full[structLen + 1 + funcLen] = '\0';
        name.start = full;
        name.length = structLen + 1 + funcLen;
    }

    // Parse function body
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' after function return type.");
    parser->current = parser->previous; // Rewind to the left brace so block() can consume it
    ASTNode* body;
    block(parser, &body);

    // Exit function scope
    parser->functionDepth--;
    parser->genericCount = prevGenericCount;

    ASTNode* fnNode = createFunctionNode(name, parameters, returnType, body,
                                         generics, genericCount);
    fnNode->data.function.isMethod = hasSelf;
    fnNode->data.function.implType = parser->currentImplType;
    *ast = fnNode;
}

static void returnStatement(Parser* parser, ASTNode** ast) {
    ASTNode* value = NULL;

    if (parser->functionDepth == 0) {
        error(parser, "'return' outside of function.");
    }

    // Check if there's a return value
    if (!check(parser, TOKEN_NEWLINE) && !check(parser, TOKEN_RIGHT_BRACE)) {
        expression(parser, &value);
    }

    consumeStatementEnd(parser);
    *ast = createReturnNode(value);
}

static void importStatement(Parser* parser, ASTNode** ast) {
    consume(parser, TOKEN_STRING, "Expect module path string after 'import'.");
    Token path = parser->previous;
    consumeStatementEnd(parser);
    *ast = createImportNode(path);
}

static void structDeclaration(Parser* parser, ASTNode** ast) {
    consume(parser, TOKEN_IDENTIFIER, "Expect struct name.");
    Token nameTok = parser->previous;

    ObjString** generics = NULL;
    int genericCount = 0;
    int prevGenericCount = parser->genericCount;
    if (match(parser, TOKEN_LESS)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expect generic parameter name.");
            Token gnameTok = parser->previous;
            ObjString* gname = allocateString(gnameTok.start, gnameTok.length);
            if (parser->genericCount >= parser->genericCapacity) {
                parser->genericCapacity = parser->genericCapacity < 8 ? 8 : parser->genericCapacity * 2;
                parser->genericParams = realloc(parser->genericParams, sizeof(ObjString*) * parser->genericCapacity);
            }
            parser->genericParams[parser->genericCount++] = gname;
            generics = realloc(generics, sizeof(ObjString*) * (genericCount + 1));
            generics[genericCount++] = gname;
        } while (match(parser, TOKEN_COMMA));
        consume(parser, TOKEN_GREATER, "Expect '>' after generic parameters.");
    }
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' after struct name.");

    ObjString* structName = allocateString(nameTok.start, nameTok.length);
    Type* structType = createStructType(structName, NULL, 0, generics, genericCount);

    FieldInfo* fields = NULL;
    int count = 0;
    int capacity = 0;

    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        // Skip any newlines between fields
        while (match(parser, TOKEN_NEWLINE)) {}
        consume(parser, TOKEN_IDENTIFIER, "Expect field name.");
        Token fieldNameTok = parser->previous;
        consume(parser, TOKEN_COLON, "Expect ':' after field name.");
        Type* fieldType = parseType(parser);
        if (parser->hadError) return;

        if (count == capacity) {
            capacity = capacity < 4 ? 4 : capacity * 2;
            fields = realloc(fields, sizeof(FieldInfo) * capacity);
        }
        ObjString* fname = allocateString(fieldNameTok.start, fieldNameTok.length);
        fields[count].name = fname;
        fields[count].type = fieldType;
        count++;

        if (match(parser, TOKEN_COMMA) || match(parser, TOKEN_NEWLINE)) {
            // Skip additional newlines after separator
            while (match(parser, TOKEN_NEWLINE)) {}
            if (check(parser, TOKEN_RIGHT_BRACE)) {
                break; // Allow trailing separator before closing brace
            }
        } else if (!check(parser, TOKEN_RIGHT_BRACE)) {
            consume(parser, TOKEN_COMMA, "Expect ',' between fields.");
        }
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after struct fields.");
    consumeStatementEnd(parser);

    structType->info.structure.fields = fields;
    structType->info.structure.fieldCount = count;
    parser->genericCount = prevGenericCount;

    // Create a dummy global variable for the struct name to allow method calls
    *ast = createLetNode(nameTok, structType, NULL);
}

static void implBlock(Parser* parser, ASTNode** ast) {
    consume(parser, TOKEN_IDENTIFIER, "Expect type name after impl.");
    Token structNameTok = parser->previous;
    Type* prevType = parser->currentImplType;
    parser->currentImplType = findStructTypeToken(structNameTok);

    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' after impl name.");
    ASTNode* methods = NULL;
    ASTNode* last = NULL;
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_FN)) {
            ASTNode* fn;
            functionDeclaration(parser, &fn);
            if (methods == NULL) {
                methods = fn;
            } else {
                last->next = fn;
            }
            last = fn;
        } else {
            advance(parser); // Skip unexpected tokens
        }
    }
    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after impl block.");
    consumeStatementEnd(parser);
    parser->currentImplType = prevType;
    // Methods defined in an impl block should remain in the outer scope
    *ast = createBlockNode(methods, false);
}

static void statement(Parser* parser, ASTNode** ast) {
    // Skip any leading newlines
    while (check(parser, TOKEN_NEWLINE)) {
        advance(parser);
    }

    if (check(parser, TOKEN_EOF)) {
        *ast = NULL;
        return;
    }

    *ast = NULL;  // Safe default

    if (match(parser, TOKEN_PRINT)) {
        if (parser->functionDepth == 0) {
            error(parser, "'print' outside of function.");
        }
        consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'print'.");

        // Parse format string or direct expression
        ASTNode* formatExpr;
        expression(parser, &formatExpr);

        // Handle both print formats
        if (match(parser, TOKEN_COMMA)) {
            // This is a formatted print with interpolation
            bool validFormat = formatExpr != NULL &&
                           formatExpr->valueType != NULL &&
                           formatExpr->valueType->kind == TYPE_STRING;

            if (!validFormat) {
                error(parser,
                  "First argument to print must evaluate to a string for "
                  "interpolation.");
                return;
            }

            // Parse comma-separated arguments
            ASTNode* arguments = NULL;
            ASTNode* lastArg = NULL;
            int argCount = 0;

            do {
                ASTNode* arg;
                expression(parser, &arg);

                if (arg == NULL) {
                    error(parser, "Expected expression as argument.");
                    return;
                }

                arg->next = NULL;  // 🔒 important!

                if (arguments == NULL) {
                    arguments = arg;
                } else {
                    lastArg->next = arg;
                }
                lastArg = arg;
                argCount++;
            } while (match(parser, TOKEN_COMMA));

            consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after print arguments.");
            consumeStatementEnd(parser);

            *ast = createPrintNode(formatExpr, arguments, argCount);
        } else {
            // This is a simple print without interpolation
            consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after print argument.");
            consumeStatementEnd(parser);
            
            // Create a print node with no additional arguments
            *ast = createPrintNode(formatExpr, NULL, 0);
        }
    } else if (match(parser, TOKEN_IF)) {
        ifStatement(parser, ast);

    } else if (match(parser, TOKEN_WHILE)) {
        whileStatement(parser, ast);

    } else if (match(parser, TOKEN_FOR)) {
        forStatement(parser, ast);

    } else if (match(parser, TOKEN_TRY)) {
        tryStatement(parser, ast);

    } else if (match(parser, TOKEN_STRUCT)) {
        structDeclaration(parser, ast);

    } else if (match(parser, TOKEN_IMPL)) {
        implBlock(parser, ast);

    } else if (match(parser, TOKEN_FN)) {
        functionDeclaration(parser, ast);

    } else if (match(parser, TOKEN_RETURN)) {
        returnStatement(parser, ast);

    } else if (match(parser, TOKEN_IMPORT)) {
        if (parser->functionDepth > 0) {
            error(parser, "'import' must be at top level.");
        }
        importStatement(parser, ast);

    } else if (match(parser, TOKEN_BREAK)) {
        consumeStatementEnd(parser);
        *ast = createBreakNode();

    } else if (match(parser, TOKEN_CONTINUE)) {
        consumeStatementEnd(parser);
        *ast = createContinueNode();

    } else if (match(parser, TOKEN_LEFT_BRACE)) {
        // Rewind to the left brace so block() can consume it
        parser->current = parser->previous;
        block(parser, ast);

    } else if (match(parser, TOKEN_LET)) {
        if (parser->functionDepth == 0) {
            error(parser, "'let' declarations must be inside a function.");
        }
        consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
        Token name = parser->previous;
        Type* type = NULL;

        if (match(parser, TOKEN_COLON)) {
            type = parseType(parser);
            if (parser->hadError) return;
        }

        consume(parser, TOKEN_EQUAL, "Expect '=' after variable name.");
        ASTNode* initializer;
        expression(parser, &initializer);
        consumeStatementEnd(parser);

        *ast = createLetNode(name, type, initializer);

    } else {
        ASTNode* expr;
        expression(parser, &expr);

        if (match(parser, TOKEN_EQUAL)) {
            ASTNode* value;
            expression(parser, &value);
            consumeStatementEnd(parser);

            if (expr->type == AST_VARIABLE) {
                *ast = createAssignmentNode(expr->data.variable.name, value);
                // The variable node itself is no longer needed
                expr->left = expr->right = NULL;
            } else if (expr->type == AST_BINARY &&
                       expr->data.operation.operator.type == TOKEN_LEFT_BRACKET) {
                ASTNode* arrayExpr = expr->left;
                ASTNode* indexExpr = expr->right;
                expr->left = expr->right = NULL;
                *ast = createArraySetNode(arrayExpr, indexExpr, value);
            } else if (expr->type == AST_FIELD) {
                ASTNode* object = expr->left;
                Token fieldName = expr->data.field.fieldName;
                expr->left = NULL;
                *ast = createFieldSetNode(object, fieldName, value);
            } else {
                error(parser, "Invalid assignment target.");
                *ast = NULL;
                return;
            }
        } else {
            consumeStatementEnd(parser);
            *ast = expr;
        }
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {parseGrouping, parseCall, PREC_CALL},
    [TOKEN_LEFT_BRACKET] = {parseArray, parseIndex, PREC_CALL},
    [TOKEN_DOT] = {NULL, parseDot, PREC_CALL},
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
    [TOKEN_NIL] = {parseNil, NULL, PREC_NONE},
    [TOKEN_NOT] = {parseUnary, NULL, PREC_UNARY},
    // Logical operators
    [TOKEN_AND] = {NULL, parseLogical, PREC_AND},
    [TOKEN_OR] = {NULL, parseLogical, PREC_OR},
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
    parser->functionDepth = 0;
    parser->currentImplType = NULL;
    parser->genericParams = NULL;
    parser->genericCount = 0;
    parser->genericCapacity = 0;
}

bool parse(const char* source, ASTNode** ast) {
    // fprintf(stderr, ">>> ENTERED PARSE FUNCTION <<<\n");
    Scanner scanner;
    init_scanner(source);
    Parser parser;
    initParser(&parser, &scanner);
    advance(&parser);

    *ast = NULL;
    ASTNode* current = NULL;

    // Skip any leading newlines
    while (check(&parser, TOKEN_NEWLINE)) {
        advance(&parser);
    }

    while (!check(&parser, TOKEN_EOF)) {
        ASTNode* stmt = NULL;
        statement(&parser, &stmt);

        // Some declarations (like structs) don't generate runtime nodes.
        // Simply skip adding them to the AST but continue parsing.
        if (stmt == NULL) {
            continue;
        }

        if (parser.hadError) {
            if (*ast) {
                *ast = NULL;
            }
            if (stmt) {
                stmt = NULL;
            }
            synchronize(&parser);
            if (parser.hadError) {
                return false;
            }
            continue;
        }
        if (!*ast) {
            *ast = stmt;
        } else {
            current->next = stmt;
        }
        current = stmt;
    }
    if (parser.genericParams) free(parser.genericParams);
    return !parser.hadError;
}

static Type* parseType(Parser* parser) {
    Type* type = NULL;
    if (match(parser, TOKEN_LEFT_PAREN)) {
        Type** params = NULL;
        int pcount = 0;
        if (!check(parser, TOKEN_RIGHT_PAREN)) {
            do {
                Type* pt = parseType(parser);
                if (parser->hadError) return NULL;
                params = realloc(params, sizeof(Type*) * (pcount + 1));
                params[pcount++] = pt;
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        consume(parser, TOKEN_ARROW, "Expect '->' after parameter types.");
        Type* ret = parseType(parser);
        if (parser->hadError) return NULL;
        type = createFunctionType(ret, params, pcount);
    } else if (match(parser, TOKEN_LEFT_BRACKET)) {
        Type* elementType = parseType(parser);
        if (parser->hadError) return NULL;
        if (match(parser, TOKEN_SEMICOLON)) {
            consume(parser, TOKEN_NUMBER, "Expect array size.");
        }
        consume(parser, TOKEN_RIGHT_BRACKET, "Expect ']' after array type.");
        type = createArrayType(elementType);
    } else if (match(parser, TOKEN_INT)) {
        type = getPrimitiveType(TYPE_I32);
    } else if (match(parser, TOKEN_U32)) {
        type = getPrimitiveType(TYPE_U32);
    } else if (match(parser, TOKEN_F64)) {
        type = getPrimitiveType(TYPE_F64);
    } else if (match(parser, TOKEN_BOOL)) {
        type = getPrimitiveType(TYPE_BOOL);
    } else if (check(parser, TOKEN_IDENTIFIER)) {
        Token ident = parser->current;
        advance(parser);
        char name[ident.length + 1];
        memcpy(name, ident.start, ident.length);
        name[ident.length] = '\0';

        if (strcmp(name, "string") == 0) {
            type = getPrimitiveType(TYPE_STRING);
        } else {
            for (int i = parser->genericCount - 1; i >= 0; i--) {
                ObjString* g = parser->genericParams[i];
                if (g->length == ident.length &&
                    strncmp(g->chars, ident.start, ident.length) == 0) {
                    type = createGenericType(g);
                    break;
                }
            }
            if (!type) {
                type = findStructType(name);
                if (!type) {
                    error(parser, "Unknown type name.");
                    return NULL;
                }
                if (match(parser, TOKEN_LESS)) {
                    Type** args = NULL;
                    int acount = 0;
                    do {
                        Type* at = parseType(parser);
                        if (parser->hadError) return NULL;
                        args = realloc(args, sizeof(Type*) * (acount + 1));
                        args[acount++] = at;
                    } while (match(parser, TOKEN_COMMA));
                    consume(parser, TOKEN_GREATER, "Expect '>' after generic arguments.");
                    type = instantiateStructType(type, args, acount);
                }
            }
        }
    } else {
        error(parser, "Expected type name (i32, u32, f64, bool, string or struct).");
        return NULL;
    }

    if (type == NULL) {
        error(parser, "Failed to get primitive type");
        return NULL;
    }

    return type;
}

static ASTNode* parseBoolean(Parser* parser) {
    bool value = parser->previous.type == TOKEN_TRUE;
    ASTNode* node = createLiteralNode(BOOL_VAL(value));
    node->valueType = getPrimitiveType(TYPE_BOOL);
    return node;
}

static void ifStatement(Parser* parser, ASTNode** ast) {
    // Parse an if statement

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
}

static void block(Parser* parser, ASTNode** ast) {
    // Parse a block of statements enclosed in { }
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

    *ast = createBlockNode(statements, true);
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