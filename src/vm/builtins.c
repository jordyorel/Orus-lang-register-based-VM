/**
 * @file builtins.c
 * @brief Implementation of built-in native functions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "../../include/builtins.h"
#include "../../include/error.h"
#include "../../include/memory.h"
#include "../../include/vm_ops.h"
#include "../../include/type.h"
#include "../../include/modules.h"

extern VM vm;


/**
 * Implementation of the `len` builtin. Returns the length of an array or
 * string.
 *
 * @param argCount Number of arguments provided.
 * @param args     Values of the arguments.
 */
static Value native_len(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("len() takes exactly one argument.");
        return NIL_VAL;
    }
    Value val = args[0];
    if (IS_ARRAY(val)) {
        return I32_VAL(AS_ARRAY(val)->length);
    } else if (IS_STRING(val)) {
        return I32_VAL(AS_STRING(val)->length);
    }
    vmRuntimeError("len() expects array or string.");
    return NIL_VAL;
}

/**
 * Extracts a substring from a string.
 *
 * @param argCount Number of arguments.
 * @param args     [string, start, length].
 */
static Value native_substring(int argCount, Value* args) {
    if (argCount != 3) {
        vmRuntimeError("substring() takes exactly three arguments.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_I32(args[1]) || !IS_I32(args[2])) {
        vmRuntimeError("substring() expects (string, i32, i32).");
        return NIL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = AS_I32(args[1]);
    int length = AS_I32(args[2]);
    if (start < 0) start = 0;
    if (start > str->length) start = str->length;
    if (length < 0) length = 0;
    if (start + length > str->length) length = str->length - start;
    ObjString* result = allocateString(str->chars + start, length);
    return STRING_VAL(result);
}

/**
 * Appends an element to the end of an array and returns the array.
 *
 * @param argCount Number of arguments.
 * @param args     [array, value].
 */
static Value native_push(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("push() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("First argument to push() must be array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    arrayPush(&vm, arr, args[1]);
    return args[0];
}

/**
 * Removes and returns the last element of an array.
 *
 * @param argCount Number of arguments.
 * @param args     [array].
 */
static Value native_pop(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("pop() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("pop() expects array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    return arrayPop(arr);
}

/**
 * Preallocate capacity for a dynamic array.
 *
 * @param argCount Number of arguments.
 * @param args     [array, capacity].
 */
static Value native_reserve(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("reserve() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("First argument to reserve() must be array.");
        return NIL_VAL;
    }
    if (!(IS_I32(args[1]) || IS_I64(args[1]) || IS_U32(args[1]) || IS_U64(args[1]))) {
        vmRuntimeError("reserve() expects integer capacity.");
        return NIL_VAL;
    }
    int cap = IS_I32(args[1])  ? AS_I32(args[1])  :
              IS_I64(args[1])  ? (int)AS_I64(args[1])  :
              IS_U32(args[1])  ? (int)AS_U32(args[1]) :
                                (int)AS_U64(args[1]);
    if (cap <= 0) return args[0];
    ObjArray* arr = AS_ARRAY(args[0]);
    if (cap > arr->capacity) {
        int oldCap = arr->capacity;
        arr->capacity = cap;
        arr->elements = GROW_ARRAY(Value, arr->elements, oldCap, arr->capacity);
        vm.bytesAllocated += sizeof(Value) * (arr->capacity - oldCap);
    }
    return args[0];
}

/**
 * Generates an array of integers from start (inclusive) to end (exclusive).
 *
 * @param argCount Number of arguments.
 * @param args     [start, end].
 */
static Value native_range(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("range() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!(IS_I32(args[0]) || IS_I64(args[0]) || IS_U32(args[0]) || IS_U64(args[0])) ||
        !(IS_I32(args[1]) || IS_I64(args[1]) || IS_U32(args[1]) || IS_U64(args[1]))) {
        vmRuntimeError("range() expects (i32/i64/u32/u64, i32/i64/u32/u64).");
        return NIL_VAL;
    }

    int64_t start =
        IS_I32(args[0])  ? AS_I32(args[0])  :
        IS_I64(args[0])  ? AS_I64(args[0])  :
        IS_U32(args[0])  ? (int64_t)AS_U32(args[0]) :
                          (int64_t)AS_U64(args[0]);
    int64_t end =
        IS_I32(args[1])  ? AS_I32(args[1])  :
        IS_I64(args[1])  ? AS_I64(args[1])  :
        IS_U32(args[1])  ? (int64_t)AS_U32(args[1]) :
                          (int64_t)AS_U64(args[1]);

    ObjRangeIterator* it = allocateRangeIterator(start, end);
    return RANGE_ITERATOR_VAL(it);
}

/**
 * Helper that converts a runtime value to its type name string.
 *
 * @param val Value to inspect.
 * @return    Name of the value's type.
 */
static const char* getValueTypeName(Value val) {
    switch (val.type) {
        case VAL_I32:   return "i32";
        case VAL_I64:   return "i64";
        case VAL_U32:   return "u32";
        case VAL_U64:   return "u64";
        case VAL_F64:   return "f64";
        case VAL_BOOL:  return "bool";
        case VAL_NIL:   return "nil";
        case VAL_STRING:return "string";
        case VAL_ARRAY: return "array";
        case VAL_ERROR: return "error";
        case VAL_RANGE_ITERATOR: return "range";
        case VAL_ENUM: return "enum";
        default:        return "unknown";
    }
}

/**
 * Returns a string describing the type of the given value.
 *
 * @param argCount Number of arguments.
 * @param args     [value].
 */
static Value native_type_of(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("type_of() takes exactly one argument.");
        return NIL_VAL;
    }
    const char* name = getValueTypeName(args[0]);
    ObjString* result = allocateString(name, (int)strlen(name));
    return STRING_VAL(result);
}

/**
 * Checks if a value's type name matches the provided string.
 *
 * @param argCount Number of arguments.
 * @param args     [value, type_name].
 */
static Value native_is_type(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("is_type() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[1])) {
        vmRuntimeError("Second argument to is_type() must be a string.");
        return NIL_VAL;
    }
    const char* name = getValueTypeName(args[0]);
    ObjString* query = AS_STRING(args[1]);
    bool result = query->length == (int)strlen(name) &&
                  strncmp(query->chars, name, query->length) == 0;
    return BOOL_VAL(result);
}

/**
 * Prompts the user for input and returns the entered string.
 *
 * @param argCount Number of arguments.
 * @param args     [prompt].
 */
static Value native_input(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("input() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0])) {
        vmRuntimeError("input() argument must be a string.");
        return NIL_VAL;
    }
    ObjString* prompt = AS_STRING(args[0]);
    printf("%s", prompt->chars);
    fflush(stdout);
    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return STRING_VAL(allocateString("", 0));
    }
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[--len] = '\0';
    }
    ObjString* result = allocateString(buffer, (int)len);
    return STRING_VAL(result);
}

/**
 * Converts a string to a 32-bit integer.
 *
 * @param argCount Number of arguments.
 * @param args     [string].
 */
static Value native_int(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("int() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0])) {
        vmRuntimeError("int() argument must be a string.");
        return NIL_VAL;
    }
    char* end;
    const char* text = AS_STRING(args[0])->chars;
    long value = strtol(text, &end, 10);
    if (*end != '\0') {
        vmRuntimeError("invalid integer literal.");
        return NIL_VAL;
    }
    if (value < INT32_MIN || value > INT32_MAX) {
        vmRuntimeError("integer value out of range.");
        return NIL_VAL;
    }
    return I32_VAL((int32_t)value);
}

/**
 * Converts a string to a floating point number.
 *
 * @param argCount Number of arguments.
 * @param args     [string].
 */
static Value native_float(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("float() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0])) {
        vmRuntimeError("float() argument must be a string.");
        return NIL_VAL;
    }
    char* end;
    const char* text = AS_STRING(args[0])->chars;
    double value = strtod(text, &end);
    if (*end != '\0') {
        vmRuntimeError("invalid float literal.");
        return NIL_VAL;
    }
    return F64_VAL(value);
}

/**
 * Compute base^exp using the platform math library.
 */
static Value native_pow(int argCount, Value* args) {
    if (argCount != 2 || !IS_F64(args[0]) || !IS_I32(args[1])) {
        vmRuntimeError("native_pow expects (f64, i32).");
        return NIL_VAL;
    }
    double base = AS_F64(args[0]);
    int exp = AS_I32(args[1]);
    return F64_VAL(pow(base, (double)exp));
}

/**
 * Compute the square root using the platform math library.
 */
static Value native_sqrt(int argCount, Value* args) {
    if (argCount != 1 || !IS_F64(args[0])) {
        vmRuntimeError("native_sqrt expects (f64).");
        return NIL_VAL;
    }
    return F64_VAL(sqrt(AS_F64(args[0])));
}

/**
 * Sums the numeric elements of an array.
 *
 * @param argCount Number of arguments.
 * @param args     [array].
 */
static Value native_sum(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("sum() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("sum() expects array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    double total = 0;
    bool asFloat = false;
    for (int i = 0; i < arr->length; i++) {
        Value v = arr->elements[i];
        if (IS_I32(v)) {
            total += AS_I32(v);
        } else if (IS_I64(v)) {
            total += AS_I64(v);
        } else if (IS_U32(v)) {
            total += AS_U32(v);
        } else if (IS_F64(v)) {
            total += AS_F64(v);
            asFloat = true;
        } else {
            vmRuntimeError("sum() array must contain only numbers.");
            return NIL_VAL;
        }
    }
    if (asFloat)
        return F64_VAL(total);
    else
        return I32_VAL((int32_t)total);
}

/**
 * Returns the smallest numeric element of an array.
 *
 * @param argCount Number of arguments.
 * @param args     [array].
 */
static Value native_min(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("min() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("min() expects array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    if (arr->length == 0) return NIL_VAL;

    Value first = arr->elements[0];
    double best;
    bool asFloat = false;
    if (IS_I32(first)) {
        best = AS_I32(first);
    } else if (IS_I64(first)) {
        best = AS_I64(first);
    } else if (IS_U32(first)) {
        best = AS_U32(first);
    } else if (IS_F64(first)) {
        best = AS_F64(first);
        asFloat = true;
    } else {
        vmRuntimeError("min() array must contain only numbers.");
        return NIL_VAL;
    }

    for (int i = 1; i < arr->length; i++) {
        Value v = arr->elements[i];
        double val;
        if (IS_I32(v)) {
            val = AS_I32(v);
        } else if (IS_I64(v)) {
            val = AS_I64(v);
        } else if (IS_U32(v)) {
            val = AS_U32(v);
        } else if (IS_F64(v)) {
            val = AS_F64(v);
            asFloat = true;
        } else {
            vmRuntimeError("min() array must contain only numbers.");
            return NIL_VAL;
        }
        if (val < best) best = val;
    }

    if (asFloat)
        return F64_VAL(best);
    else
        return I32_VAL((int32_t)best);
}

/**
 * Returns the largest numeric element of an array.
 *
 * @param argCount Number of arguments.
 * @param args     [array].
 */
static Value native_max(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("max() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("max() expects array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    if (arr->length == 0) return NIL_VAL;

    Value first = arr->elements[0];
    double best;
    bool asFloat = false;
    if (IS_I32(first)) {
        best = AS_I32(first);
    } else if (IS_I64(first)) {
        best = AS_I64(first);
    } else if (IS_U32(first)) {
        best = AS_U32(first);
    } else if (IS_F64(first)) {
        best = AS_F64(first);
        asFloat = true;
    } else {
        vmRuntimeError("max() array must contain only numbers.");
        return NIL_VAL;
    }

    for (int i = 1; i < arr->length; i++) {
        Value v = arr->elements[i];
        double val;
        if (IS_I32(v)) {
            val = AS_I32(v);
        } else if (IS_I64(v)) {
            val = AS_I64(v);
        } else if (IS_U32(v)) {
            val = AS_U32(v);
        } else if (IS_F64(v)) {
            val = AS_F64(v);
            asFloat = true;
        } else {
            vmRuntimeError("max() array must contain only numbers.");
            return NIL_VAL;
        }
        if (val > best) best = val;
    }

    if (asFloat)
        return F64_VAL(best);
    else
        return I32_VAL((int32_t)best);
}

// ---------- sorted() built-in ----------

// Helper functions for timsort implementation
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Determines whether a value is numeric.
 *
 * @param v Candidate value.
 * @return  True if the value is a number.
 */
static bool isNumber(Value v) {
    return IS_I32(v) || IS_U32(v) || IS_F64(v);
}

/**
 * Converts a numeric Value to a C double.
 *
 * @param v Numeric value.
 */
static double toNumber(Value v) {
    if (IS_I32(v)) return AS_I32(v);
    if (IS_U32(v)) return AS_U32(v);
    return AS_F64(v);
}

/**
 * Compares two values for use in sorting.
 *
 * @param a First value.
 * @param b Second value.
 * @return  Negative if a < b, zero if equal, positive if a > b.
 */
static int compareValues(Value a, Value b) {
    if (isNumber(a) && isNumber(b)) {
        double da = toNumber(a);
        double db = toNumber(b);
        if (da < db) return -1;
        if (da > db) return 1;
        return 0;
    } else if (IS_STRING(a) && IS_STRING(b)) {
        int cmp = strcmp(AS_STRING(a)->chars, AS_STRING(b)->chars);
        if (cmp < 0) return -1;
        if (cmp > 0) return 1;
        return 0;
    } else {
        vmRuntimeError("sorted() array must contain only numbers or strings.");
        return 0;
    }
}

/**
 * Simple insertion sort used by timsort.
 *
 * @param arr     Array of values to sort.
 * @param left    Starting index.
 * @param right   Ending index.
 * @param reverse True to sort descending.
 */
static void insertionSort(Value* arr, int left, int right, bool reverse) {
    for (int i = left + 1; i <= right; i++) {
        Value key = arr[i];
        int j = i - 1;
        while (j >= left && (reverse ? compareValues(arr[j], key) < 0
                                    : compareValues(arr[j], key) > 0)) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/**
 * Merges two sorted halves used by timsort.
 *
 * @param arr     Array of values to merge.
 * @param l       Left index.
 * @param m       Middle index.
 * @param r       Right index.
 * @param temp    Temporary buffer.
 * @param reverse True for descending order.
 */
static void merge(Value* arr, int l, int m, int r, Value* temp, bool reverse) {
    int len1 = m - l + 1;
    int len2 = r - m;
    for (int i = 0; i < len1; i++) temp[i] = arr[l + i];
    for (int i = 0; i < len2; i++) temp[len1 + i] = arr[m + 1 + i];

    int i = 0, j = len1, k = l;
    while (i < len1 && j < len1 + len2) {
        if (reverse ? compareValues(temp[i], temp[j]) >= 0
                     : compareValues(temp[i], temp[j]) <= 0) {
            arr[k++] = temp[i++];
        } else {
            arr[k++] = temp[j++];
        }
    }
    while (i < len1) arr[k++] = temp[i++];
    while (j < len1 + len2) arr[k++] = temp[j++];
}

/**
 * Minimal timsort implementation used by the sorted() builtin.
 *
 * @param arr     Array of values to sort.
 * @param n       Number of elements.
 * @param reverse True to sort in descending order.
 */
static void timSort(Value* arr, int n, bool reverse) {
    const int MIN_RUN = 32;
    for (int i = 0; i < n; i += MIN_RUN) {
        int right = MIN(i + MIN_RUN - 1, n - 1);
        insertionSort(arr, i, right, reverse);
    }

    Value* temp = (Value*)malloc(sizeof(Value) * n);
    for (int size = MIN_RUN; size < n; size *= 2) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = MIN(left + 2 * size - 1, n - 1);
            if (mid < right) {
                merge(arr, left, mid, right, temp, reverse);
            }
        }
    }
    free(temp);
}

/**
 * Returns a sorted copy of an array. A boolean argument may reverse the order.
 *
 * @param argCount Number of arguments.
 * @param args     Array plus optional flags.
 */
static Value native_sorted(int argCount, Value* args) {
    if (argCount < 1 || argCount > 3) {
        vmRuntimeError("sorted() takes between 1 and 3 arguments.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("sorted() first argument must be array.");
        return NIL_VAL;
    }

    bool reverse = false;

    if (argCount == 2) {
        if (IS_BOOL(args[1])) {
            reverse = AS_BOOL(args[1]);
        } else if (!IS_NIL(args[1])) {
            vmRuntimeError("sorted() key function not supported yet.");
            return NIL_VAL;
        }
    } else if (argCount == 3) {
        if (!IS_NIL(args[1])) {
            vmRuntimeError("sorted() key function not supported yet.");
            return NIL_VAL;
        }
        if (!IS_BOOL(args[2])) {
            vmRuntimeError("sorted() third argument must be bool.");
            return NIL_VAL;
        }
        reverse = AS_BOOL(args[2]);
    }

    ObjArray* in = AS_ARRAY(args[0]);
    ObjArray* out = allocateArray(in->length);
    out->length = in->length;
    for (int i = 0; i < in->length; i++) {
        out->elements[i] = in->elements[i];
    }

    timSort(out->elements, out->length, reverse);

    return ARRAY_VAL(out);
}

/**
 * Returns the short name of a previously loaded module.
 *
 * @param argCount Number of arguments.
 * @param args     [module_path].
 */
static Value native_module_name(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        vmRuntimeError("module_name() expects module path string.");
        return NIL_VAL;
    }
    Module* m = get_module(AS_STRING(args[0])->chars);
    if (!m) {
        vmRuntimeError("Module not loaded.");
        return NIL_VAL;
    }
    ObjString* s = allocateString(m->name, (int)strlen(m->name));
    return STRING_VAL(s);
}

/**
 * Returns the current Unix timestamp.
 *
 * @param argCount Number of arguments (must be 0).
 * @param args     Unused.
 */
static Value native_timestamp(int argCount, Value* args) {
    if (argCount != 0) {
        vmRuntimeError("timestamp() takes no arguments.");
        return NIL_VAL;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    double seconds = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    return F64_VAL(seconds);
}

/**
 * Retrieves the canonical path of a loaded module.
 *
 * @param argCount Number of arguments.
 * @param args     [module_path].
 */
static Value native_module_path(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        vmRuntimeError("module_path() expects module path string.");
        return NIL_VAL;
    }
    Module* m = get_module(AS_STRING(args[0])->chars);
    if (!m) {
        vmRuntimeError("Module not loaded.");
        return NIL_VAL;
    }
    ObjString* s = allocateString(m->module_name, (int)strlen(m->module_name));
    return STRING_VAL(s);
}

// ---- Wrapper helpers for register VM ----
Value builtin_range(Value start, Value end) {
    Value args[2] = {start, end};
    return native_range(2, args);
}

Value builtin_sum(Value array) {
    Value args[1] = {array};
    return native_sum(1, args);
}

Value builtin_min(Value array) {
    Value args[1] = {array};
    return native_min(1, args);
}

Value builtin_max(Value array) {
    Value args[1] = {array};
    return native_max(1, args);
}

Value builtin_is_type(Value value, Value type_name) {
    Value args[2] = {value, type_name};
    return native_is_type(2, args);
}

Value builtin_input(Value prompt) {
    Value args[1] = {prompt};
    return native_input(1, args);
}

Value builtin_int(Value text) {
    Value args[1] = {text};
    return native_int(1, args);
}

Value builtin_float(Value text) {
    Value args[1] = {text};
    return native_float(1, args);
}

Value builtin_timestamp(void) {
    return native_timestamp(0, NULL);
}

Value builtin_sorted(Value array, Value key, Value reverse) {
    Value args[3] = {array, key, reverse};
    int argc = 1;
    if (!IS_NIL(key)) argc = 2;
    if (!IS_NIL(reverse)) argc = 3;
    return native_sorted(argc, args);
}

Value builtin_module_name(Value path) {
    Value args[1] = {path};
    return native_module_name(1, args);
}

Value builtin_module_path(Value path) {
    Value args[1] = {path};
    return native_module_path(1, args);
}

Value builtin_native_pow(Value base, Value exp) {
    Value args[2] = {base, exp};
    return native_pow(2, args);
}

Value builtin_native_sqrt(Value value) {
    Value args[1] = {value};
    return native_sqrt(1, args);
}

/**
 * Table entry describing a builtin function.
 */
typedef struct {
    const char* name;
    NativeFn fn;
    int arity;
    TypeKind returnKind; // TYPE_COUNT indicates no specific return type
} BuiltinEntry;

static BuiltinEntry builtinTable[] = {
    {"len", native_len, 1, TYPE_I32},
    {"substring", native_substring, 3, TYPE_STRING},
    {"push", native_push, 2, TYPE_COUNT},
    {"pop", native_pop, 1, TYPE_COUNT},
    {"reserve", native_reserve, 2, TYPE_COUNT},
    {"range", native_range, 2, TYPE_COUNT},
    {"sum", native_sum, 1, TYPE_COUNT},
    {"min", native_min, 1, TYPE_COUNT},
    {"max", native_max, 1, TYPE_COUNT},
    {"type_of", native_type_of, 1, TYPE_STRING},
    {"is_type", native_is_type, 2, TYPE_BOOL},
    {"input", native_input, 1, TYPE_STRING},
    {"int", native_int, 1, TYPE_I32},
    {"float", native_float, 1, TYPE_F64},
    {"timestamp", native_timestamp, 0, TYPE_F64},
    {"sorted", native_sorted, -1, TYPE_ARRAY},
    {"module_name", native_module_name, 1, TYPE_STRING},
    {"module_path", native_module_path, 1, TYPE_STRING},
    {"native_pow", native_pow, 2, TYPE_F64},
    {"native_sqrt", native_sqrt, 1, TYPE_F64},
};

/**
 * Registers all builtin functions with the VM.
 */
void initBuiltins(void) {
    size_t count = sizeof(builtinTable) / sizeof(BuiltinEntry);
    for (size_t i = 0; i < count; i++) {
        Type* ret = builtinTable[i].returnKind == TYPE_COUNT
                         ? NULL
                         : getPrimitiveType(builtinTable[i].returnKind);
        defineNative(builtinTable[i].name,
                     builtinTable[i].fn,
                     builtinTable[i].arity,
                     ret);
    }
}

