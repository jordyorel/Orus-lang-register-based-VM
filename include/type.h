#ifndef clox_type_h
#define clox_type_h

#include <stdbool.h>
#include "value.h"

typedef enum {
    TYPE_I32,
    TYPE_I64,
    TYPE_U32,
    TYPE_U64,
    TYPE_F64,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_NIL,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_GENERIC,
    TYPE_COUNT
} TypeKind;

typedef struct FieldInfo {
    ObjString* name;
    struct Type* type;
} FieldInfo;

typedef struct VariantInfo {
    ObjString* name;
    struct Type** fieldTypes;  // Types of data carried by this variant
    ObjString** fieldNames;    // Names of fields (for destructuring)
    int fieldCount;            // Number of fields in this variant
} VariantInfo;

typedef struct Type {
    Obj obj;
    TypeKind kind;
    union {
        struct {
            struct Type* elementType;
        } array;
        struct {
            struct Type* returnType;
            struct Type** paramTypes;
            int paramCount;
        } function;
        struct {
            ObjString* name;
            FieldInfo* fields;
            int fieldCount;
            ObjString** genericParams;
            int genericCount;
        } structure;
        struct {
            ObjString* name;
        } generic;
        struct {
            ObjString* name;
            VariantInfo* variants;
            int variantCount;
            ObjString** genericParams;
            int genericCount;
        } enumeration;
    } info;
} Type;

Type* createPrimitiveType(TypeKind kind);
Type* createArrayType(Type* elementType);
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount);
Type* createStructType(ObjString* name, FieldInfo* fields, int fieldCount,
                       ObjString** generics, int genericCount);
Type* createEnumType(ObjString* name, VariantInfo* variants, int variantCount,
                     ObjString** generics, int genericCount);
Type* createGenericType(ObjString* name);
Type* findStructType(const char* name);
void freeType(Type* type);
bool typesEqual(Type* a, Type* b);
const char* getTypeName(TypeKind kind);
void initTypeSystem(void);
void freeTypeSystem(void);  // Add this
Type* getPrimitiveType(TypeKind kind);
void markTypeRoots();
Type* substituteGenerics(Type* type, ObjString** names, Type** subs, int count);
Type* instantiateStructType(Type* base, Type** args, int argCount);

extern Type* primitiveTypes[TYPE_COUNT];

typedef enum {
    CONSTRAINT_NONE,
    CONSTRAINT_NUMERIC,
    CONSTRAINT_COMPARABLE
} GenericConstraint;

#endif