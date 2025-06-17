# Environment Variables

The interpreter recognises several optional environment variables that
control runtime behaviour. Set them in your shell before invoking
`orusc`.

| Variable | Purpose |
| -------- | ------- |
| `ORUS_TRACE` | If set, prints executed bytecode instructions and GC events. |
| `ORUS_PATH` | Location of the standard library directory. Defaults to `./std`. |
| `ORUS_CACHE_PATH` | Directory for cached `.obc` bytecode files. |
| `ORUS_DEV_MODE` | When non-empty, reloads modules if their source changes. |

All variables are optional and can be left unset for default behaviour.
