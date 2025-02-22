#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "value.h"
#include "vm.h"

extern VM vm;

static void error(Compiler* compiler, const char* message) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;
    fprintf(stderr, "Compiler Error: %s\n", message);
    compiler->hadError = true;
}

static void writeOp(Compiler* compiler, uint8_t op) {
    writeChunk(compiler->chunk, op, 0);  // Line number could be stored in AST
}

static void emitConstant(Compiler* compiler, Value value) {
    writeConstant(compiler->chunk, value, 0);
}

static void typeCheckNode(Compiler* compiler, ASTNode* node) {
    if (!node) {
        printf("DEBUG: Skipping type check for null node\n");
        return;
    }
    
    printf("DEBUG: Type checking node type %d\n", node->type);

    switch (node->type) {
        case AST_LITERAL: {
            if (!node->valueType) {
                error(compiler, "Literal node has no type set.");
            }
            break;
        }

        case AST_BINARY: {
            printf("DEBUG: Checking binary node\n");

            typeCheckNode(compiler, node->left);
            typeCheckNode(compiler, node->right);
            if (compiler->hadError) return;

            Type* leftType = node->left->valueType;
            Type* rightType = node->right->valueType;
            if (!leftType || !rightType) {
                error(compiler, "Binary operand type not set.");
                return;
            }

            printf("DEBUG: Left type: %s, Right type: %s\n",
                   getTypeName(leftType->kind), getTypeName(rightType->kind));

            TokenType operator= node->data.operation.operator.type;
            switch (operator) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_STAR: {
                    if (leftType->kind == TYPE_F64 ||
                        rightType->kind == TYPE_F64) {
                        node->valueType = getPrimitiveType(
                            TYPE_F64);  // Use existing primitive type
                    } else if (typesEqual(leftType, rightType)) {
                        node->valueType = leftType;
                    } else {
                        error(compiler,
                              "Type mismatch in arithmetic operation.");
                        return;
                    }
                    break;
                }

                case TOKEN_MODULO: {
                    if (leftType->kind != TYPE_I32 &&
                        leftType->kind != TYPE_U32) {
                        error(compiler,
                              "Left operand of modulo must be an integer.");
                        return;
                    }
                    if (rightType->kind != TYPE_I32 &&
                        rightType->kind != TYPE_U32) {
                        error(compiler,
                              "Right operand of modulo must be an integer.");
                        return;
                    }
                    node->valueType =
                        leftType;  // Result matches left operand type
                    break;
                }

                default:
                    error(compiler,
                          "Unsupported binary operator in type checker.");
                    return;
            }
            break;
        }

        case AST_UNARY: {
            printf("DEBUG: Checking unary node\n");
            typeCheckNode(compiler, node->left);
            if (compiler->hadError) return;

            Type* operandType = node->left->valueType;
            if (!operandType) {
                error(compiler, "Unary operand type not set.");
                return;
            }

            TokenType operator= node->data.operation.operator.type;
            switch (operator) {
                case TOKEN_MINUS:
                    if (operandType->kind != TYPE_I32 &&
                        operandType->kind != TYPE_U32 &&
                        operandType->kind != TYPE_F64) {
                        error(compiler,
                              "Unary minus operand must be a number.");
                        return;
                    }
                    node->valueType = operandType;
                    break;

                default:
                    error(compiler, "Unsupported unary operator.");
                    return;
            }
            break;
        }

        case AST_VARIABLE: {
            printf("DEBUG: Checking variable node\n");
            uint8_t index = resolveVariable(compiler, node->data.variable.name);
            if (index == UINT8_MAX) {
                error(compiler, "Undefined variable.");
                return;
            }
            node->data.variable.index = index;
            node->valueType = variableTypes[index];
            if (!node->valueType) {
                error(compiler, "Variable has no type defined.");
                return;
            }
            break;
        }

        case AST_LET: {
            printf("DEBUG: Type checking let node\n");
            // First type check the initializer
            if (node->data.let.initializer) {
                typeCheckNode(compiler, node->data.let.initializer);
                if (compiler->hadError) return;
            } else {
                error(compiler, "Let statement requires an initializer");
                return;
            }

            Type* initType = node->data.let.initializer->valueType;
            Type* declType = node->data.let.type;

            if (!initType) {
                error(compiler, "Could not determine initializer type");
                return;
            }

            if (declType) {
                // Allow i32 literals to be used for u32 variables if the value is non-negative
                if (declType->kind == TYPE_U32 && initType->kind == TYPE_I32) {
                    if (IS_I32(node->data.let.initializer->data.literal) && 
                        AS_I32(node->data.let.initializer->data.literal) >= 0) {
                        // Convert the literal to u32
                        int32_t value = AS_I32(node->data.let.initializer->data.literal);
                        node->data.let.initializer->data.literal = U32_VAL((uint32_t)value);
                        node->data.let.initializer->valueType = declType;
                        initType = declType;
                    }
                }
                // Allow i32/u32 literals to be used for f64 variables
                else if (declType->kind == TYPE_F64 && 
                        (initType->kind == TYPE_I32 || initType->kind == TYPE_U32)) {
                    if (IS_I32(node->data.let.initializer->data.literal)) {
                        int32_t value = AS_I32(node->data.let.initializer->data.literal);
                        node->data.let.initializer->data.literal = F64_VAL((double)value);
                        node->data.let.initializer->valueType = declType;
                        initType = declType;
                    } else if (IS_U32(node->data.let.initializer->data.literal)) {
                        uint32_t value = AS_U32(node->data.let.initializer->data.literal);
                        node->data.let.initializer->data.literal = F64_VAL((double)value);
                        node->data.let.initializer->valueType = declType;
                        initType = declType;
                    }
                }
                if (!typesEqual(declType, initType)) {
                    error(compiler, "Type mismatch in let declaration.");
                    return;
                }
            }

            // Add the variable to the symbol table
            uint8_t index = addLocal(compiler, node->data.let.name, initType);
            node->data.let.index = index;

            // Set the node's type to the initializer type
            node->valueType = initType;
            break;
        }

        case AST_PRINT: {
            printf("DEBUG: Checking print node\n");
            typeCheckNode(compiler, node->data.print.expr);
            if (compiler->hadError) return;
            break;
        }

        default:
            error(compiler, "Unsupported AST node type in type checker.");
            break;
    }
}

