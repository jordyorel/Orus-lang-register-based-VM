# Limitations in Implementing std/random

Prior versions of Orus lacked global mutable variables, blocking an implementation of `std/random`. With the addition of `static mut` declarations the module can now keep state between calls.

The `std/random` module uses a simple linear congruential generator. The state is seeded with the current timestamp on first use so that calls return different values across runs. Functions `rand()`, `rand_int`, `choice`, and `shuffle` provide basic random utilities without external dependencies.
