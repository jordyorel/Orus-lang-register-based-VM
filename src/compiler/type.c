#include <stdio.h>
#include <stdlib.h>
#include "../../include/type.h"
#include "../../include/memory.h"
#include "../../include/common.h"

/**
 * @file type.c
 * @brief Runtime type system utilities.
 *
 * This module manages the `Type` structures used during compilation and
 * execution. It provides constructors for primitive and composite types and
 * helpers for comparing and substituting types.
 */
#include <string.h>

// Define the array of primitive types
Type* primitiveTypes[TYPE_COUNT] = {NULL};
static Type* structTypes[UINT8_COUNT] = {NULL};
static int structTypeCount = 0;
static Type* enumTypes[UINT8_COUNT] = {NULL};
static int enumTypeCount = 0;
static bool typeSystemInitialized = false;

/**
 * Initialize the global type system and create primitive types.
 */
void initTypeSystem(void) {
    if (typeSystemInitialized) return;
    
    for (int i = 0; i < TYPE_COUNT; i++) {
        primitiveTypes[i] = NULL;
    }
    
    primitiveTypes[TYPE_I32] = createPrimitiveType(TYPE_I32);
    primitiveTypes[TYPE_I64] = createPrimitiveType(TYPE_I64);
    primitiveTypes[TYPE_U32] = createPrimitiveType(TYPE_U32);
    primitiveTypes[TYPE_U64] = createPrimitiveType(TYPE_U64);
    primitiveTypes[TYPE_F64] = createPrimitiveType(TYPE_F64);
    primitiveTypes[TYPE_BOOL] = createPrimitiveType(TYPE_BOOL);
    primitiveTypes[TYPE_STRING] = createPrimitiveType(TYPE_STRING);
    primitiveTypes[TYPE_VOID] = createPrimitiveType(TYPE_VOID);
    primitiveTypes[TYPE_NIL] = createPrimitiveType(TYPE_NIL);
    
    typeSystemInitialized = true;
}

// Add this to help with debugging
Type* getPrimitiveType(TypeKind kind) {
    if (kind < 0 || kind >= TYPE_COUNT) return NULL;
    return primitiveTypes[kind];
}

/**
 * Allocate a primitive type descriptor.
 *
 * @param kind Primitive kind constant.
 * @return Newly allocated Type object.
 */
Type* createPrimitiveType(TypeKind kind) {
    Type* type = allocateType();
    type->kind = kind;
    return type;
}

/**
 * Create an array type.
 *
 * @param elementType Element type stored in the array.
 * @return Newly allocated Type object.
 */
Type* createArrayType(Type* elementType) {
    Type* type = allocateType();
    type->kind = TYPE_ARRAY;
    type->info.array.elementType = elementType;
    return type;
}

/**
 * Create a function type.
 *
 * @param returnType  Return type of the function.
 * @param paramTypes  Array of parameter types.
 * @param paramCount  Number of parameters.
 * @return Newly allocated Type object.
 */
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount) {
    Type* type = allocateType();
    type->kind = TYPE_FUNCTION;
    type->info.function.returnType = returnType;
    type->info.function.paramTypes = paramTypes;
    type->info.function.paramCount = paramCount;
    return type;
}

Type* createStructType(ObjString* name, FieldInfo* fields, int fieldCount,
                       ObjString** generics, int genericCount) {
    if (structTypeCount >= UINT8_COUNT) return NULL;
    Type* type = allocateType();
    type->kind = TYPE_STRUCT;
    type->info.structure.name = name;
    type->info.structure.fields = fields;
    type->info.structure.fieldCount = fieldCount;
    type->info.structure.genericParams = generics;
    type->info.structure.genericCount = genericCount;
    structTypes[structTypeCount++] = type;
    return type;
}

Type* createEnumType(ObjString* name, VariantInfo* variants, int variantCount,
                     ObjString** generics, int genericCount) {
    if (enumTypeCount >= UINT8_COUNT) return NULL;
    Type* type = allocateType();
    type->kind = TYPE_ENUM;
    type->info.enumeration.name = name;
    type->info.enumeration.variants = variants;
    type->info.enumeration.variantCount = variantCount;
    type->info.enumeration.genericParams = generics;
    type->info.enumeration.genericCount = genericCount;
    enumTypes[enumTypeCount++] = type;
    return type;
}