static void generateCode(Compiler* compiler, ASTNode* node) {
    if (!node || compiler->hadError) {
        printf("DEBUG: Skipping code generation due to null node or error\n");
        return;
    }

    printf("DEBUG: Generating code for node type %d\n", node->type);

    switch (node->type) {
        case AST_LITERAL: {
            printf("DEBUG: Generating literal value\n");
            emitConstant(compiler, node->data.literal);
            break;
        }

        case AST_BINARY: {
            printf("DEBUG: Generating binary operation\n");

            // Generate code for operands
            generateCode(compiler, node->left);
            generateCode(compiler, node->right);
            if (compiler->hadError) return;

            // Get operand and result types
            Type* leftType = node->left->valueType;
            Type* rightType = node->right->valueType;
            TypeKind resultType = node->valueType->kind;

            // Convert right operand to result type if needed
            if (rightType->kind != resultType) {
                printf("DEBUG: Converting right operand from %s to %s\n",
                       getTypeName(rightType->kind), getTypeName(resultType));
                switch (resultType) {
                    case TYPE_F64:
                        if (rightType->kind == TYPE_I32)
                            writeOp(compiler, OP_I32_TO_F64);
                        else if (rightType->kind == TYPE_U32)
                            writeOp(compiler, OP_U32_TO_F64);
                        else {
                            error(compiler,
                                  "Unsupported right operand conversion for "
                                  "binary operation.");
                            return;
                        }
                        break;
                    default:
                        error(compiler,
                              "Unsupported result type for binary operation.");
                        return;
                }
            }

            // Convert left operand to result type if needed
            if (leftType->kind != resultType) {
                printf("DEBUG: Converting left operand from %s to %s\n",
                       getTypeName(leftType->kind), getTypeName(resultType));
                switch (resultType) {
                    case TYPE_F64:
                        if (leftType->kind == TYPE_I32)
                            writeOp(compiler, OP_I32_TO_F64);
                        else if (leftType->kind == TYPE_U32)
                            writeOp(compiler, OP_U32_TO_F64);
                        else {
                            error(compiler,
                                  "Unsupported left operand conversion for "
                                  "binary operation.");
                            return;
                        }
                        break;
                    default:
                        error(compiler,
                              "Unsupported result type for binary operation.");
                        return;
                }
            }

            // Emit the operator instruction
            TokenType operator= node->data.operation.operator.type;
            printf("DEBUG: Emitting operator %d for result type %s\n", operator,
                   getTypeName(resultType));
            switch (operator) {
                case TOKEN_PLUS:
                    switch (resultType) {
                        case TYPE_I32:
                            writeOp(compiler, OP_ADD_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_ADD_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_ADD_F64);
                            break;
                        default:
                            error(compiler,
                                  "Addition not supported for this type.");
                            return;
                    }
                    break;

                case TOKEN_MINUS:
                    switch (resultType) {
                        case TYPE_I32:
                            writeOp(compiler, OP_SUBTRACT_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_SUBTRACT_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_SUBTRACT_F64);
                            break;
                        default:
                            error(compiler,
                                  "Subtraction not supported for this type.");
                            return;
                    }
                    break;

                case TOKEN_STAR:
                    switch (resultType) {
                        case TYPE_I32:
                            writeOp(compiler, OP_MULTIPLY_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_MULTIPLY_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_MULTIPLY_F64);
                            break;
                        default:
                            error(
                                compiler,
                                "Multiplication not supported for this type.");
                            return;
                    }
                    break;
                case TOKEN_SLASH:
                    switch (resultType) {
                        case TYPE_I32:
                            writeOp(compiler, OP_DIVIDE_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_DIVIDE_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_DIVIDE_F64);
                            break;
                        default:
                            error(compiler,
                                  "Division not supported for this type.");
                            return;
                    }
                    break;
                case TOKEN_MODULO:
                    switch (resultType) {
                        case TYPE_I32:
                            writeOp(compiler, OP_MODULO_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_MODULO_U32);
                            break;
                        default:
                            error(compiler,
                                  "Modulo not supported for this type.");
                            return;
                    }
                    break;
                default:
                    error(compiler, "Unsupported binary operator.");
                    return;
            }
            break;
        }

        case AST_UNARY: {
            printf("DEBUG: Generating unary operation\n");
            generateCode(compiler, node->left);
            if (compiler->hadError) return;

            TypeKind operandType = node->valueType->kind;
            TokenType operator= node->data.operation.operator.type;

            switch (operator) {
                case TOKEN_MINUS:
                    switch (operandType) {
                        case TYPE_I32:
                            writeOp(compiler, OP_NEGATE_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_NEGATE_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_NEGATE_F64);
                            break;
                        default:
                            error(compiler,
                                  "Negation not supported for this type.");
                            return;
                    }
                    break;
                default:
                    error(compiler, "Unsupported unary operator.");
                    return;
            }
            break;
        }

        case AST_VARIABLE: {
            printf("DEBUG: Generating variable access\n");
            writeOp(compiler, OP_GET_GLOBAL);
            writeOp(compiler, node->data.variable.index);
            break;
        }

        case AST_LET: {
            printf("DEBUG: Generating variable definition\n");
            generateCode(compiler, node->data.let.initializer);
            if (compiler->hadError) return;
            writeOp(compiler, OP_DEFINE_GLOBAL);
            writeOp(compiler, node->data.let.index);
            break;
        }

        case AST_PRINT: {
            printf("DEBUG: Generating print statement\n");
            generateCode(compiler, node->data.print.expr);
            if (compiler->hadError) return;
            writeOp(compiler, OP_PRINT);
            break;
        }

        default:
            error(compiler, "Unsupported AST node type in code generator.");
            break;
    }
}

