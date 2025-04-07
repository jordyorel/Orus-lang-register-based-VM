#include <stdlib.h>
#include <stdio.h>

#include "ast.h"
#include "memory.h"

ASTNode* createLiteralNode(Value value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LITERAL;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.literal = value;
    node->valueType = NULL;  // Set by type checker
    return node;
}

ASTNode* createBinaryNode(Token operator, ASTNode* left, ASTNode* right) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_BINARY;
    node->left = left;
    node->right = right;
    node->next = NULL;
    node->data.operation.operator = operator;
    node->data.operation.arity = 2;
    node->data.operation.convertLeft = false;
    node->data.operation.convertRight = false;
    node->valueType = NULL;
    return node;
}

ASTNode* createUnaryNode(Token operator, ASTNode * operand) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_UNARY;
    node->left = operand;
    node->right = NULL;
    node->next = NULL;
    node->data.operation.operator = operator;
    node->data.operation.arity = 1;
    node->data.operation.convertLeft = false;
    node->data.operation.convertRight = false;
    node->valueType = NULL;
    return node;
}

ASTNode* createVariableNode(Token name, uint8_t index) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_VARIABLE;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.variable.name = name;
    node->data.variable.index = index;
    node->valueType = NULL;
    return node;
}

ASTNode* createLetNode(Token name, Type* type, ASTNode* initializer) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LET;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.let.name = name;
    node->data.let.type = type;
    node->data.let.initializer = initializer;
    node->data.let.index = 0;
    node->valueType = NULL;
    return node;
}

ASTNode* createPrintNode(ASTNode* expr) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_PRINT;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.print.expr = expr;
    node->valueType = NULL;
    return node;
}

ASTNode* createAssignmentNode(Token name, ASTNode* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ASSIGNMENT;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.variable.name = name;
    node->data.variable.index = 0; // Will be resolved during compilation
    node->valueType = NULL;
    // Store the value expression in the left child
    node->left = value;
    return node;
}

void freeASTNode(ASTNode* node) {
    if (node == NULL) return;
    if (node->left) freeASTNode(node->left);
    if (node->right) freeASTNode(node->right);
    if (node->type == AST_LET && node->data.let.initializer)
        freeASTNode(node->data.let.initializer);
    if (node->type == AST_PRINT && node->data.print.expr)
        freeASTNode(node->data.print.expr);
    // Assignment nodes store their value in the left child, which is already freed above
    if (node->next) freeASTNode(node->next);
    free(node);
}

// void freeASTNode(ASTNode* node) {
//     if (node == NULL) return;

//     if (node->left) {
//         freeASTNode(node->left);
//     }
//     if (node->right) {
//         freeASTNode(node->right);
//     }

//     if (node->type == AST_LET && node->data.let.initializer) {
//         freeASTNode(node->data.let.initializer);
//     }
//     if (node->type == AST_PRINT && node->data.print.expr) {
//         freeASTNode(node->data.print.expr);
//     }

//     if (node->next) {
//         freeASTNode(node->next);
//     }

//     // Do NOT free node->valueType; it's managed by type.c
//     free(node);
// }