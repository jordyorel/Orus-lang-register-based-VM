#ifndef clox_symtable_h
#define clox_symtable_h

#include "common.h"
#include "type.h"

typedef struct {
    const char* name;
    Type* type;
    bool isDefined;
    int scope;
    uint8_t index;
} Symbol;

typedef struct {
    Symbol* symbols;
    int count;
    int capacity;
} SymbolTable;

void initSymbolTable(SymbolTable* table);
void freeSymbolTable(SymbolTable* table);
bool addSymbol(SymbolTable* table, const char* name, Type* type, int scope, uint8_t index);
Symbol* findSymbol(SymbolTable* table, const char* name);
void removeSymbolsFromScope(SymbolTable* table, int scope);

#endif
