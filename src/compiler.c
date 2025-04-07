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
        return;
    }

    switch (node->type) {
        case AST_LITERAL: {
            if (!node->valueType) {
                error(compiler, "Literal node has no type set.");
            }
            break;
        }

        case AST_BINARY: {

            typeCheckNode(compiler, node->left);
            typeCheckNode(compiler, node->right);
            if (compiler->hadError) return;

            Type* leftType = node->left->valueType;
            Type* rightType = node->right->valueType;
            if (!leftType || !rightType) {
                error(compiler, "Binary operand type not set.");
                return;
            }



            TokenType operator= node->data.operation.operator.type;
            switch (operator) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_STAR:
                case TOKEN_SLASH: {
                    // If either operand is a float, the result is a float
                    if (leftType->kind == TYPE_F64 ||
                        rightType->kind == TYPE_F64) {
                        node->valueType = getPrimitiveType(TYPE_F64);

                        // Mark operands for conversion if needed
                        if (leftType->kind == TYPE_I32 || leftType->kind == TYPE_U32) {

                            node->data.operation.convertLeft = true;
                        } else {
                            node->data.operation.convertLeft = false;
                        }

                        if (rightType->kind == TYPE_I32 || rightType->kind == TYPE_U32) {

                            node->data.operation.convertRight = true;
                        } else {
                            node->data.operation.convertRight = false;
                        }
                    }
                    // If both operands are the same type, the result is that type
                    else if (typesEqual(leftType, rightType)) {
                        node->valueType = leftType;
                        node->data.operation.convertLeft = false;
                        node->data.operation.convertRight = false;
                    }
                    // Otherwise, it's a type mismatch
                    else {
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

                // Comparison operators
                case TOKEN_LESS:
                case TOKEN_LESS_EQUAL:
                case TOKEN_GREATER:
                case TOKEN_GREATER_EQUAL:
                case TOKEN_EQUAL_EQUAL:
                case TOKEN_BANG_EQUAL: {
                    // Comparison operators always return a boolean
                    node->valueType = getPrimitiveType(TYPE_BOOL);
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
            typeCheckNode(compiler, node->data.print.expr);
            if (compiler->hadError) return;
            break;
        }

        case AST_ASSIGNMENT: {
            // First type check the value expression
            if (node->left) {
                typeCheckNode(compiler, node->left);
                if (compiler->hadError) return;
            } else {
                error(compiler, "Assignment requires a value expression");
                return;
            }

            // Resolve the variable being assigned to
            uint8_t index = resolveVariable(compiler, node->data.variable.name);
            if (index == UINT8_MAX) {
                error(compiler, "Cannot assign to undefined variable.");
                return;
            }
            node->data.variable.index = index;

            // Check that the types are compatible
            Type* varType = variableTypes[index];
            Type* valueType = node->left->valueType;

            if (!varType) {
                error(compiler, "Variable has no type defined.");
                return;
            }

            if (!valueType) {
                error(compiler, "Could not determine value type.");
                return;
            }

            // Allow i32 literals to be used for u32 variables if the value is non-negative
            if (varType->kind == TYPE_U32 && valueType->kind == TYPE_I32 &&
                node->left->type == AST_LITERAL) {
                if (IS_I32(node->left->data.literal) &&
                    AS_I32(node->left->data.literal) >= 0) {
                    // Convert the literal to u32
                    int32_t value = AS_I32(node->left->data.literal);
                    node->left->data.literal = U32_VAL((uint32_t)value);
                    node->left->valueType = varType;
                    valueType = varType;
                }
            }
            // Allow i32/u32 literals to be used for f64 variables
            else if (varType->kind == TYPE_F64 &&
                    (valueType->kind == TYPE_I32 || valueType->kind == TYPE_U32) &&
                    node->left->type == AST_LITERAL) {
                if (IS_I32(node->left->data.literal)) {
                    int32_t value = AS_I32(node->left->data.literal);
                    node->left->data.literal = F64_VAL((double)value);
                    node->left->valueType = varType;
                    valueType = varType;
                } else if (IS_U32(node->left->data.literal)) {
                    uint32_t value = AS_U32(node->left->data.literal);
                    node->left->data.literal = F64_VAL((double)value);
                    node->left->valueType = varType;
                    valueType = varType;
                }
            }

            if (!typesEqual(varType, valueType)) {
                error(compiler, "Type mismatch in assignment.");
                return;
            }

            // The assignment expression has the same type as the variable
            node->valueType = varType;
            break;
        }

        case AST_IF: {
            // Type check the condition
            typeCheckNode(compiler, node->data.ifStmt.condition);
            if (compiler->hadError) return;

            // Ensure the condition is a boolean
            Type* condType = node->data.ifStmt.condition->valueType;
            if (!condType || condType->kind != TYPE_BOOL) {
                error(compiler, "If condition must be a boolean expression.");
                return;
            }

            // Type check the then branch
            typeCheckNode(compiler, node->data.ifStmt.thenBranch);
            if (compiler->hadError) return;

            // Type check the elif conditions and branches
            ASTNode* elifCondition = node->data.ifStmt.elifConditions;
            ASTNode* elifBranch = node->data.ifStmt.elifBranches;
            while (elifCondition != NULL && elifBranch != NULL) {
                // Type check the elif condition
                typeCheckNode(compiler, elifCondition);
                if (compiler->hadError) return;

                // Ensure the elif condition is a boolean
                Type* elifCondType = elifCondition->valueType;
                if (!elifCondType || elifCondType->kind != TYPE_BOOL) {
                    error(compiler, "Elif condition must be a boolean expression.");
                    return;
                }

                // Type check the elif branch
                typeCheckNode(compiler, elifBranch);
                if (compiler->hadError) return;

                // Move to the next elif condition and branch
                elifCondition = elifCondition->next;
                elifBranch = elifBranch->next;
            }

            // Type check the else branch if it exists
            if (node->data.ifStmt.elseBranch) {
                typeCheckNode(compiler, node->data.ifStmt.elseBranch);
                if (compiler->hadError) return;
            }

            // If statements don't have a value type
            node->valueType = NULL;
            break;
        }

        case AST_BLOCK: {

            // Type check each statement in the block
            ASTNode* stmt = node->data.block.statements;
            while (stmt) {
                typeCheckNode(compiler, stmt);
                if (compiler->hadError) return;
                stmt = stmt->next;
            }

            // Blocks don't have a value type
            node->valueType = NULL;
            break;
        }

        case AST_WHILE: {
            // Type check the condition
            typeCheckNode(compiler, node->data.whileStmt.condition);
            if (compiler->hadError) return;

            // Ensure the condition is a boolean
            Type* condType = node->data.whileStmt.condition->valueType;
            if (!condType || condType->kind != TYPE_BOOL) {
                error(compiler, "While condition must be a boolean expression.");
                return;
            }

            // Type check the body
            typeCheckNode(compiler, node->data.whileStmt.body);
            if (compiler->hadError) return;

            // While statements don't have a value type
            node->valueType = NULL;
            break;
        }

        case AST_FOR: {
            // Type check the range start expression
            typeCheckNode(compiler, node->data.forStmt.startExpr);
            if (compiler->hadError) return;

            // Type check the range end expression
            typeCheckNode(compiler, node->data.forStmt.endExpr);
            if (compiler->hadError) return;

            // Ensure the range expressions are integers
            Type* startType = node->data.forStmt.startExpr->valueType;
            Type* endType = node->data.forStmt.endExpr->valueType;

            if (!startType || (startType->kind != TYPE_I32 && startType->kind != TYPE_U32)) {
                error(compiler, "For loop range start must be an integer.");
                return;
            }

            if (!endType || (endType->kind != TYPE_I32 && endType->kind != TYPE_U32)) {
                error(compiler, "For loop range end must be an integer.");
                return;
            }

            // Define the iterator variable
            uint8_t index = defineVariable(compiler, node->data.forStmt.iteratorName, startType);
            node->data.forStmt.iteratorIndex = index;

            // Type check the body
            typeCheckNode(compiler, node->data.forStmt.body);
            if (compiler->hadError) return;

            // For statements don't have a value type
            node->valueType = NULL;
            break;
        }

        default:
            error(compiler, "Unsupported AST node type in type checker.");
            break;
    }
}

static void generateCode(Compiler* compiler, ASTNode* node) {
    if (!node || compiler->hadError) {
        return;
    }

    switch (node->type) {
        case AST_LITERAL: {
            emitConstant(compiler, node->data.literal);
            break;
        }

        case AST_BINARY: {

            // Generate code for operands
            generateCode(compiler, node->left);
            generateCode(compiler, node->right);
            if (compiler->hadError) return;

            // Get operand and result types
            Type* leftType = node->left->valueType;
            Type* rightType = node->right->valueType;
            TypeKind resultType = node->valueType->kind;

            // Convert right operand to result type if needed
            if (node->data.operation.convertRight) {

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
            if (node->data.operation.convertLeft) {

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

                // Comparison operators
                case TOKEN_LESS:
                    switch (leftType->kind) {
                        case TYPE_I32:
                            writeOp(compiler, OP_LESS_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_LESS_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_LESS_F64);
                            break;
                        default:
                            error(compiler, "Less than not supported for this type.");
                            return;
                    }
                    break;

                case TOKEN_LESS_EQUAL:
                    switch (leftType->kind) {
                        case TYPE_I32:
                            writeOp(compiler, OP_LESS_EQUAL_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_LESS_EQUAL_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_LESS_EQUAL_F64);
                            break;
                        default:
                            error(compiler, "Less than or equal not supported for this type.");
                            return;
                    }
                    break;

                case TOKEN_GREATER:
                    switch (leftType->kind) {
                        case TYPE_I32:
                            writeOp(compiler, OP_GREATER_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_GREATER_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_GREATER_F64);
                            break;
                        default:
                            error(compiler, "Greater than not supported for this type.");
                            return;
                    }
                    break;

                case TOKEN_GREATER_EQUAL:
                    switch (leftType->kind) {
                        case TYPE_I32:
                            writeOp(compiler, OP_GREATER_EQUAL_I32);
                            break;
                        case TYPE_U32:
                            writeOp(compiler, OP_GREATER_EQUAL_U32);
                            break;
                        case TYPE_F64:
                            writeOp(compiler, OP_GREATER_EQUAL_F64);
                            break;
                        default:
                            error(compiler, "Greater than or equal not supported for this type.");
                            return;
                    }
                    break;

                case TOKEN_EQUAL_EQUAL:
                    writeOp(compiler, OP_EQUAL);
                    break;

                case TOKEN_BANG_EQUAL:
                    writeOp(compiler, OP_NOT_EQUAL);
                    break;

                default:
                    error(compiler, "Unsupported binary operator.");
                    return;
            }
            break;
        }

        case AST_UNARY: {
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
            writeOp(compiler, OP_GET_GLOBAL);
            writeOp(compiler, node->data.variable.index);
            break;
        }

        case AST_LET: {
            generateCode(compiler, node->data.let.initializer);
            if (compiler->hadError) return;
            writeOp(compiler, OP_DEFINE_GLOBAL);
            writeOp(compiler, node->data.let.index);
            break;
        }

        case AST_PRINT: {
            generateCode(compiler, node->data.print.expr);
            if (compiler->hadError) return;
            writeOp(compiler, OP_PRINT);
            break;
        }

        case AST_ASSIGNMENT: {
            generateCode(compiler, node->left);
            if (compiler->hadError) return;
            writeOp(compiler, OP_SET_GLOBAL);
            writeOp(compiler, node->data.variable.index);
            break;
        }

        case AST_IF: {

            // Generate code for the condition
            generateCode(compiler, node->data.ifStmt.condition);
            if (compiler->hadError) return;

            // Emit a jump-if-false instruction
            // We'll patch this jump later
            int thenJump = compiler->chunk->count;
            writeOp(compiler, OP_JUMP_IF_FALSE);
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset

            // Pop the condition value from the stack
            writeOp(compiler, OP_POP);

            // Generate code for the then branch
            generateCode(compiler, node->data.ifStmt.thenBranch);
            if (compiler->hadError) return;

            // Emit a jump instruction to skip the else branch
            // We'll patch this jump later
            int elseJump = compiler->chunk->count;
            writeOp(compiler, OP_JUMP);
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset

            // Patch the then jump
            int thenEnd = compiler->chunk->count;
            compiler->chunk->code[thenJump + 1] = (thenEnd - thenJump - 3) >> 8;
            compiler->chunk->code[thenJump + 2] = (thenEnd - thenJump - 3) & 0xFF;

            // Generate code for elif branches if any
            ASTNode* elifCondition = node->data.ifStmt.elifConditions;
            ASTNode* elifBranch = node->data.ifStmt.elifBranches;
            int* elifJumps = NULL;
            int elifCount = 0;

            // Count the number of elif branches
            ASTNode* tempCondition = elifCondition;
            while (tempCondition != NULL) {
                elifCount++;
                tempCondition = tempCondition->next;
            }

            // Allocate memory for elif jumps
            if (elifCount > 0) {
                elifJumps = (int*)malloc(sizeof(int) * elifCount);
            }

            // Generate code for each elif branch
            int elifIndex = 0;
            while (elifCondition != NULL && elifBranch != NULL) {
                // Generate code for the elif condition
                generateCode(compiler, elifCondition);
                if (compiler->hadError) {
                    if (elifJumps) free(elifJumps);
                    return;
                }

                // Emit a jump-if-false instruction
                // We'll patch this jump later
                int elifThenJump = compiler->chunk->count;
                writeOp(compiler, OP_JUMP_IF_FALSE);
                writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset
                writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset

                // Pop the condition value from the stack
                writeOp(compiler, OP_POP);

                // Generate code for the elif branch
                generateCode(compiler, elifBranch);
                if (compiler->hadError) {
                    if (elifJumps) free(elifJumps);
                    return;
                }

                // Emit a jump instruction to skip the remaining branches
                // We'll patch this jump later
                elifJumps[elifIndex] = compiler->chunk->count;
                writeOp(compiler, OP_JUMP);
                writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset
                writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset

                // Patch the elif jump
                int elifEnd = compiler->chunk->count;
                compiler->chunk->code[elifThenJump + 1] = (elifEnd - elifThenJump - 3) >> 8;
                compiler->chunk->code[elifThenJump + 2] = (elifEnd - elifThenJump - 3) & 0xFF;

                // Move to the next elif condition and branch
                elifCondition = elifCondition->next;
                elifBranch = elifBranch->next;
                elifIndex++;
            }

            // Generate code for the else branch if it exists
            if (node->data.ifStmt.elseBranch) {
                generateCode(compiler, node->data.ifStmt.elseBranch);
                if (compiler->hadError) {
                    if (elifJumps) free(elifJumps);
                    return;
                }
            }

            // Patch the else jump
            int end = compiler->chunk->count;
            compiler->chunk->code[elseJump + 1] = (end - elseJump - 3) >> 8;
            compiler->chunk->code[elseJump + 2] = (end - elseJump - 3) & 0xFF;

            // Patch all elif jumps
            for (int i = 0; i < elifCount; i++) {
                int elifJump = elifJumps[i];
                compiler->chunk->code[elifJump + 1] = (end - elifJump - 3) >> 8;
                compiler->chunk->code[elifJump + 2] = (end - elifJump - 3) & 0xFF;
            }

            // Free the elif jumps array
            if (elifJumps) free(elifJumps);

            break;
        }

        case AST_BLOCK: {
            // Generate code for each statement in the block
            ASTNode* stmt = node->data.block.statements;
            while (stmt) {
                generateCode(compiler, stmt);
                if (compiler->hadError) return;
                stmt = stmt->next;
            }
            break;
        }

        case AST_WHILE: {

            // Store the current position to jump back to for the loop condition
            int loopStart = compiler->chunk->count;

            // Generate code for the condition
            generateCode(compiler, node->data.whileStmt.condition);
            if (compiler->hadError) return;

            // Emit a jump-if-false instruction
            // We'll patch this jump later
            int exitJump = compiler->chunk->count;
            writeOp(compiler, OP_JUMP_IF_FALSE);
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset

            // Pop the condition value from the stack
            writeOp(compiler, OP_POP);

            // Generate code for the body
            generateCode(compiler, node->data.whileStmt.body);
            if (compiler->hadError) return;

            // Emit a loop instruction to jump back to the condition
            writeOp(compiler, OP_LOOP);
            int offset = compiler->chunk->count - loopStart + 2;
            writeChunk(compiler->chunk, (offset >> 8) & 0xFF, 0);
            writeChunk(compiler->chunk, offset & 0xFF, 0);

            // Patch the exit jump
            int exitDest = compiler->chunk->count;
            compiler->chunk->code[exitJump + 1] = (exitDest - exitJump - 3) >> 8;
            compiler->chunk->code[exitJump + 2] = (exitDest - exitJump - 3) & 0xFF;

            break;
        }

        case AST_FOR: {

            // Generate code for the range start expression and store it in the iterator variable
            generateCode(compiler, node->data.forStmt.startExpr);
            if (compiler->hadError) return;

            writeOp(compiler, OP_DEFINE_GLOBAL);
            writeOp(compiler, node->data.forStmt.iteratorIndex);

            // Store the current position to jump back to for the loop condition
            int loopStart = compiler->chunk->count;

            // Generate code to check if the iterator is less than the end value
            // Get the current iterator value
            writeOp(compiler, OP_GET_GLOBAL);
            writeOp(compiler, node->data.forStmt.iteratorIndex);

            // Generate code for the range end expression
            generateCode(compiler, node->data.forStmt.endExpr);
            if (compiler->hadError) return;

            // Compare the iterator with the end value
            // Use the appropriate comparison based on the iterator type
            Type* iterType = node->data.forStmt.startExpr->valueType;
            if (iterType->kind == TYPE_I32) {
                writeOp(compiler, OP_LESS_I32);
            } else if (iterType->kind == TYPE_U32) {
                writeOp(compiler, OP_LESS_U32);
            } else {
                error(compiler, "Unsupported iterator type for for loop.");
                return;
            }

            // Emit a jump-if-false instruction to exit the loop
            // We'll patch this jump later
            int exitJump = compiler->chunk->count;
            writeOp(compiler, OP_JUMP_IF_FALSE);
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset
            writeChunk(compiler->chunk, 0xFF, 0);  // Placeholder for jump offset

            // Pop the condition value from the stack
            writeOp(compiler, OP_POP);

            // Generate code for the body
            generateCode(compiler, node->data.forStmt.body);
            if (compiler->hadError) return;

            // Increment the iterator
            writeOp(compiler, OP_GET_GLOBAL);
            writeOp(compiler, node->data.forStmt.iteratorIndex);

            // Add 1 to the iterator
            emitConstant(compiler, I32_VAL(1));

            // Use the appropriate addition based on the iterator type
            if (iterType->kind == TYPE_I32) {
                writeOp(compiler, OP_ADD_I32);
            } else if (iterType->kind == TYPE_U32) {
                writeOp(compiler, OP_ADD_U32);
            }

            // Store the incremented value back in the iterator
            writeOp(compiler, OP_SET_GLOBAL);
            writeOp(compiler, node->data.forStmt.iteratorIndex);

            // Emit a loop instruction to jump back to the condition
            writeOp(compiler, OP_LOOP);
            int offset = compiler->chunk->count - loopStart + 2;
            writeChunk(compiler->chunk, (offset >> 8) & 0xFF, 0);
            writeChunk(compiler->chunk, offset & 0xFF, 0);

            // Patch the exit jump
            int exitDest = compiler->chunk->count;
            compiler->chunk->code[exitJump + 1] = (exitDest - exitJump - 3) >> 8;
            compiler->chunk->code[exitJump + 2] = (exitDest - exitJump - 3) & 0xFF;

            break;
        }

        default:
            error(compiler, "Unsupported AST node type in code generator.");
            break;
    }
}

uint8_t defineVariable(Compiler* compiler, Token name, Type* type) {
    return addLocal(compiler, name, type);
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