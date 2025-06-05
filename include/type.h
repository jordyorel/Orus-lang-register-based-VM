#ifndef clox_type_h
#define clox_type_h

#include <stdbool.h>
#include "value.h"

typedef enum {
    TYPE_I32,
    TYPE_U32,
    TYPE_F64,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_NIL,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_COUNT
} TypeKind;

typedef struct FieldInfo {
    ObjString* name;
    struct Type* type;
} FieldInfo;

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
        } structure;
    } info;
} Type;

Type* createPrimitiveType(TypeKind kind);
Type* createArrayType(Type* elementType);
Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount);
Type* createStructType(ObjString* name, FieldInfo* fields, int fieldCount);
Type* findStructType(const char* name);
void freeType(Type* type);
bool typesEqual(Type* a, Type* b);
const char* getTypeName(TypeKind kind);
void initTypeSystem(void);
void freeTypeSystem(void);  // Add this
Type* getPrimitiveType(TypeKind kind);
void markTypeRoots();

extern Type* primitiveTypes[TYPE_COUNT];

#endif