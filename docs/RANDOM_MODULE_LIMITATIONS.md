# Limitations in Implementing std/random

Prior versions of Orus lacked global mutable variables, blocking an implementation of `std/random`. With the addition of `static mut` declarations the module can now keep state between calls.

The `std/random` module uses a simple linear congruential generator. When the module is loaded it seeds the generator with the current UNIX timestamp so values vary between program runs. Call `set_seed()` to override this behaviour for reproducible results. Functions `random()`, `randint`, `uniform`, `choice`, `sample` and `shuffle` provide basic random utilities without external dependencies.
