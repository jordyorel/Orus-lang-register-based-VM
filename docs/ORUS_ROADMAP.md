# Orus Language Roadmap

This document consolidates the development roadmaps for the Orus language, tracking progress and version increments across multiple development streams.

**Current Version: 0.5.0**

## Version History

- **0.1.0**: Initial release with basic language features
- **0.5.0**: Current version with generics, modules, error handling, and garbage collection

## Completed Major Features

| Feature | Description | Version Impact |
|---------|-------------|----------------|
| ✅ Garbage Collection | Mark-sweep GC replacing manual memory management | 0.3.0 → 0.4.0 |
| ✅ Generic Types (Basic) | Generic functions and structs with type parameters | 0.2.0 → 0.3.0 |
| ✅ Module System | Modules and import statements | 0.1.0 → 0.2.0 |
| ✅ Error Handling | Try/catch blocks for exception handling | 0.4.0 → 0.5.0 |
| ✅ Dynamic Arrays | Arrays with push, pop, and len operations | Minor feature |

## Pending Features and Version Targets

### 1. Generics Enhancements

**Status**: Partially implemented (Basic functionality available)

**Note**: Regular (non-generic) forward declarations are already supported in the language. Forward declarations for generic functions are still needed.

| Task | Status | Priority | Version Impact |
|------|--------|----------|----------------|
| Generic Forward Declarations & Prepass | Partially implemented | High | 0.5.0 → 0.6.0 |
| Generic Constraints | Not started | High | 0.6.0 → 0.7.0 |
| Improved Type Inference | Not started | High | Minor feature |
| Generic Arithmetic and Operators | Not started | Medium | Minor feature |
| Collection and Iterator Support | Not started | Medium | Minor feature |
| Enhanced Error Reporting | Not started | Medium | Minor feature |
| Cross-Module Generics | Not started | Low | Minor feature |
| Generics Documentation | Not started | Low | No impact |

**Next Version Milestone**: 0.6.0 - Implementation of Generic Forward Declarations & Prepass Collection

### 2. Native Compilation Pipeline

**Status**: Optional future enhancement (post-stability)

**Note**: Compilation is considered an optional feature to be explored after language stability is achieved. It is not a mandatory development path and may be pursued based on community interest and practical needs.

| Task | Status | Priority | Version Impact |
|------|--------|----------|----------------|
| IR Design | Not started | Low | Post-1.0 |
| Type System Enhancements | Not started | Low | Minor feature |
| Core Compilation Pipeline | Not started | Low | Post-1.0 |
| Compiler Optimizations | Not started | Low | Minor feature |
| Standard Library Adaptation | Not started | Low | Minor feature |
| Build System | Not started | Low | Minor feature |
| Toolchain Integration | Not started | Low | Minor feature |
| Advanced Compilation Features | Not started | Low | Post-1.0 |
| Compiler Documentation | Not started | Low | No impact |
| Interpreter/Compiler Compatibility | Not started | Medium | Minor feature |
| ABI Stability | Not started | Medium | Minor feature |

**Next Version Milestone**: 0.8.0 - Implementation of IR Design and basic compilation

### 3. Memory Management (Garbage Collection)

**Status**: ✅ Implemented

A mark-sweep garbage collector has been implemented according to the GARBAGE_COLLECTOR_PLAN.md document:

| Task | Status | Impact |
|------|--------|--------|
| Object Header Structure | ✅ Complete | Core implementation |
| Value Representation Changes | ✅ Complete | Core implementation |
| VM State Updates | ✅ Complete | Core implementation |
| Allocation Helper Functions | ✅ Complete | Core implementation |
| Mark Phase Implementation | ✅ Complete | Core implementation |
| Sweep Phase Implementation | ✅ Complete | Core implementation |
| Integration with Existing Code | ✅ Complete | Core implementation |
| GC Testing and Tuning | ✅ Complete | Core implementation |

The garbage collection implementation resulted in version increment from 0.3.0 to 0.4.0.

### 4. Standard Library

**Status**: Early planning phase (Not yet supported)

