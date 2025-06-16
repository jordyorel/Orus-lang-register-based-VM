# Example Code Snippets

This page collects small code samples demonstrating common language
features. Each snippet can be executed with the Orus interpreter.

## Looping Over a Range

```orus
fn main() {
    for i in range(0, 5) {
        print(i)
    }
}
```

## Generic Function

```orus
fn identity<T>(value: T) -> T {
    return value
}
```

## Dynamic Arrays

```orus
fn demo() {
    let mut nums = [1, 2, 3]
    push(nums, 4)
    print(len(nums))
}
```