uint8_t addLocal(Compiler* compiler, Token name, Type* type) {
    for (uint8_t i = 0; i < vm.variableCount; i++) {
        if (vm.variableNames[i].length == name.length &&
            memcmp(vm.variableNames[i].name, name.start, name.length) == 0) {
            error(compiler, "Variable already declared.");
            return i;
        }
    }
    if (vm.variableCount >= UINT8_COUNT) {
        error(compiler, "Too many variables.");
        return 0;
    }
    uint8_t index = vm.variableCount++;
    char* name_copy = copyString(name.start, name.length);
    if (name_copy == NULL) {
        error(compiler, "Memory allocation failed for variable name.");
        return 0;
    }
    vm.variableNames[index].name = name_copy;
    printf("DEBUG: Added variable '%.*s' at index %d, address %p\n",
           name.length, name.start, index, vm.variableNames[index].name);
    vm.variableNames[index].length = name.length;
    variableTypes[index] = type;  // Should be getPrimitiveType result
    vm.globalTypes[index] = type;
    vm.globals[index] = NIL_VAL;
    return index;
}

uint8_t resolveVariable(Compiler* compiler, Token name) {
    for (int i = 0; i < vm.variableCount; i++) {
        if (vm.variableNames[i].length == name.length &&
            memcmp(vm.variableNames[i].name, name.start, name.length) == 0) {
            return (uint8_t)i;
        }
    }
    return UINT8_MAX;  // Not found
}

void initCompiler(Compiler* compiler, Chunk* chunk) {
    compiler->symbols = NULL;  // Initialize later if needed
    compiler->chunk = chunk;
    compiler->hadError = false;
    compiler->panicMode = false;
}

bool compile(ASTNode* ast, Compiler* compiler) {
    initTypeSystem();
    ASTNode* current = ast;
    while (current) {
        typeCheckNode(compiler, current);
        if (!compiler->hadError) generateCode(compiler, current);
        current = current->next;
    }
    writeOp(compiler, OP_RETURN);
    return !compiler->hadError;
}