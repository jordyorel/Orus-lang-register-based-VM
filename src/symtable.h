#ifndef clox_symtable_h
#define clox_symtable_h

#include "common.h"
#include "type.h"

typedef struct {
    const char* name;
    Type* type;
    bool isDefined;
    int scope;
} Symbol;

typedef struct {
    Symbol* symbols;
    int count;
    int capacity;
} SymbolTable;

void initSymbolTable(SymbolTable* table);
void freeSymbolTable(SymbolTable* table);
bool addSymbol(SymbolTable* table, const char* name, Type* type, int scope);
Symbol* findSymbol(SymbolTable* table, const char* name);

#endif
