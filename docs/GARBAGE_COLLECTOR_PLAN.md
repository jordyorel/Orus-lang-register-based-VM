# Garbage Collector Integration Plan

This document outlines a high level approach for replacing the current manual memory management with an automatic garbage collector.

## Motivation

At the moment memory is allocated and freed manually using helpers in `memory.c` such as `reallocate` and `copyString`:

```c
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}
```

Manual freeing is spread across the code base (for example in `freeASTNode` and the VM cleanup). This makes memory ownership tricky and has already resulted in leaks. Introducing a garbage collector will centralise reclamation and simplify the interpreter.

## Proposed Collector

A mark–sweep collector fits the current architecture. It only requires a list of allocated objects and a way to mark reachable ones. Sweeping will free everything else.

1. **Object header**
   * Introduce an `Obj` struct with a `type` tag, a `bool marked` field and a `Obj* next` pointer linking all heap objects.
   * Heap allocated values such as strings and arrays will embed this header (e.g. `typedef struct { Obj obj; int length; char* chars; } ObjString;`).

2. **Value representation changes**
   * Replace inline `String`/`Array` data inside `Value` with pointers to the corresponding `Obj` structures.
   * Update creation macros in `value.h` accordingly.

3. **VM state changes**
   * Add a `Obj* objects` field to `VM` to track the list of allocations and a `size_t bytesAllocated` counter.
   * Roots for marking include the VM stack, global variables and the function/AST structures currently stored in the VM.

4. **Allocation helpers**
   * Implement `allocateObject` in `memory.c` to allocate memory, link the new object into `vm.objects` and optionally trigger a GC cycle when `bytesAllocated` crosses a threshold.
   * Replace direct `malloc` usage (e.g. in `copyString` and `createLiteralNode`) with these helpers.

5. **Mark phase**
   * Provide `markValue` and `markObject` functions that set the `marked` flag on reachable objects, recursively marking arrays and strings.
   * When running the collector, walk the stack, globals and other roots calling these functions.

6. **Sweep phase**
   * Iterate over the `vm.objects` list freeing unmarked objects using the existing `reallocate`/`free` logic.
   * Clear marks on the remaining objects for the next GC cycle.

7. **Integrating with existing code**
   * Remove `free()` calls such as those in `freeASTNode` and `freeVM`; the collector will reclaim nodes and strings automatically.
   * Update the `FREE_ARRAY` macro in `memory.h` so it uses `reallocate` but relies on GC for final cleanup of array contents.

8. **Testing and tuning**
   * Add stress tests allocating many temporary values to exercise the GC.
   * Tune the initial `bytesAllocated` threshold and potentially expose a manual `collect()` function for testing.

This plan introduces automatic memory management while preserving the existing VM structure. Further improvements such as generational collection or arenas can be explored later once basic GC support is stable.
