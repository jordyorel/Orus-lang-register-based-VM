#ifndef ORUS_BUILTINS_H
#define ORUS_BUILTINS_H

#include "register_vm.h"

void initBuiltins(void);

// Wrapper helpers used by the register VM
Value builtin_range(Value start, Value end);
Value builtin_sum(Value array);
Value builtin_min(Value array);
Value builtin_max(Value array);
Value builtin_is_type(Value value, Value type_name);
Value builtin_input(Value prompt);
Value builtin_int(Value text);
Value builtin_float(Value text);
Value builtin_timestamp(void);
Value builtin_sorted(Value array, Value key, Value reverse);
Value builtin_module_name(Value path);
Value builtin_module_path(Value path);
Value builtin_native_pow(Value base, Value exp);
Value builtin_native_sqrt(Value value);

#endif // ORUS_BUILTINS_H
