#ifndef clox_symtable_h
#define clox_symtable_h

#include "common.h"
#include "type.h"
#include "scanner.h"

// Forward declarations - Module defined in modules.h
typedef struct Module Module;

typedef struct {
    const char* name;
    Type* type;
    bool isDefined;
    int scope;
    uint8_t index;
    bool active;
    bool isMutable;
    bool isConst;
    bool isModule;           // True if this symbol represents a module alias
    Module* module;          // Module associated with the alias
    Token token;
} Symbol;

typedef struct {
    Symbol* symbols;
    int count;
    int capacity;
} SymbolTable;

void initSymbolTable(SymbolTable* table);
void freeSymbolTable(SymbolTable* table);
bool addSymbol(SymbolTable* table, const char* name, Token token, Type* type,
               int scope, uint8_t index, bool isMutable, bool isConst,
               bool isModule, Module* module);
Symbol* findSymbol(SymbolTable* table, const char* name);
Symbol* findAnySymbol(SymbolTable* table, const char* name);
void removeSymbolsFromScope(SymbolTable* table, int scope);

#endif
