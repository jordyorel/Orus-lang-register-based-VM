#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../include/symtable.h"
#include "../../include/memory.h"

/**
 * @file symtable.c
 * @brief Symbol table implementation for the compiler.
 *
 * The symbol table tracks variables and functions across scopes during
 * compilation. Entries are stored in a simple array that grows as needed.
 */

#define INITIAL_CAPACITY 8

/**
 * Initialize a symbol table instance.
 *
 * @param table Table to initialize.
 */
void initSymbolTable(SymbolTable* table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

/**
 * Free memory associated with a symbol table.
 *
 * @param table Table to clean up.
 */
void freeSymbolTable(SymbolTable* table) {
    // Types are managed elsewhere; do not free them here
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

bool addSymbol(SymbolTable* table, const char* name, Token token, Type* type,
               int scope, uint8_t index, bool isMutable, bool isConst,
               bool isModule, Module* module) {
    for (int i = 0; i < table->count; i++) {
        if (table->symbols[i].scope == scope && table->symbols[i].active &&
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
    table->symbols[table->count].index = index;
    table->symbols[table->count].active = true;
    table->symbols[table->count].isMutable = isMutable;
    table->symbols[table->count].isConst = isConst;
    table->symbols[table->count].isModule = isModule;
    table->symbols[table->count].module = module;
    table->symbols[table->count].token = token;
    table->count++;
    
    return true;
}

Symbol* findSymbol(SymbolTable* table, const char* name) {
    for (int i = table->count - 1; i >= 0; i--) {
        if (!table->symbols[i].active) continue;
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }
    return NULL;
}

Symbol* findAnySymbol(SymbolTable* table, const char* name) {
    for (int i = table->count - 1; i >= 0; i--) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }
    return NULL;
}

void removeSymbolsFromScope(SymbolTable* table, int scope) {
    for (int i = table->count - 1; i >= 0; i--) {
        if (table->symbols[i].scope < scope) break;
        table->symbols[i].active = false;
    }
}