Type* createGenericType(ObjString* name) {
    Type* type = allocateType();
    type->kind = TYPE_GENERIC;
    type->info.generic.name = name;
    return type;
}

Type* findStructType(const char* name) {
    for (int i = 0; i < structTypeCount; i++) {
        if (strcmp(structTypes[i]->info.structure.name->chars, name) == 0) {
            return structTypes[i];
        }
    }
    return NULL;
}

Type* findEnumType(const char* name) {
    for (int i = 0; i < enumTypeCount; i++) {
        if (strcmp(enumTypes[i]->info.enumeration.name->chars, name) == 0) {
            return enumTypes[i];
        }
    }
    return NULL;
}

void freeType(Type* type) {
    (void)type; // GC-managed
}

void freeTypeSystem(void) {
    if (!typeSystemInitialized) return;
    for (int i = 0; i < TYPE_COUNT; i++) {
        primitiveTypes[i] = NULL;
    }
    for (int i = 0; i < structTypeCount; i++) {
        structTypes[i] = NULL;
    }
    for (int i = 0; i < enumTypeCount; i++) {
        enumTypes[i] = NULL;
    }
    structTypeCount = 0;
    enumTypeCount = 0;
    typeSystemInitialized = false;
}

bool typesEqual(Type* a, Type* b) {
    if (a == NULL || b == NULL) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_I32:
        case TYPE_I64:
        case TYPE_U32:
        case TYPE_U64:
        case TYPE_F64:
        case TYPE_BOOL:
        case TYPE_STRING:
        case TYPE_NIL:
            return true;  // Same kind means same type for primitives
            
        case TYPE_ARRAY:
            return typesEqual(a->info.array.elementType,
                            b->info.array.elementType);
            
        case TYPE_FUNCTION: {
            if (!typesEqual(a->info.function.returnType,
                           b->info.function.returnType)) {
                return false;
            }
            if (a->info.function.paramCount != b->info.function.paramCount) {
                return false;
            }
            for (int i = 0; i < a->info.function.paramCount; i++) {
                if (!typesEqual(a->info.function.paramTypes[i],
                               b->info.function.paramTypes[i])) {
                    return false;
                }
            }
            return true;
        }

        case TYPE_STRUCT:
            return strcmp(a->info.structure.name->chars,
                          b->info.structure.name->chars) == 0;

        case TYPE_ENUM:
            return strcmp(a->info.enumeration.name->chars,
                          b->info.enumeration.name->chars) == 0;

        case TYPE_GENERIC:
            return strcmp(a->info.generic.name->chars,
                          b->info.generic.name->chars) == 0;

        default:
            return false;
    }
}

bool canImplicitlyConvert(Type* from, Type* to, ASTNode* node) {
    if (from == NULL || to == NULL) return false;
    
    // If types are already equal, no conversion needed
    if (typesEqual(from, to)) return true;
    
    // Allow safe numeric conversions for literals
    if (node && node->type == AST_LITERAL) {
        // Small integer literals can be safely promoted to larger types
        
        // i32 literals can be implicitly converted to i64 if they fit
        if (from->kind == TYPE_I32 && to->kind == TYPE_I64) {
            return true;
        }
        
        // u32 literals can be implicitly converted to u64 if they fit  
        if (from->kind == TYPE_U32 && to->kind == TYPE_U64) {
            return true;
        }
        
        // Small integer literals can be implicitly converted to other integer types
        // if the value fits (for common cases like 0, 1, -1, small constants)
        if ((from->kind == TYPE_I32 || from->kind == TYPE_U32) && 
            (to->kind == TYPE_I32 || to->kind == TYPE_I64 || to->kind == TYPE_U32 || to->kind == TYPE_U64)) {
            
            // Get the literal value to check if it fits in the target type
            Value val = node->data.literal;
            
            if (from->kind == TYPE_I32) {
                int32_t i32Val = AS_I32(val);
                
                // Check if i32 value fits in target type
                if (to->kind == TYPE_I64) return true; // i32 always fits in i64
                if (to->kind == TYPE_U32 && i32Val >= 0) return true; // non-negative i32 fits in u32
                if (to->kind == TYPE_U64 && i32Val >= 0) return true; // non-negative i32 fits in u64
            }
            
            if (from->kind == TYPE_U32) {
                uint32_t u32Val = AS_U32(val);
                
                // Check if u32 value fits in target type  
                if (to->kind == TYPE_I64) return true; // u32 always fits in i64
                if (to->kind == TYPE_U64) return true; // u32 always fits in u64
                if (to->kind == TYPE_I32 && u32Val <= INT32_MAX) return true; // small u32 fits in i32
            }
        }
        
        // i32 literals can be implicitly converted to f64
        if (from->kind == TYPE_I32 && to->kind == TYPE_F64) {
            return true;
        }
        
        // u32 literals can be implicitly converted to f64
        if (from->kind == TYPE_U32 && to->kind == TYPE_F64) {
            return true;
        }
    }
    
    // No implicit conversion possible
    return false;
}

