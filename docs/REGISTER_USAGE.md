# Register Usage Conventions

To keep register allocation simple during early development the register VM follows
a fixed usage convention. This allows later phases to implement a real allocator
while still keeping code readable.

| Register Range | Purpose        |
| -------------- | -------------- |
| R0–R15        | Temporaries    |
| R16–R63       | Local vars     |
| R64+           | Builtins / VM  |

The compiler currently adheres to this convention when assigning registers for
basic arithmetic. Future optimization passes can replace this simple scheme with
a true register allocator.
