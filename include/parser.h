#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "ast.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // not -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Scanner* scanner;
    int functionDepth; // Track nested function declarations
    Type* currentImplType; // Track struct type for methods
    ObjString** genericParams;
    int genericCount;
    int genericCapacity;
} Parser;

typedef ASTNode* (*ParseFn)(Parser*);

typedef struct {
    ParseFn prefix;
    ASTNode* (*infix)(Parser*, ASTNode* left);
    Precedence precedence;
} ParseRule;

void initParser(Parser* parser, Scanner* scanner);
bool parse(const char* source, ASTNode** ast);
ParseRule* get_rule(TokenType type);

#endif