const char* getTypeName(TypeKind kind) {
    switch (kind) {
        case TYPE_I32: return "i32";
        case TYPE_I64: return "i64";
        case TYPE_U32: return "u32";
        case TYPE_U64: return "u64";
        case TYPE_F64: return "f64";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_NIL: return "nil";
        case TYPE_ARRAY: return "array";
        case TYPE_FUNCTION: return "function";
        case TYPE_STRUCT: return "struct";
        case TYPE_ENUM: return "enum";
        case TYPE_GENERIC: return "generic";
        default: return "unknown";
    }
}

void markTypeRoots() {
    for (int i = 0; i < TYPE_COUNT; i++) {
        if (primitiveTypes[i]) markObject((Obj*)primitiveTypes[i]);
    }
    for (int i = 0; i < structTypeCount; i++) {
        if (structTypes[i]) markObject((Obj*)structTypes[i]);
    }
    for (int i = 0; i < enumTypeCount; i++) {
        if (enumTypes[i]) markObject((Obj*)enumTypes[i]);
    }
}

Type* substituteGenerics(Type* type, ObjString** names, Type** subs, int count) {
    if (!type) return NULL;
    switch (type->kind) {
        case TYPE_GENERIC: {
            for (int i = 0; i < count; i++) {
                if (names[i] && strcmp(type->info.generic.name->chars,
                                      names[i]->chars) == 0) {
                    return subs[i] ? subs[i] : type;
                }
            }
            return type;
        }
        case TYPE_ARRAY: {
            Type* elem = substituteGenerics(type->info.array.elementType,
                                           names, subs, count);
            if (elem == type->info.array.elementType) return type;
            return createArrayType(elem);
        }
        case TYPE_FUNCTION: {
            int pc = type->info.function.paramCount;
            Type** params = NULL;
            if (pc > 0) {
                params = (Type**)malloc(sizeof(Type*) * pc);
                for (int i = 0; i < pc; i++) {
                    params[i] = substituteGenerics(type->info.function.paramTypes[i],
                                                   names, subs, count);
                }
            }
            Type* ret = substituteGenerics(type->info.function.returnType,
                                          names, subs, count);
            return createFunctionType(ret, params, pc);
        }
        default:
            return type;
    }
}

Type* instantiateStructType(Type* base, Type** args, int argCount) {
    if (!base || base->kind != TYPE_STRUCT) return base;
    FieldInfo* fields = NULL;
    int fcount = base->info.structure.fieldCount;
    if (fcount > 0) fields = (FieldInfo*)malloc(sizeof(FieldInfo) * fcount);
    for (int i = 0; i < fcount; i++) {
        fields[i].name = base->info.structure.fields[i].name;
        fields[i].type = substituteGenerics(base->info.structure.fields[i].type,
                                           base->info.structure.genericParams,
                                           args, argCount);
    }
    return createStructType(base->info.structure.name, fields, fcount, NULL, 0);
}
