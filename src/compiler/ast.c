#include <stdlib.h>
#include <stdio.h>

#include "../../include/ast.h"
#include "../../include/memory.h"

ASTNode* createLiteralNode(Value value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LITERAL;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.literal = value;
    node->valueType = NULL;  // Set by type checker

    // Comment out debug print
    // fprintf(stderr, "DEBUG: Initializing AST_LITERAL node with value: ");
    // printValue(node->data.literal);
    // fprintf(stderr, "\n");

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

ASTNode* createPrintNode(ASTNode* format, ASTNode* arguments, int argCount) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_PRINT;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.print.format = format;
    node->data.print.arguments = arguments;
    node->data.print.argCount = argCount;
    node->valueType = NULL;
    return node;
}

ASTNode* createIfNode(ASTNode* condition, ASTNode* thenBranch, ASTNode* elifConditions, ASTNode* elifBranches, ASTNode* elseBranch) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.thenBranch = thenBranch;
    node->data.ifStmt.elifConditions = elifConditions;
    node->data.ifStmt.elifBranches = elifBranches;
    node->data.ifStmt.elseBranch = elseBranch;
    node->valueType = NULL;
    return node;
}

ASTNode* createBlockNode(ASTNode* statements) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.block.statements = statements;
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

ASTNode* createWhileNode(ASTNode* condition, ASTNode* body) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.body = body;
    node->valueType = NULL;
    return node;
}

ASTNode* createForNode(Token iteratorName, ASTNode* startExpr, ASTNode* endExpr, ASTNode* stepExpr, ASTNode* body) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FOR;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.forStmt.iteratorName = iteratorName;
    node->data.forStmt.iteratorIndex = 0; // Will be resolved during compilation
    node->data.forStmt.startExpr = startExpr;
    node->data.forStmt.endExpr = endExpr;
    node->data.forStmt.stepExpr = stepExpr;
    node->data.forStmt.body = body;
    node->valueType = NULL;
    return node;
}

ASTNode* createFunctionNode(Token name, ASTNode* parameters, Type* returnType, ASTNode* body) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FUNCTION;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.function.name = name;
    node->data.function.parameters = parameters;
    node->data.function.returnType = returnType;
    node->data.function.body = body;
    node->data.function.index = 0; // Will be resolved during compilation
    node->data.function.isMethod = false;
    node->data.function.implType = NULL;
    node->valueType = NULL;
    return node;
}

ASTNode* createCallNode(Token name, ASTNode* arguments, int argCount) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_CALL;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.call.name = name;
    node->data.call.arguments = arguments;
    node->data.call.index = 0; // Will be resolved during compilation
    node->data.call.convertArgs = NULL; // Will be allocated during type checking
    node->data.call.argCount = argCount;
    node->valueType = NULL;
    return node;
}

ASTNode* createReturnNode(ASTNode* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.returnStmt.value = value;
    node->valueType = NULL;
    return node;
}

ASTNode* createArrayNode(ASTNode* elements, int elementCount) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ARRAY;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.array.elements = elements;
    node->data.array.elementCount = elementCount;
    node->valueType = NULL;
    return node;
}

ASTNode* createArraySetNode(ASTNode* array, ASTNode* index, ASTNode* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ARRAY_SET;
    node->left = value;
    node->right = array;
    node->next = NULL;
    node->data.arraySet.index = index;
    node->valueType = NULL;
    return node;
}

ASTNode* createStructLiteralNode(Token name, ASTNode* values, int fieldCount) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_STRUCT_LITERAL;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->data.structLiteral.name = name;
    node->data.structLiteral.values = values;
    node->data.structLiteral.fieldCount = fieldCount;
    node->valueType = NULL;
    return node;
}

ASTNode* createFieldAccessNode(ASTNode* object, Token name) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FIELD;
    node->left = object;
    node->right = NULL;
    node->next = NULL;
    node->data.field.fieldName = name;
    node->data.field.index = -1;
    node->valueType = NULL;
    return node;
}

ASTNode* createFieldSetNode(ASTNode* object, Token name, ASTNode* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FIELD_SET;
    node->left = value;
    node->right = object;
    node->next = NULL;
    node->data.fieldSet.fieldName = name;
    node->data.fieldSet.index = -1;
    node->valueType = NULL;
    return node;
}

ASTNode* createBreakNode() {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_BREAK;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->valueType = NULL;
    return node;
}

ASTNode* createContinueNode() {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_CONTINUE;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->valueType = NULL;
    return node;
}

void freeASTNode(ASTNode* node) {
    if (node == NULL) return;
    if (node->left) freeASTNode(node->left);
    if (node->right) freeASTNode(node->right);
    if (node->type == AST_LET && node->data.let.initializer)
        freeASTNode(node->data.let.initializer);
    if (node->type == AST_PRINT) {
        if (node->data.print.format) freeASTNode(node->data.print.format);
        if (node->data.print.arguments) freeASTNode(node->data.print.arguments);
    }
    // Assignment nodes store their value in the left child, which is already freed above
    if (node->type == AST_IF) {
        if (node->data.ifStmt.condition) freeASTNode(node->data.ifStmt.condition);
        if (node->data.ifStmt.thenBranch) freeASTNode(node->data.ifStmt.thenBranch);
        if (node->data.ifStmt.elifConditions) freeASTNode(node->data.ifStmt.elifConditions);
        if (node->data.ifStmt.elifBranches) freeASTNode(node->data.ifStmt.elifBranches);
        if (node->data.ifStmt.elseBranch) freeASTNode(node->data.ifStmt.elseBranch);
    }
    if (node->type == AST_BLOCK && node->data.block.statements)
        freeASTNode(node->data.block.statements);
    if (node->type == AST_WHILE) {
        if (node->data.whileStmt.condition) freeASTNode(node->data.whileStmt.condition);
        if (node->data.whileStmt.body) freeASTNode(node->data.whileStmt.body);
    }
    if (node->type == AST_FOR) {
        if (node->data.forStmt.startExpr) freeASTNode(node->data.forStmt.startExpr);
        if (node->data.forStmt.endExpr) freeASTNode(node->data.forStmt.endExpr);
        if (node->data.forStmt.stepExpr) freeASTNode(node->data.forStmt.stepExpr);
        if (node->data.forStmt.body) freeASTNode(node->data.forStmt.body);
    }
    if (node->type == AST_FUNCTION) {
        if (node->data.function.parameters) freeASTNode(node->data.function.parameters);
        if (node->data.function.body) freeASTNode(node->data.function.body);
    }
    if (node->type == AST_ARRAY) {
        if (node->data.array.elements) freeASTNode(node->data.array.elements);
    }
    if (node->type == AST_STRUCT_LITERAL) {
        if (node->data.structLiteral.values)
            freeASTNode(node->data.structLiteral.values);
    }
    if (node->type == AST_ARRAY_SET) {
        if (node->data.arraySet.index) freeASTNode(node->data.arraySet.index);
    }
    if (node->type == AST_FIELD || node->type == AST_FIELD_SET) {
        // children already freed via left/right
    }
    if (node->type == AST_CALL) {
        if (node->data.call.arguments) freeASTNode(node->data.call.arguments);
        if (node->data.call.convertArgs) free(node->data.call.convertArgs);
    }
    if (node->type == AST_RETURN) {
        if (node->data.returnStmt.value) freeASTNode(node->data.returnStmt.value);
    }
    if (node->type == AST_LITERAL && node->data.literal.type == VAL_STRING) {
        free(node->data.literal.as.string.chars);
        node->data.literal.as.string.chars = NULL;
        node->data.literal.as.string.length = 0;
    }
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