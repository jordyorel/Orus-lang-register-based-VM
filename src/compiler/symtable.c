#include <stdlib.h>
#include <string.h>
#include "../../include/symtable.h"
#include "../../include/memory.h"

#define INITIAL_CAPACITY 8

void initSymbolTable(SymbolTable* table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

void freeSymbolTable(SymbolTable* table) {
    for (int i = 0; i < table->count; i++) {
        freeType(table->symbols[i].type);
    }
    free(table->symbols);
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

static void growCapacity(SymbolTable* table) {
    int newCapacity = table->capacity < 8 ? 8 : table->capacity * 2;
    Symbol* newSymbols = (Symbol*)realloc(table->symbols, 
                                         sizeof(Symbol) * newCapacity);
    table->symbols = newSymbols;
    table->capacity = newCapacity;
}

bool addSymbol(SymbolTable* table, const char* name, Type* type, int scope) {
    // Check if symbol already exists in current scope
    for (int i = 0; i < table->count; i++) {
        if (table->symbols[i].scope == scope && 
            strcmp(table->symbols[i].name, name) == 0) {
            return false;  // Symbol already defined in this scope
        }
    }
    
    if (table->count + 1 > table->capacity) {
        growCapacity(table);
    }
    
    table->symbols[table->count].name = name;
    table->symbols[table->count].type = type;
    table->symbols[table->count].isDefined = true;
    table->symbols[table->count].scope = scope;
    table->count++;
    
    return true;
}

Symbol* findSymbol(SymbolTable* table, const char* name) {
    // Search from most recent scope to oldest
    for (int i = table->count - 1; i >= 0; i--) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }
    return NULL;
}
