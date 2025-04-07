#ifndef clox_ast_h
#define clox_ast_h

#include "common.h"
#include "scanner.h"
#include "type.h"
#include "value.h"

typedef enum {
    AST_LITERAL,
    AST_BINARY,
    AST_UNARY,
    AST_VARIABLE,
    AST_ASSIGNMENT,
    AST_CALL,
    AST_LET,
    AST_PRINT
} ASTNodeType;

typedef struct {
    Token name;
    uint8_t index;
} VariableData;

typedef struct {
    Token name;
    Type* type;
    struct ASTNode* initializer;
    uint8_t index;
} LetData;

typedef struct {
    struct ASTNode* expr;
} PrintData;

typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* next;
    union {
        Value literal;
        struct {
            Token operator;
            int arity;
            bool convertLeft;   // Flag to indicate if left operand needs conversion
            bool convertRight;  // Flag to indicate if right operand needs conversion
        } operation;
        VariableData variable;
        LetData let;
        PrintData print;
    } data;
    Type* valueType;
} ASTNode;

ASTNode* createLiteralNode(Value value);
ASTNode* createBinaryNode(Token operator, ASTNode * left, ASTNode* right);
ASTNode* createUnaryNode(Token operator, ASTNode * operand);
ASTNode* createVariableNode(Token name, uint8_t index);
ASTNode* createLetNode(Token name, Type* type, ASTNode* initializer);
ASTNode* createPrintNode(ASTNode* expr);  // Ensure this is declared
ASTNode* createAssignmentNode(Token name, ASTNode* value);

void freeASTNode(ASTNode* node);

#endif