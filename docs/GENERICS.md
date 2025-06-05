# Generic Data Structures

The C implementation uses a small macro based system to define dynamic arrays
for any type. A single invocation of `DEFINE_ARRAY_TYPE(T, Name)` creates the
`NameArray` struct along with inline `initNameArray`, `writeNameArray` and
`freeNameArray` helpers.

This approach allows reusable collections without relying on inheritance or
manual repetition. `ValueArray` in the virtual machine is now implemented using
this macro.