| Module | Status | Priority | Version Impact |
|--------|--------|----------|----------------|
| Core Types | Not started | High | 0.5.0 → 0.6.0 |
| Collections | Not started | High | Minor feature |
| I/O | Not started | High | Minor feature |
| String Processing | Not started | Medium | Minor feature |
| File System | Not started | Medium | Minor feature |
| Time and Date | Not started | Medium | Minor feature |
| Networking | Not started | Low | 0.7.0 → 0.8.0 |
| Regular Expressions | Not started | Low | Minor feature |
| Serialization | Not started | Low | Minor feature |
| Concurrency | Not started | Low | 0.9.0 → 1.0.0 |

**Standard Library Development Plan:**

1. **Core Types (High Priority)**
   - Enhanced numeric types (BigInt, BigDecimal)
   - Option/Result types for error handling
   - Range implementations
   - Basic tuple types
   - Reference documentation

2. **Collections (High Priority)**
   - Generic Map implementation
   - Generic Set implementation
   - Linked List
   - Queue and Priority Queue
   - Stack
   - Comprehensive iterator support
   - Collection algorithms (sort, filter, map, reduce)

3. **I/O and System (High Priority)**
   - Standard input/output streams
   - File reading and writing
   - Environment variables
   - Command-line argument parsing
   - Process execution

4. **String Processing (Medium Priority)**
   - String builders
   - Pattern matching
   - String transformations (case conversion, trimming, etc.)
   - Unicode support
   - String validation

5. **File System (Medium Priority)**
   - Directory operations
   - File metadata
   - Path manipulation
   - File watching

6. **Time and Date (Medium Priority)**
   - Date and time types
   - Time zone support
   - Date formatting and parsing
   - Duration and interval calculations

7. **Networking (Low Priority)**
   - TCP/IP client/server
   - HTTP client
   - URI parsing
   - Socket programming
   - Basic protocols (SMTP, FTP)

8. **Regular Expressions (Low Priority)**
   - Pattern compilation
   - String matching
   - Capturing groups
   - Replacement operations

9. **Serialization (Low Priority)**
   - JSON support
   - Binary serialization
   - CSV parsing/generation
   - YAML support

10. **Concurrency (Low Priority)**
    - Thread abstractions
    - Async/await patterns
    - Worker pools
    - Channels for communication
    - Synchronization primitives

**Implementation Strategy:**

- Modular design with minimal interdependencies
- Consistent API design across all modules
- Comprehensive test coverage
- Focus on performance and memory efficiency
- Leverage generic capabilities when available
- Build incrementally, starting with core modules

**Version Impact:**
- Core Types and Collections are targeted for version 0.6.0
- Networking features are targeted for version 0.8.0
- Complete concurrency support is targeted for version 1.0.0

## Path to Version 1.0

Based on the current state and roadmap, the path to version 1.0 is expected to include:

1. **Version 0.6.0**: Complete generic forward declarations/prepass (regular forward declarations already supported)
2. **Version 0.7.0**: Implement generic constraints
3. **Version 0.8.0**: Expand standard library, focus on core types and collections
4. **Version 0.9.0**: Complete concurrency support and enhanced error handling
5. **Version 1.0.0**: Finalize language stability, documentation, and performance optimizations

**Note**: Native compilation remains an optional enhancement that may be pursued after achieving language stability in version 1.0, but is not a requirement for the 1.0 release.

## Timeline Estimates

- **Version 0.6.0**: Q3 2025
- **Version 0.7.0**: Q4 2025
- **Version 0.8.0**: Q1 2026
- **Version 0.9.0**: Q3 2026
- **Version 1.0.0**: Q1 2027
- **Compilation (if pursued)**: Post-1.0 release, timeline TBD based on community interest

## Development Priorities

1. **Short-term (3 months)**
   - Complete generic forward declarations & prepass collection (building on existing non-generic forward declaration support)
   - Begin design work on generic constraints

2. **Medium-term (6-12 months)**
   - Implement generic constraints 
   - Expand standard library core types and collections
   - Enhance error handling and reporting

3. **Long-term (1-2 years)**
   - Complete concurrency support
   - Optimize interpreter performance
   - Enhance language stability and reliability
   - Reach 1.0.0 stability milestone

4. **Post 1.0 (Optional)**
   - Explore compilation options if community interest warrants
   - Design IR structure if compilation is pursued
   - Consider LLVM-based approach as one possible implementation path

## Compatibility Considerations

- Maintain backward compatibility with existing Orus code through 0.x series
- Document breaking changes clearly when they occur
- Provide migration guides between major versions
- Consider providing compatibility layers where feasible
