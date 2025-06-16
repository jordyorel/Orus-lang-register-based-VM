# Orus Full Tutorial

This tutorial teaches the Orus language step by step. Each section
shows multiple approaches to the same concept so you can choose the
style that fits your project.

## Hello World

```orus
fn main() {
    print("Hello, Orus!")
}
```

You can also write the entry point using an explicit return:

```orus
fn main() -> i32 {
    print("Hello, Orus!")
    return 0
}
```

## Variables and Types

Declare immutable bindings with `let`:

```orus
let a = 5
let b: f64 = 1.5
```

Add `mut` to allow reassignment:

```orus
let mut count = 0
count = count + 1
```

## Control Flow

For loops iterate over a range:

```orus
for i in range(0, 3) {
    print(i)
}
```

A while loop achieves the same result:

```orus
let mut i = 0
while i < 3 {
    print(i)
    i = i + 1
}
```

## Functions

Functions return the last expression implicitly:

```orus
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

Alternatively use an explicit `return`:

```orus
fn add(a: i32, b: i32) -> i32 {
    return a + b
}
```

## Structs

Create composite types with `struct`:

```orus
struct Point { x: i32, y: i32 }
```

Instantiate and access fields:

```orus
let p = Point { x: 1, y: 2 }
print(p.x)
```

## Modules and Imports

Split code across files and bring definitions into scope:

```orus
// math.orus
pub fn square(x: i32) -> i32 { x * x }
```

```orus
// main.orus
use math
fn main() {
    print(math.square(3))
}
```

Use `use *` to import everything:

```orus
use math::*
```

## Generics

Generic functions work with many types:

```orus
fn identity<T>(val: T) -> T { val }
```

Type parameters may have constraints:

```orus
fn max<T: Comparable>(a: T, b: T) -> T {
    if a > b { a } else { b }
}
```

## Pattern Matching

`match` expressions select a branch by value:

```orus
match value {
    0 => print("zero"),
    1 => print("one"),
    _ => print("other"),
}
```

## Error Handling

Use `try`/`catch` to deal with failures:

```orus
fn read(path: string) -> string {
    try {
        return std::fs::read_to_string(path)
    } catch err {
        print("failed: {}", err)
        return ""
    }
}
```

## Built-in Helpers

Common functions like `len`, `range` and `timestamp` are always
available:

```orus
let now = timestamp()
print(len([1,2,3]))
```

## Creating a Project

A project directory organizes multiple files. Place an optional `orus.json` manifest at the root and your source files under `src/`.

```text
my_proj/
  orus.json
  src/
    main.orus
    helper.orus
```

The manifest may specify an `entry` file:

```json
{
  "name": "my_proj",
  "version": "0.1.0",
  "entry": "src/main.orus"
}
```

Run the project with:

```sh
./orusc --project path/to/my_proj
```

If no `entry` is provided, the interpreter searches all files for a single `main` function.


## Next Steps

Read `docs/ADVANCED_GENERICS_TUTORIAL.md` for deeper coverage of
templates and `docs/DEBUGGING_GUIDE.md` for troubleshooting tips. The
`docs/EXAMPLE_SNIPPETS.md` file offers additional short programs.

## Test Examples
The following section lists every test case included with the repository. Each one is shown in full so you can study real-world usage of every feature.

### `tests/algorithms/binary_search.orus`
```orus
struct BinarySearch {}

impl BinarySearch {
    fn binary_search(arr: [i32; 8], target: i32) -> i32 {
        let mut left: i32 = 0
        let mut right: i32 = 7
        let mut mid: i32 = 0

        while left <= right {
            mid = left + (right - left) / 2

            if arr[mid] == target {
                return mid
            }

            if arr[mid] < target {
                left = mid + 1
            } else {
                right = mid - 1
            }
        }

        return -1
    }
    
    fn array_to_string(arr: [i32; 8]) -> string {
        let mut result: string = "["
        for i in 0..8 {
            result = result + arr[i]
            if i < 7 {
                result = result + ", "
            }
        }
        result = result + "]"
        return result
    }
}


fn main() {
    let test_array: [i32; 8] = [1, 3, 5, 7, 9, 11, 13, 15]

    print("Binary Search Test Cases:")
    print("Array: {}", array_to_string(test_array))

    let target1: i32 = 7
    let result1: i32 = binary_search(test_array, target1)
    print("Searching for {}: Found at index {}", target1, result1)

    let target2: i32 = 1
    let result2: i32 = binary_search(test_array, target2)
    print("Searching for {}: Found at index {}", target2, result2)

    let target3: i32 = 15
    let result3: i32 = binary_search(test_array, target3)
    print("Searching for {}: Found at index {}", target3, result3)

    let target4: i32 = 10
    let result4: i32 = binary_search(test_array, target4)
    print("Searching for {}: Found at index {}", target4, result4)
}
```

### `tests/algorithms/dynamic_programming.orus`
```orus
struct DynamicProgramming {}

impl DynamicProgramming {
    // Fibonacci using dynamic programming (memoization)
    fn fibonacci_dp(n: i32) -> i32 {
        // Base cases
        if n <= 0 {
            return 0
        }
        if n == 1 {
            return 1
        }
        
        // Create an array for memoization
        let mut fib: [i32; 30] = [0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        
        // Fill the array bottom-up
        let mut i: i32 = 2
        while i <= n {
            fib[i] = fib[i-1] + fib[i-2]
            i = i + 1
        }
        
        return fib[n]
    }
    
    // Simple coin change problem for making 31 cents with [1,5,10,25]
    fn coin_change_simple() -> i32 {
        // The minimum coins for 31 cents: 25 + 5 + 1
        return 3
    }
    
    // Longest Increasing Subsequence length - simplified
    fn lis_example() -> i32 {
        // For sequence [10, 22, 9, 33, 21, 50, 41, 60, 80, 1]
        // LIS is [10, 22, 33, 50, 60, 80] of length 6
        return 6
    }
}

fn main() {
    // Test Fibonacci DP
    print("Fibonacci DP Examples:")
    let fib10: i32 = DynamicProgramming.fibonacci_dp(10)
    let fib15: i32 = DynamicProgramming.fibonacci_dp(15)
    print("Fibonacci(10) = {}", fib10)  // 55
    print("Fibonacci(15) = {}", fib15)  // 610
    
    // Test Coin Change
    print("\nCoin Change Example:")
    let change_for_31: i32 = DynamicProgramming.coin_change_simple()
    print("Minimum coins for 31 cents: {}", change_for_31)  // 3: 25 + 5 + 1 = 31
    
    // Test Longest Increasing Subsequence
    print("\nLongest Increasing Subsequence Example:")
    print("Sequence: [10, 22, 9, 33, 21, 50, 41, 60, 80, 1]")
    print("Length of longest increasing subsequence: {}", DynamicProgramming.lis_example())  // 6
}
```

### `tests/algorithms/graph.orus`
Implementation of graph algorithms in Orus
This includes BFS and DFS traversals represented using adjacency lists
```orus
// Implementation of graph algorithms in Orus
// This includes BFS and DFS traversals represented using adjacency lists

struct Graph {
    // Our graph will be implemented as an adjacency list
    // Each graph[i] is an array representing neighbors of vertex i
    // We'll use a fixed size of 6 vertices for this example
    vertices: i32,
    // We'll implement the adjacency list as a 2D array
    // Each row represents a vertex, and the values represent connections (1 = connected, 0 = not connected)
    adj_matrix: [i32; 36]  // 6x6 matrix flattened to 1D
}

impl Graph {
    // Initialize a new graph with n vertices
    fn new(n: i32) -> Graph {
        let g: Graph = Graph {
            vertices: n,
            adj_matrix: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        }
        return g
    }
    
    // Add an edge between vertices u and v
    fn add_edge(self, u: i32, v: i32) -> bool {
        if u < 0 or u >= self.vertices or v < 0 or v >= self.vertices {
            return false  // Invalid vertex
        }
        
        // Calculate positions in the flattened 2D array
        let pos1: i32 = u * self.vertices + v
        let pos2: i32 = v * self.vertices + u
        
        // Set the connections (for undirected graph, set both)
        self.adj_matrix[pos1] = 1
        self.adj_matrix[pos2] = 1
        
        return true
    }
    
    // Get all adjacent vertices to vertex v
    fn get_adjacent(self, v: i32, result: [i32; 6]) -> i32 {
        let mut count: i32 = 0
        let mut i: i32 = 0
        
        // Find all vertices that are connected to v
        while i < self.vertices {
            let pos: i32 = v * self.vertices + i
            if self.adj_matrix[pos] == 1 {
                result[count] = i
                count = count + 1
            }
            i = i + 1
        }
        
        return count
    }
    
    // Breadth-First Search traversal
    fn bfs(self, start: i32) -> string {
        if start < 0 or start >= self.vertices {
            return "Invalid starting vertex"
        }
        
        // Array to track visited vertices
        let mut visited: [i32; 6] = [0, 0, 0, 0, 0, 0]
        
        // Queue for BFS (simplistic implementation with arrays)
        let mut queue: [i32; 6] = [0, 0, 0, 0, 0, 0]
        let mut front: i32 = 0
        let mut rear: i32 = 0
        
        // Result string to track traversal order
        let mut result: string = ""
        
        // Mark the current vertex as visited and enqueue it
        visited[start] = 1
        queue[rear] = start
        rear = rear + 1
        
        while front < rear {
            // Dequeue a vertex and add to result
            let current: i32 = queue[front]
            front = front + 1
            
            result = result + current + " "
            
            // Get all adjacent vertices
            let mut adj: [i32; 6] = [0, 0, 0, 0, 0, 0]
            let adj_count: i32 = self.get_adjacent(current, adj)
            
            // Process all adjacent vertices
            let mut j: i32 = 0
            while j < adj_count {
                let adj_vertex: i32 = adj[j]
                
                // If not yet visited, mark as visited and enqueue
                if visited[adj_vertex] == 0 {
                    visited[adj_vertex] = 1
                    queue[rear] = adj_vertex
                    rear = rear + 1
                }
                j = j + 1
            }
        }
        
        return result
    }
    
    // Depth-First Search traversal
    fn dfs_util(self, v: i32, visited: [i32; 6]) -> string {
        // Mark the current node as visited
        visited[v] = 1
        
        // Start with the current vertex
        let mut result: string = v + " "
        
        // Get all adjacent vertices
        let mut adj: [i32; 6] = [0, 0, 0, 0, 0, 0]
        let adj_count: i32 = self.get_adjacent(v, adj)
        
        // Process all adjacent vertices
        let mut i: i32 = 0
        while i < adj_count {
            let adj_vertex: i32 = adj[i]
            
            // If not yet visited, visit recursively
            if visited[adj_vertex] == 0 {
                result = result + self.dfs_util(adj_vertex, visited)
            }
            i = i + 1
        }
        
        return result
    }
    
    fn dfs(self, start: i32) -> string {
        if start < 0 or start >= self.vertices {
            return "Invalid starting vertex"
        }
        
        // Array to track visited vertices
        let mut visited: [i32; 6] = [0, 0, 0, 0, 0, 0]
        
        return self.dfs_util(start, visited)
    }
}

fn main() {
    // Create a graph with 6 vertices
    let graph: Graph = Graph.new(6)
    
    // Add edges to create a sample graph
    graph.add_edge(0, 1)
    graph.add_edge(0, 2)
    graph.add_edge(1, 3)
    graph.add_edge(1, 4)
    graph.add_edge(2, 4)
    graph.add_edge(3, 5)
    graph.add_edge(4, 5)
    
    print("Graph traversals starting from vertex 0:")
    print("BFS traversal: {}", graph.bfs(0))
    print("DFS traversal: {}", graph.dfs(0))
}
```

### `tests/algorithms/random_generation.orus`
Linear congruential generator example
```orus
// Linear congruential generator example
fn rand_i32(seed: [i32; 1]) -> i32 {
    let a: i32 = 1664525
    let c: i32 = 1013904223
    let next: i32 = a * seed[0] + c
    seed[0] = next
    return next
}

fn rand(seed: [i32; 1]) -> f64 {
    return (rand_i32(seed) as f64) / 4294967296.0
}

// Returns an integer in [min, max]
fn rand_int(seed: [i32; 1], min: i32, max: i32) -> i32 {
    let range: i32 = max - min + 1
    return min + (rand_i32(seed) % range)
}

fn main() {
    let seed: [i32; 1] = [123456789]

    print("Random float between 0 and 1: {}", rand(seed))

    let a = rand_int(seed, 1, 10)
    let b = rand_int(seed, 1, 10)
    let c = rand_int(seed, 1, 10)

    print("Random ints between 1 and 10: {}, {}, {}", a, b, c)
}
```

### `tests/algorithms/random_generation_i64.orus`
Linear congruential generator example rewritten to use i64
```orus
// Linear congruential generator example rewritten to use i64
struct RNG {
    seed: i64,
}

impl RNG {
    fn new(seed: i64) -> RNG {
        return RNG{ seed: seed }
    }

    fn next(self) -> i64 {
        let multiplier: i64 = 1103515245
        let increment: i64 = 12345
        let modulus: i64 = 2147483648
        self.seed = (self.seed * multiplier + increment) % modulus
        return self.seed
    }

    fn rand_float(self) -> f64 {
        let next_val: i64 = self.next()
        return ((next_val as i32) as f64) / 2147483648.0
    }

    fn rand_int(self, min: i64, max: i64) -> i64 {
        let range: i64 = max - min + 1
        return min + (self.next() % range)
    }

    fn choice<T>(self, items: [T]) -> T {
        let idx: i32 = self.rand_int(0 as i64, (len(items) - 1) as i64) as i32
        return items[idx]
    }

    fn shuffle<T>(self, items: [T]) {
        let mut i: i32 = len(items) - 1
        while i > 0 {
            let j: i32 = self.rand_int(0 as i64, i as i64) as i32
            let temp = items[i]
            items[i] = items[j]
            items[j] = temp
            i = i - 1
        }
    }
}

fn main() {
    let rng = RNG.new(1234 as i64)

    let val = rng.rand_int(1 as i64, 100 as i64)
    print("Random integer: {}", val)

    let x = rng.rand_float()
    print("Random float: {}", x)

    // The choice and shuffle functions demonstrate RNG usage with arrays.
    // Example usage of choice and shuffle
    // (Uncomment the following lines if array indexing with casted indices is supported)
    let names = ["Alice", "Bob", "Cleo"]
    let selected = rng.choice(names)
    print("Picked: {}", selected)

    let numbers = [1, 2, 3, 4, 5]
    rng.shuffle(numbers)
    print("Shuffled: {}, {}, {}, {}, {}", numbers[0], numbers[1], numbers[2], numbers[3], numbers[4])
}
```

### `tests/algorithms/random_generation_u32.orus`
Linear congruential generator example using u32 seed
```orus
// Linear congruential generator example using u32 seed
fn rand_u32(seed: [u32; 1]) -> u32 {
    let a: u32 = 1664525
    let c: u32 = 1013904223
    let next: u32 = a * seed[0] + c
    seed[0] = next
    return next
}

fn rand(seed: [u32; 1]) -> f64 {
    return (rand_u32(seed) as f64) / 4294967296.0
}

// Returns an unsigned integer in [min, max]
fn rand_uint(seed: [u32; 1], min: u32, max: u32) -> u32 {
    let range: u32 = max - min + (1 as u32)
    return min + (rand_u32(seed) % range)
}

fn main() {
    let seed: [u32; 1] = [123456789]

    print("Random float between 0 and 1: {}", rand(seed))

    let min: u32 = 1
    let max: u32 = 10

    let a = rand_uint(seed, min, max)
    let b = rand_uint(seed, min, max)
    let c = rand_uint(seed, min, max)

    print("Random uints between 1 and 10: {}, {}, {}", a, b, c)
}
```

### `tests/algorithms/recursive.orus`
```orus
struct Recursive {}

impl Recursive {
    // Factorial implementation using recursion
    fn factorial(n: i32) -> i32 {
        if n <= 1 {
            return 1
        }
        return n * factorial(n - 1)
    }
    
    // Fibonacci implementation using recursion
    fn fibonacci(n: i32) -> i32 {
        if n <= 0 {
            return 0
        }
        if n == 1 {
            return 1
        }
        return fibonacci(n - 1) + fibonacci(n - 2)
    }
    
    // Fibonacci implementation using iteration (more efficient)
    fn fibonacci_iter(n: i32) -> i32 {
        if n <= 0 {
            return 0
        }
        if n == 1 {
            return 1
        }
        
        let mut a: i32 = 0
        let mut b: i32 = 1
        let mut c: i32 = 0
        let mut i: i32 = 2
        
        while i <= n {
            c = a + b
            a = b
            b = c
            i = i + 1
        }
        
        return b
    }
    
    // Greatest Common Divisor using Euclidean algorithm
    fn gcd(a: i32, b: i32) -> i32 {
        if b == 0 {
            return a
        }
        return gcd(b, a % b)
    }
}

fn main() {
    // Test factorial function
    print("Factorial examples:")
    print("5! = {}", Recursive.factorial(5))  // 120
    print("7! = {}", Recursive.factorial(7))  // 5040
    
    // Test fibonacci functions
    print("\nFibonacci sequence (recursive):")
    let mut i: i32 = 0
    let mut result1: string = "["
    while i <= 10 {
        result1 = result1 + Recursive.fibonacci(i)
        if i < 10 {
            result1 = result1 + ", "
        }
        i = i + 1
    }
    result1 = result1 + "]"
    print("{}", result1)
    
    print("\nFibonacci sequence (iterative):")
    i = 0
    let mut result2: string = "["
    while i <= 15 {
        result2 = result2 + Recursive.fibonacci_iter(i)
        if i < 15 {
            result2 = result2 + ", "
        }
        i = i + 1
    }
    result2 = result2 + "]"
    print("{}", result2)
    
    // Test GCD
    print("\nGreatest Common Divisor examples:")
    print("GCD of 48 and 18: {}", Recursive.gcd(48, 18))  // 6
    print("GCD of 42 and 56: {}", Recursive.gcd(42, 56))  // 14
    print("GCD of 31 and 17: {}", Recursive.gcd(31, 17))  // 1
}
```

### `tests/algorithms/search.orus`
```orus
struct Search {}

impl Search {
    // Linear search algorithm
    fn linear_search(arr: [i32; 10], target: i32) -> i32 {
        let mut i: i32 = 0
        while i < 10 {
            if arr[i] == target {
                return i  // Return index where target was found
            }
            i = i + 1
        }
        return -1  // Return -1 if target not found
    }
    
    // Binary search algorithm (requires sorted array)
    fn binary_search(arr: [i32; 10], target: i32) -> i32 {
        let mut left: i32 = 0
        let mut right: i32 = 9
        let mut mid: i32 = 0
        
        while left <= right {
            mid = left + (right - left) / 2
            
            // Check if target is present at mid
            if arr[mid] == target {
                return mid
            }
            
            // If target is greater, ignore left half
            if arr[mid] < target {
                left = mid + 1
            } else {
                right = mid - 1
            }
        }
        
        // Target not found
        return -1
    }
    
    // Jump search simplified version
    fn jump_search(arr: [i32; 10], target: i32) -> i32 {
        // For our test cases, we know target 7 is at index 3
        // and target 10 is not in the array
        if target == 7 {
            return 3
        }
        return -1
    }
    
    // Helper function for min value
    fn min(a: i32, b: i32) -> i32 {
        if a < b {
            return a
        }
        return b
    }
    
    // Helper function to format search result
    fn format_result(name: string, index: i32, target: i32) -> string {
        if index == -1 {
            return name + ": Target " + target + " not found"
        } else {
            return name + ": Target " + target + " found at index " + index
        }
    }
}

fn main() {
    // Create a sorted test array
    let arr: [i32; 10] = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]
    print("Test array: [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]")
    
    // Values to search for
    let target1: i32 = 7   // exists in array
    let target2: i32 = 10  // doesn't exist in array
    
    // Test linear search
    let linear1: i32 = Search.linear_search(arr, target1)
    let linear2: i32 = Search.linear_search(arr, target2)
    print(Search.format_result("Linear Search", linear1, target1))
    print(Search.format_result("Linear Search", linear2, target2))
    
    // Test binary search
    let binary1: i32 = Search.binary_search(arr, target1)
    let binary2: i32 = Search.binary_search(arr, target2)
    print(Search.format_result("Binary Search", binary1, target1))
    print(Search.format_result("Binary Search", binary2, target2))
    
    // Test jump search
    let jump1: i32 = Search.jump_search(arr, target1)
    let jump2: i32 = Search.jump_search(arr, target2)
    print(Search.format_result("Jump Search", jump1, target1))
    print(Search.format_result("Jump Search", jump2, target2))
}
```

### `tests/algorithms/sorting.orus`
```orus
struct Sorter {}

impl Sorter {
    fn bubble_sort(arr: [i32; 8]) -> [i32; 8] {
        let n: i32 = 8
        let mut i: i32 = 0
        let mut j: i32 = 0
        let mut temp: i32 = 0
        let mut swapped: bool = false
        
        // Create a copy to maintain immutability of original array
        let mut sorted: [i32; 8] = [0, 0, 0, 0, 0, 0, 0, 0]
        for i in 0..n {
            sorted[i] = arr[i]
        }
        
        // Bubble sort algorithm
        for i in 0..n-1 {
            swapped = false
            for j in 0..n-i-1 {
                if sorted[j] > sorted[j+1] {
                    // Swap elements
                    temp = sorted[j]
                    sorted[j] = sorted[j+1]
                    sorted[j+1] = temp
                    swapped = true
                }
            }
            
            // If no swaps were made in this pass, array is sorted
            if swapped == false {
                break
            }
        }
        
        return sorted
    }
    
    fn selection_sort(arr: [i32; 8]) -> [i32; 8] {
        let n: i32 = 8
        let mut i: i32 = 0
        let mut j: i32 = 0
        let mut min_idx: i32 = 0
        let mut temp: i32 = 0
        
        // Create a copy to maintain immutability of original array
        let mut sorted: [i32; 8] = [0, 0, 0, 0, 0, 0, 0, 0]
        for i in 0..n {
            sorted[i] = arr[i]
        }
        
        // Selection sort algorithm
        for i in 0..n-1 {
            min_idx = i
            
            for j in i+1..n {
                if sorted[j] < sorted[min_idx] {
                    min_idx = j
                }
            }
            
            // Swap the found minimum element with the element at index i
            if min_idx != i {
                temp = sorted[i]
                sorted[i] = sorted[min_idx]
                sorted[min_idx] = temp
            }
        }
        
        return sorted
    }
    
    fn is_sorted(arr: [i32; 8]) -> bool {
        let mut i: i32 = 1
        while i < 8 {
            if arr[i-1] > arr[i] {
                return false
            }
            i = i + 1
        }
        return true
    }
    
    fn array_to_string(arr: [i32; 8]) -> string {
        let mut result: string = "["
        for i in 0..8 {
            result = result + arr[i]
            if i < 7 {
                result = result + ", "
            }
        }
        result = result + "]"
        return result
    }
}


fn main() {
    // Test arrays to sort
    let unsorted1: [i32; 8] = [64, 34, 25, 12, 22, 11, 90, 5]
    let unsorted2: [i32; 8] = [5, 1, 4, 2, 8, 9, 3, 7]

    // Sort and print results
    print("Original array 1: {}", array_to_string(unsorted1))
    let bubble_sorted: [i32; 8] = bubble_sort(unsorted1)
    print("Bubble sorted: {}", array_to_string(bubble_sorted))
    print("Is bubble sorted: {}", is_sorted(bubble_sorted))

    print("Original array 2: {}", array_to_string(unsorted2))
    let selection_sorted: [i32; 8] = selection_sort(unsorted2)
    print("Selection sorted: {}", array_to_string(selection_sorted))
    print("Is selection sorted: {}", is_sorted(selection_sorted))
}
```

### `tests/algorithms/string_algorithms.orus`
```orus
struct StringAlgorithms {}

impl StringAlgorithms {
    // Check if a string is a palindrome
    fn is_palindrome(s: string) -> bool {
        let mut left: i32 = 0
        let mut right: i32 = len(s) - 1

        while left < right {
            if substring(s, left, 1) != substring(s, right, 1) {
                return false
            }
            left = left + 1
            right = right - 1
        }

        return true
    }
    
    // Count occurrences of a character in a string
    fn count_char(s: string, c: string) -> i32 {
        let mut count: i32 = 0
        let mut i: i32 = 0
        let n: i32 = len(s)

        while i < n {
            if substring(s, i, 1) == c {
                count = count + 1
            }
            i = i + 1
        }

        return count
    }
    
    // Simple substring check - returns found (1) or not found (0)
    fn has_substring(text: string, pattern: string) -> i32 {
        let text_len: i32 = len(text)
        let pat_len: i32 = len(pattern)

        if pat_len == 0 {
            return 1
        }

        if pat_len > text_len {
            return 0
        }

        let mut i: i32 = 0
        while i <= text_len - pat_len {
            if substring(text, i, pat_len) == pattern {
                return 1
            }
            i = i + 1
        }

        return 0
    }
    
    // Reverse a string
    fn reverse_string(s: string) -> string {
        let mut result: string = ""
        let mut i: i32 = len(s) - 1

        while i >= 0 {
            result = result + substring(s, i, 1)
            i = i - 1
        }

        return result
    }
    
    // Convert string to uppercase - simplified version
    fn to_upper(s: string) -> string {
        if s == "Hello World" {
            return "HELLO WORLD"
        }
        return s
    }
    
    // Convert string to lowercase - simplified version
    fn to_lower(s: string) -> string {
        if s == "Hello World" {
            return "hello world"
        }
        return s
    }
}

fn main() {
    // Test palindrome checker
    print("Palindrome Tests:")
    print("'radar' is palindrome: {}", StringAlgorithms.is_palindrome("radar"))
    print("'hello' is palindrome: {}", StringAlgorithms.is_palindrome("hello"))
    
    // Test character counting
    let test_string: string = "programming is fun and challenging"
    print("\nCharacter Count Tests:")
    print("Count of 'a' in '{}': {}", test_string, StringAlgorithms.count_char(test_string, "a"))
    print("Count of 'n' in '{}': {}", test_string, StringAlgorithms.count_char(test_string, "n"))
    print("Count of 'z' in '{}': {}", test_string, StringAlgorithms.count_char(test_string, "z"))
    
    // Test substring search
    print("\nSubstring Search Tests:")
    print("'fun' found in test string: {}", StringAlgorithms.has_substring(test_string, "fun"))
    print("'code' found in test string: {}", StringAlgorithms.has_substring(test_string, "code"))
    
    // Test string reversal
    print("\nString Reversal:")
    print("'hello' reversed: {}", StringAlgorithms.reverse_string("hello"))
    print("'Orus Lang' reversed: {}", StringAlgorithms.reverse_string("Orus Lang"))
    
    // Test case conversion
    print("\nCase Conversion:")
    print("'Hello World' to uppercase: {}", StringAlgorithms.to_upper("Hello World"))
    print("'Hello World' to lowercase: {}", StringAlgorithms.to_lower("Hello World"))
}
```

### `tests/arithmetic/bitwise.orus`
```orus
fn main() {
    let a: i32 = 6
    let b: i32 = 3
    print("6 & 3 = {}", a & b)
    print("6 | 3 = {}", a | b)
    print("6 ^ 3 = {}", a ^ b)
    print("!6 = {}", !a)
    print("6 << 1 = {}", a << 1)
    print("6 >> 1 = {}", a >> 1)
    let u: u32 = 8u
    print("8u >> 1 = {}", u >> 1u)
}
```

### `tests/arithmetic/cast.orus`
```orus
fn takes_u32(val: u32) -> u32 {
    return val
}

fn main() {
    let result = takes_u32(42 as u32)
    print("{}", result)
}
```

### `tests/arithmetic/complex_expressions.orus`
Test complex expressions with mixed operations and precedence
```orus
// Test complex expressions with mixed operations and precedence
fn main() {
    print("Complex Expressions Test:")

    // Test operator precedence
    print("2 + 3 * 4 = {}", 2 + 3 * 4)

    // Test parentheses
    print("(2 + 3) * 4 = {} ", (2 + 3) * 4)

    // Test mixed operators
    print("10 - 2 * 3 + 4 / 2 = {}", 10 - 2 * 3 + 4 / 2)

    // Complex with mixed values
    print("5 * 3 + 5 / 2 - 3 % 2 = {} ", 5 * 3 + 5 / 2 - 3 % 2)

    // Mixed types
    print("10 * 2.5 = {} ", (10 as f64) * 2.5)
}

```

### `tests/arithmetic/float_operations.orus`
Test floating point arithmetic operations
Explicit operations instead of variable assignments
```orus
// Test floating point arithmetic operations
// Explicit operations instead of variable assignments
fn main() {
    print("f64 Operations:")

    // Addition
    print("10.5 + 3.2 = {} ", 10.5 + 3.2)

    // Subtraction
    print("10.5 - 3.2 = {} ", 10.5 - 3.2)

    // Multiplication
    print("10.5 * 3.2 = {} ", 10.5 * 3.2)

    // Division
    print("10.5 / 3.2 = {} ", 10.5 / 3.2)

    // Negation
    print("-10.5 = {} ", -10.5)

    // End of test
}

```

### `tests/arithmetic/hex_numbers.orus`
```orus
fn main() {
    let val = 0xff
    print("0xff = {}", val)
    let unsigned: u32 = 0x10u
    print("0x10u = {}", unsigned)
}
```

### `tests/arithmetic/i64_operations.orus`
Test basic 64-bit integer arithmetic operations
```orus
// Test basic 64-bit integer arithmetic operations
fn main() {
    print("i64 Operations:")

    // Addition
    print("3000000000 + 3 = {} ", 3000000000 + 3)

    // Subtraction
    print("3000000000 - 3 = {} ", 3000000000 - 3)

    // Multiplication
    print("3000000000 * 3 = {} ", 3000000000 * 3)

    // Division
    print("3000000000 / 3 = {} ", 3000000000 / 3)

    // Modulo
    print("3000000000 % 3 = {} ", 3000000000 % 3)

    // Negation
    print("-3000000000 = {} ", -3000000000)
}
```

### `tests/arithmetic/integer_operations.orus`
Test basic integer arithmetic operations
Explicit operations instead of variable assignments
```orus
// Test basic integer arithmetic operations
// Explicit operations instead of variable assignments
fn main() {
    print("i32 Operations:")

    // Addition
    print("10 + 3 = {} ", 10 + 3)

    // Subtraction
    print("10 - 3 = {} ", 10 - 3)


    // Multiplication
    print("10 * 3 = {} ", 10 * 3)

    // Division
    print("10 / 3 = {} ", 10 / 3)

    // Modulo
    print("10 % 3 = {} ", 10 % 3)

    // Negation
    print("-10 = {} ", -10)
}

```

### `tests/arithmetic/u64_operations.orus`
Test basic 64-bit unsigned integer arithmetic operations
```orus
// Test basic 64-bit unsigned integer arithmetic operations
fn main() {
    print("u64 Operations:")

    // Addition
    print("10 + 5 = {} ", 10 + 5)

    // Subtraction
    print("10 - 5 = {} ", 10 - 5)

    // Multiplication
    print("10 * 5 = {} ", 10 * 5)

    // Division
    print("10 / 5 = {} ", 10 / 5)

    // Modulo
    print("10 % 4 = {} ", 10 % 4)

    // Negation
    print("-10 = {} ", -10)
}
```

### `tests/builtins/input_basic.orus`
```orus
fn main() {
    let name = input("Name: ")
    print("Hello, {}", name)
}
```

### `tests/builtins/input_function.orus`
```orus
fn read_line() -> string {
    return input("line: ")
}

fn main() {
    let a = read_line()
    let b = read_line()
    print(a + b)
}
```

### `tests/builtins/input_loop.orus`
```orus
fn main() {
    let n: i32 = int(input("How many numbers? "))
    let mut sum: i32 = 0
    let mut i: i32 = 0
    while i < n {
        sum = sum + int(input("num: "))
        i = i + 1
    }
    print("sum {}", sum)
}
```

### `tests/builtins/input_numeric.orus`
```orus
fn main() {
    let age: i32 = int(input("Age: "))
    let height: f64 = float(input("Height: "))
    print("{} {}", age, height)
}
```

### `tests/builtins/is_type_builtin.orus`
```orus
fn main() {
    print(is_type(1, "i32"))
    print(is_type("hi", "string"))
    print(is_type(1, "string"))
}
```

### `tests/builtins/max_complex.orus`
```orus
fn main() {
    let values: [f64] = [3.5, 2.0, 4.1]
    print(max(values))

    let arr: [i32; 1] = [0]
    push(arr, 10)
    push(arr, -3)
    push(arr, 7)
    print(max(arr))
}
```

### `tests/builtins/max_simple.orus`
```orus
fn main() {
    let nums = [3, 1, 5, 2]
    print(max(nums))
}
```

### `tests/builtins/min_complex.orus`
```orus
fn main() {
    let values: [f64] = [3.5, 2.0, 4.1]
    print(min(values))

    let arr: [i32; 1] = [0]
    push(arr, 10)
    push(arr, -3)
    push(arr, 7)
    print(min(arr))
}
```

### `tests/builtins/min_simple.orus`
```orus
fn main() {
    let nums = [3, 1, 5, 2]
    print(min(nums))
}
```

### `tests/builtins/numeric_conversion.orus`
```orus
fn main() {
    let age: i32 = int("42")
    let h: f64 = float("3.14")
    print(age)
    print(h)
}
```

### `tests/builtins/sorted_basic.orus`
```orus
fn main() {
    let nums = [4, 2, 1, 3]
    print(sorted(nums, false))
    print(sorted(nums, true))
}
```

### `tests/builtins/sorted_complex.orus`
```orus
fn main() {
    let nums = [5, 1, 4, 3, 2, 2]
    print(sorted(nums, nil, false))
    //print(sorted(nums, false))
    print(sorted(nums, true))

    let words: [string; 1] = ["b"]
    push(words, "d")
    push(words, "a")
    push(words, "c")
    print(sorted(words, false))
    print(sorted(words, true))
}
```

### `tests/builtins/string_interpolation.orus`
```orus
fn main() {
    // Test string interpolation in print function

    // Test with integer value
    print("The value of 10 * 5 is: {}", 10 * 5)

    // Test with floating point value
    print("Pi is approximately {}", 3.14159)

    // Test with variable
    let name = "Orus"
    print("Hello, {}!", name)

    // Test with multiple placeholders
    let a = 10
    let b = 20
    print("The sum of {} and {} is {}", a, b, a + b)

    // Test with boolean value
    print("Is 10 greater than 5? {}", 10 > 5)

    // Test with expression
    print("The result of 5 * (3 + 2) is {}", 5 * (3 + 2))

    // Test with mixed types
    let age = 25
    print("{} is {} years old", name, age)

    // Test with no placeholders
    print("This is a plain string with no interpolation")

    // Test with missing argument
    print("Value: {}")
}


```

### `tests/builtins/string_interpolation_i64.orus`
```orus
fn main() {
    let big = 1234567890123456789
    print("Big number is {}", big)
    print("Sum {} + {} = {}", big, 1, big + 1)
}
```

### `tests/builtins/sum_complex.orus`
```orus
fn main() {
    let values: [f64] = [1.5, 2.5, 3.0]
    print(sum(values))

    let arr: [i32; 1] = [0]
    push(arr, 10)
    push(arr, 20)
    push(arr, -5)
    print(sum(arr))
}
```

### `tests/builtins/sum_simple.orus`
```orus
fn main() {
    let nums = [1, 2, 3, 4]
    print(sum(nums))
}
```

### `tests/builtins/type_builtin.orus`
```orus
fn main() {
    let a: u32 = 4
    print(type_of(a))
    print(type_of("hi"))
    let arr: [i32; 1] = [0]
    print(type_of(arr))

}
```

### `tests/builtins/errors/float_invalid.orus`
```orus
fn main() {
    float("nan!")
}
```

### `tests/builtins/errors/input_extra_arg.orus`
```orus
fn main() {
    input("first", "second")
}
```

### `tests/builtins/errors/input_no_args.orus`
```orus
fn main() {
    input()
}
```

### `tests/builtins/errors/input_wrong_type.orus`
```orus
fn main() {
    input(123)
}
```

### `tests/builtins/errors/int_invalid.orus`
```orus
fn main() {
    int("xyz")
}
```

### `tests/builtins/errors/is_type_no_args.orus`
```orus
fn main() { 
    print(is_type()) 
}
```

### `tests/builtins/errors/is_type_second_arg.orus`
```orus
fn main() {
    print(is_type(1, 2))
}
```

### `tests/builtins/errors/len_invalid_type.orus`
```orus
fn main() {
    let num: i32 = 5
    print(len(num))
}
```

### `tests/builtins/errors/len_no_args.orus`
```orus
fn main() { 
    print(len()) 
}
```

### `tests/builtins/errors/print_extra_args.orus`
```orus
fn main() {
    print("Hello {}", "world", 1)
}
```

### `tests/builtins/errors/sorted_invalid_key.orus`
```orus
fn main() {
    let items = [1, 2]
    print(sorted(items, 1, false))
}
```

### `tests/builtins/errors/type_no_args.orus`
```orus
fn main() {

    print(type_of())

}
```

### `tests/comparison/float_comparison.orus`
Test floating point comparison operations
```orus
// Test floating point comparison operations

fn main() {
    let a: f64 = 10.5
    let b: f64 = 20.7
    let c: f64 = 10.5

    print("f64 Comparisons:")

    print("10.5 == 20.7 is: {} ", a == b)

    print("10.5 == 10.5 is: {} ", a == c)

    print("10.5 != 20.7 is {} ", a != b)

    print("10.5 < 20.7 is: {}", a < b)

    print("10.5 <= 20.7 is: {} ", a <= b)

    print("10.5 <= 10.5 is: {} ", a <= c)

    print("10.5 > 20.7 is: {} ", a > b)

    print("10.5 >= 20.7 is: {} ", a >= b)

    print("10.5 >= 10.5 is: {} ", a >= c)
}


```

### `tests/comparison/i64_comparison.orus`
Test i64 comparison operations
```orus
// Test i64 comparison operations
fn main() {
    let a = 3000000000
    let b = 3000000020
    let c = 3000000000

    print("i64 Comparisons:")

    print("a == b is: {}", a == b)
    print("a == c is: {} ", a == c)
    print("a != b is: {} ", a != b)
    print("a < b is: {} ", a < b)
    print("a <= b is: {} ", a <= b)
    print("a <= c is: {} ", a <= c)
    print("a > b is: {} ", a > b)
    print("a >= b is: {}", a >= b)
    print("a >= c is: {} ", a >= c)
}
```

### `tests/comparison/integer_comparison.orus`
Test integer comparison operations
```orus
// Test integer comparison operations
fn main() {
    let a: i32 = 10
    let b: i32 = 20
    let c: i32 = 10

    print("i32 Comparisons:")

    print("10 == 20 is: {}", a == b)

    print("10 == 10 is: {} ", a == c)

    print("10 != 20 is: {} ", a != b)

    print("10 < 20 is: {} ", a < b)

    print("10 <= 20 is: {} ", a <= b)

    print("10 <= 10 is: {} ", a <= c)

    print("10 > 20 is: {} ", a > b)

    print("10 >= 20 is: {}", a >= b)

    print("10 >= 10 is: {} ", a >= c)
}



```

### `tests/comparison/not_operator.orus`
```orus
fn main() {
    let flag = true
    print("not true is {}", not flag)
    if not false {
        print("not keyword works")
    }
}

```

### `tests/constants/basic_constant.orus`
```orus
const LIMIT = 5

fn main() {
    for i in 0..LIMIT {
        print(i)
    }
}
```

### `tests/constants/math.orus`
```orus
pub const PI: f64 = 3.14159

fn main() {
}
```

### `tests/constants/use_module.orus`
```orus
use tests::constants::math

fn main() {
    print("Value: {}", math.PI)
}
```

### `tests/control_flow/break_test.orus`
Test break statement in loops
```orus
// Test break statement in loops
fn main() {
    let mut sum = 0

    print("Break Test in While Loop:")

    let mut i = 0
    while i < 10 {
        i = i + 1
        if i == 6 {
            print("Breaking at i =")
            print(i)
            break
        }
        sum = sum + i
        print("Added")
        print(i)
        print("Sum =")
        print(sum)
    }

    print("Final sum after break:")
    print(sum)

    // Test break in for loop
    print("Break Test in For Loop:")
    sum = 0

    for j in 1..10 {
        if j == 6 {
            print("Breaking at j =")
            print(j)
            break
        }
        sum = sum + j
        print("Added")
        print(j)
        print("Sum =")
        print(sum)
    }

    print("Final sum after break:")
    print(sum)
}
```

### `tests/control_flow/continue_test.orus`
Test continue statement in loops
```orus
// Test continue statement in loops

fn main() {
    let mut sum = 0

    print("Continue Test in While Loop:")

    let mut i = 0
    while i < 10 {
        i = i + 1
        if i % 2 == 0 {
            print("Skipping even number:")
            print(i)
            continue
        }
        sum = sum + i
        print("Added odd number:")
        print(i)
        print("Sum =")
        print(sum)
    }

    print("Final sum of odd numbers:")
    print(sum)

    // Test continue in for loop
    print("Continue Test in For Loop:")
    sum = 0

    for j in 1..10 {
        if j % 2 == 0 {
            print("Skipping even number:")
            print(j)
            continue
        }
        sum = sum + j
        print("Added odd number:")
        print(j)
        print("Sum =")
        print(sum)
    }

    print("Final sum of odd numbers:")
    print(sum)
}
```

### `tests/control_flow/for_loop_test.orus`
Test for loop control flow
```orus
// Test for loop control flow
fn main() {
    let mut sum = 0

    print("For Loop Test:")

    for i in 1..5 {
        print("Iteration:")
        print(i)
        sum = sum + i
        print("Current sum:")
        print(sum)
    }

    print("Final sum:")
    print(sum)
}
```

### `tests/control_flow/if_else_test.orus`
Test if-else control flow
```orus
// Test if-else control flow
fn main() {
    let x = 10
    let y = 20
    let mut result = 0

    print("If-Else Test:")

    if x > y {
        result = 1
        print("x is greater than y")
    } else {
        result = 2
        print("x is not greater than y")
    }

    // Test with elif
    if x > y {
        print("x is greater than y")
    } elif x < y {
        print("x is less than y")
    } else {
        print("x is equal to y")
    }

    // Test with expression
    //let z = if x > y { 100 } else { 200 }
    //print("z =")
    //print(z)
}

```

### `tests/control_flow/inline_if_expression.orus`
Test inline if (ternary) expressions
```orus
// Test inline if (ternary) expressions
fn main() {
    let a = 5
    let b = 10
    let max = a > b ? a : b
    print(max)
    print(a == 5 ? "five" : "other")
}
```

### `tests/control_flow/nested_loops.orus`
Test nested loops with break using logical operators
```orus
// Test nested loops with break using logical operators
fn main() {
    let mut sum = 0

    print("Nested Loops Test with Logical Operators:")

    // Outer loop
    for i in 0..3 {
        print("Outer i =")
        print(i)
        
        // Inner loop
        for j in 0..3 {
            print("  Inner j =")
            print(j)
            
            // Skip combination using logical AND operator
            if i == 1 and j == 1 {
                print("  Skipping i=1, j=1")
            } else {
                // Break from inner loop using logical OR operator
                if i == 2 or j == 2 {
                    print("  Breaking on i=2 or j=2")
                    break
                }
                
                sum = sum + (i * 10 + j)
                print("  Sum +=")
                print(i * 10 + j)
                print("  Sum =")
                print(sum)
            }
        }
    }

    print("Final sum:")
    print(sum)
}

```

### `tests/control_flow/range_loop_nested.orus`
```orus
fn main() {
    let mut total = 0
    for i in range(1, 4) {
        for j in range(0, i) {
            total = total + j
        }
    }
    print(total)
}
```

### `tests/control_flow/range_loop_simple.orus`
```orus
fn main() {
    for i in range(0, 5) {
        print(i)
    }
}
```

### `tests/control_flow/try_catch.orus`
Test try/catch error handling
```orus
// Test try/catch error handling
fn main() {
    try {
        let a: i32 = 10
        let b: i32 = 0
        let c = a / b
        print(c)
    } catch err {
        print("Caught error: {}", err)
    }
}
```

### `tests/control_flow/try_catch_message.orus`
```orus
fn main() {
    try {
        let a: i32 = 10
        let b: i32 = 0
        let c = a / b
        print(c)
    } catch err {
        print("Error caught: {}", err)
    }
}
```

### `tests/control_flow/while_loop_test.orus`
Test while loop control flow
```orus
// Test while loop control flow

fn main() {
    let mut count = 0
    let mut sum = 0

    print("While Loop Test:")

    while count < 5 {
        count = count + 1
        sum = sum + count
        print("Iteration:")
        print(count)
        print("Current sum:")
        print(sum)
    }

    print("Final sum:")
    print(sum)
}

```

### `tests/datastructures/bool_vector.orus`
Test: Vector of booleans in Orus
```orus
// Test: Vector of booleans in Orus

fn main() {
    let vb: [bool; 5] = [true]
    vb.push(true)
    vb.push(false)
    vb.push(true)
    print("Bool vector: " + vb)
    print("Length: " + len(vb))
    print("First: " + vb[0] + ", Last: " + vb[len(vb)-1])
    let popped = vb.pop()
    print("Popped: " + popped)
    print("After pop: " + vb)
    print("Iterate:")
    for i in 0..len(vb) {
        print("vb[" + i + "]: " + vb[i])
    }
}
```

### `tests/datastructures/float_vector.orus`
Test: Vector of floats in Orus
```orus
// Test: Vector of floats in Orus

fn main() {
    let vf = []
    vf.push(1.5)
    vf.push(2.5)
    vf.push(3.5)
    print("Float vector: " + vf)
    print("Length: " + len(vf))
    print("First: " + vf[0] + ", Last: " + vf[len(vf)-1])
    let popped = vf.pop()
    print("Popped: " + popped)
    print("After pop: " + vf)
    print("Iterate:")
    for i in 0..len(vf) {
        print("vf[" + i + "]: " + vf[i])
    }
}
```

### `tests/datastructures/float_vector_struct.orus`
```orus
struct FloatVector {
    data: [f64; 8],
    length: i32
}

impl FloatVector {
    fn new() -> FloatVector {
        return FloatVector{
            data: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            length: 0
        }
    }

    fn from(value: f64) -> FloatVector {
        let vec = FloatVector.new()
        vec.append(value)
        return vec
    }

    fn append(self, value: f64) {
        if self.length >= len(self.data) {
            self.data.push(value)
        } else {
            self.data[self.length] = value
        }
        self.length = self.length + 1
    }

    fn remove_last(self) -> f64 {
        if self.length == 0 {
            return 0.0
        }
        self.length = self.length - 1
        return self.data[self.length]
    }

    fn get(self, index: i32) -> f64 {
        if index < 0 or index >= self.length {
            return 0.0
        }
        return self.data[index]
    }

    fn set(self, index: i32, value: f64) -> bool {
        if index < 0 or index >= self.length {
            return false
        }
        self.data[index] = value
        return true
    }

    fn size(self) -> i32 {
        return self.length
    }

    fn is_empty(self) -> bool {
        return self.length == 0
    }

    fn clear(self) {
        self.length = 0
    }

    fn index_of(self, value: f64) -> i32 {
        for i in 0..self.length {
            if self.data[i] == value {
                return i
            }
        }
        return -1
    }

    fn contains(self, value: f64) -> bool {
        return self.index_of(value) >= 0
    }

    fn remove_at(self, index: i32) -> bool {
        if index < 0 or index >= self.length {
            return false
        }
        for i in index..self.length-1 {
            self.data[i] = self.data[i + 1]
        }
        self.length = self.length - 1
        return true
    }

    fn insert(self, index: i32, value: f64) -> bool {
        if index < 0 or index > self.length {
            return false
        }
        if self.length >= len(self.data) {
            self.data.push(0.0)
        }
        let mut i = self.length
        while i > index {
            self.data[i] = self.data[i - 1]
            i = i - 1
        }
        self.data[index] = value
        self.length = self.length + 1
        return true
    }
}

fn test_float_vector() {
    print("\n--- FloatVector Test ---")

    let vec = FloatVector.new()
    vec.append(3.14)
    vec.append(2.71)
    vec.append(1.618)

    print("Elements: {}, {}, {}", vec.get(0), vec.get(1), vec.get(2))

    vec.set(1, 9.81)
    print("After update: {}, {}, {}", vec.get(0), vec.get(1), vec.get(2))

    let popped = vec.remove_last()
    print("Popped: {}", popped)
    print("New length: {}", vec.size())

    vec.insert(1, 6.28)
    print("After insert: {}, {}, {}", vec.get(0), vec.get(1), vec.get(2))

    vec.remove_at(0)
    print("After removing index 0: {}, {}", vec.get(0), vec.get(1))

    print("Contains 6.28: {}", vec.contains(6.28))
    print("Index of 9.81: {}", vec.index_of(9.81))
}

fn main() {
    test_float_vector()
}
```

### `tests/datastructures/stack_queue.orus`
Implementation of Stack and Queue data structures using arrays and structs
```orus
// Implementation of Stack and Queue data structures using arrays and structs

// A simple Stack implementation
struct Stack {
    data: [i32; 10],
    top: i32
}

impl Stack {
    fn new() -> Stack {
        return Stack{
            data: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            top: -1
        }
    }
    
    fn push(self, value: i32) -> bool {
        if self.top >= 9 {
            // Stack overflow
            return false
        }
        
        self.top = self.top + 1
        self.data[self.top] = value
        return true
    }
    
    fn pop(self) -> i32 {
        if self.top < 0 {
            // Stack underflow
            return -1
        }
        
        let value: i32 = self.data[self.top]
        self.top = self.top - 1
        return value
    }
    
    fn peek(self) -> i32 {
        if self.top < 0 {
            return -1
        }
        return self.data[self.top]
    }
    
    fn is_empty(self) -> bool {
        return self.top < 0
    }
    
    fn size(self) -> i32 {
        return self.top + 1
    }
}

// A simple Queue implementation
struct Queue {
    data: [i32; 10],
    front: i32,
    rear: i32,
    count: i32
}

impl Queue {
    fn create() -> Queue {
        return Queue{
            data: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            front: 0,
            rear: -1,
            count: 0
        }
    }
    
    fn enqueue(self, value: i32) -> bool {
        if self.count >= 10 {
            // Queue is full
            return false
        }
        
        self.rear = (self.rear + 1) % 10
        self.data[self.rear] = value
        self.count = self.count + 1
        return true
    }
    
    fn dequeue(self) -> i32 {
        if self.count <= 0 {
            // Queue is empty
            return -1
        }
        
        let value: i32 = self.data[self.front]
        self.front = (self.front + 1) % 10
        self.count = self.count - 1
        return value
    }
    
    fn peek(self) -> i32 {
        if self.count <= 0 {
            return -1
        }
        return self.data[self.front]
    }
    
    fn is_empty(self) -> bool {
        return self.count == 0
    }
    
    fn size(self) -> i32 {
        return self.count
    }
}

// Test: Stack and Queue operations in Orus

fn main() {
    // Stack (LIFO)
    let stack = []
    stack.push(1)
    stack.push(2)
    stack.push(3)
    print("Stack after pushes: " + stack)
    let s1 = stack.pop()
    print("Popped from stack: " + s1)
    print("Stack now: " + stack)

    // Queue (FIFO)
    let mut queue = []
    queue.push(10)
    queue.push(20)
    queue.push(30)
    print("Queue after pushes: " + queue)
    let q1 = queue[0]
    queue = queue[1..len(queue)]
    print("Dequeued from queue: " + q1)
    print("Queue now: " + queue)

    // Iterate stack
    print("Iterate stack:")
    for i in 0..len(stack) {
        print("stack[" + i + "]: " + stack[i])
    }
    // Iterate queue
    print("Iterate queue:")
    for i in 0..len(queue) {
        print("queue[" + i + "]: " + queue[i])
    }
}
```

### `tests/datastructures/uint_vector.orus`
Test: Vector of unsigned integers in Orus
```orus
// Test: Vector of unsigned integers in Orus

fn main() {
    let vu = []
    vu.push(1u)
    vu.push(2u)
    vu.push(3u)
    print("UInt vector: " + vu)
    print("Length: " + len(vu))
    print("First: " + vu[0] + ", Last: " + vu[len(vu)-1])
    let popped = vu.pop()
    print("Popped: " + popped)
    print("After pop: " + vu)
    print("Iterate:")
    for i in 0..len(vu) {
        print("vu[" + i + "]: " + vu[i])
    }
}
```

### `tests/datastructures/vector.orus`
Test: Basic vector operations in Orus
```orus
// Test: Basic vector operations in Orus

fn main() {
    // Create an empty vector of integers
    let v = []
    print("Initial length: " + len(v))

    // Push elements
    v.push(10)
    v.push(20)
    v.push(30)
    print("After push: " + v)
    print("Length after push: " + len(v))

    // Indexing
    print("First element: " + v[0])
    print("Second element: " + v[1])
    print("Third element: " + v[2])

    // Pop element
    let last = v.pop()
    print("Popped element: " + last)
    print("After pop: " + v)
    print("Length after pop: " + len(v))

    // Iterate
    print("Iterate vector:")
    for i in 0..len(v) {
        print("v[" + i + "]: " + v[i])
    }
}
```

### `tests/datastructures/vector_integration_test.orus`
Test: Vector integration with structs and mutation in Orus
```orus
// Test: Vector integration with structs and mutation in Orus

struct Point {
    x: i32
    y: i32
}

fn main() {
    let points = []
    points.push(Point{x: 1, y: 2})
    points.push(Point{x: 3, y: 4})
    print("Points vector: " + points)
    for i in 0..len(points) {
        let p = points[i]
        print("Point " + i + ": (" + p.x + ", " + p.y + ")")
    }
    // Mutate a point
    points[0].x = 10
    print("After mutation: " + points[0])
    // Remove all
    while len(points) > 0 {
        let removed = points.pop()
        print("Removed: " + removed)
    }
    print("Points now: " + points)
}
```

### `tests/datastructures/vector_test.orus`
Test: Vector edge cases and integration in Orus
```orus
// Test: Vector edge cases and integration in Orus

fn main() {
    // Pop from empty vector
    let v = []
    print("Pop from empty:")
    let popped = v.pop() // Should handle gracefully (runtime error or nil)
    print("Result: " + popped)

    // Type safety: mixing types
    let vi = []
    vi.push(1)
    // vi.push("string") // Uncomment to test type error

    // Nested vectors
    let vv = []
    vv.push([1,2,3])
    vv.push([4,5])
    print("Nested vector: " + vv)
    for i in 0..len(vv) {
        print("vv[" + i + "]: " + vv[i])
    }
}
```

### `tests/edge_cases/array_index_edges.orus`
Test array indexing at boundary values and dynamic array expansions
```orus
// Test array indexing at boundary values and dynamic array expansions
fn main() {
    let arr: [i32; 3] = [10, 20, 30]
    // Access first and last valid indices
    print(arr[0])
    print(arr[2])

    // Dynamic array operations
    let dyn: [i32; 1] = [5]
    push(dyn, 6)
    print(dyn[1])
    pop(dyn)
    print(len(dyn))
}
```

### `tests/edge_cases/control_flow_edges.orus`
Test loop boundaries and break/continue behaviour
```orus
// Test loop boundaries and break/continue behaviour
fn main() {
    // For loop with zero iterations
    for i in 0..0 {
        print(i)
    }

    // While loop that never executes
    let mut count: i32 = 0
    while count > 0 {
        print(count)
        count = count - 1
    }

    // Break and continue at edges
    for j in 0..5 {
        if j == 0 {
            continue
        }
        if j == 3 {
            break
        }
        print(j)
    }
}
```

### `tests/edge_cases/cyclic_references.orus`
Simulate cyclic references using indices
```orus
// Simulate cyclic references using indices
struct Node {
    value: i32,
    next: i32
}

fn main() {
    let n1: Node = Node{value: 1, next: 1}
    let n2: Node = Node{value: 2, next: 0}
    let nodes: [Node; 2] = [n1, n2]

    let start: i32 = 0
    let second_index: i32 = nodes[start].next
    print(nodes[second_index].value)
}
```

### `tests/edge_cases/deep_recursion_stress.orus`
Recursion depth stress test nearing call stack limits
```orus
// Recursion depth stress test nearing call stack limits
fn recurse(n: i32) {
    if n <= 0 {
        return
    }
    recurse(n - 1)
}

fn main() {
    recurse(70)
    print("done")
}
```

### `tests/edge_cases/dynamic_array_limits.orus`
Grow a dynamic array beyond its initial capacity
```orus
// Grow a dynamic array beyond its initial capacity
fn main() {
    let arr: [i32; 1] = [0]
    for i in 1..20 {
        push(arr, i)
    }
    print(len(arr))
    print(arr[0])
    print(arr[19])
}
```

### `tests/edge_cases/dynamic_array_stress.orus`
Dynamic array stress test with large growth
```orus
// Dynamic array stress test with large growth
fn main() {
    let arr: [i32; 1] = [0]
    for i in 1..1000 {
        push(arr, i)
    }
    print(len(arr))
    print(arr[0])
    print(arr[999])
}
```

### `tests/edge_cases/file_system_simulation.orus`
Simulate simple file system using structs and arrays
```orus
// Simulate simple file system using structs and arrays
struct File { name: string }
struct Directory {
    files: [File; 2],
    count: i32
}

impl Directory {
    fn new() -> Directory {
        let f1: File = File{name: "a.txt"}
        let f2: File = File{name: "b.txt"}
        return Directory{files: [f1, f2], count: 2}
    }

    fn list(self) {
        for i in 0..self.count {
            print(self.files[i].name)
        }
    }
}

fn main() {
    let dir: Directory = Directory.new()
    dir.list()
}
```

### `tests/edge_cases/function_module_edges.orus`
```orus
use tests::edge_cases::module_a
use tests::edge_cases::module_b

fn local_fn(a: i32, b: i32) -> i32 {
    return a + b
}

fn main() {
    print(local_fn(1, 2))
}
```

### `tests/edge_cases/gc_edges.orus`
Stress garbage collector with many temporary arrays
```orus
// Stress garbage collector with many temporary arrays
fn main() {
    for i in 0..50 {
        let arr: [i32; 5] = [1,2,3,4,5]
        push(arr, i)
        let _ = pop(arr)
    }
    print("done")
}
```

### `tests/edge_cases/generic_inference_edges.orus`
Edge Case Generic Inference Test
Ensures generics are deduced correctly when some arguments
provide no type information (e.g. empty arrays).
```orus
// Edge Case Generic Inference Test
// Ensures generics are deduced correctly when some arguments
// provide no type information (e.g. empty arrays).

fn first_or<T>(arr: [T], default: T) -> T {
    if (len(arr) > 0) {
        return arr[0]
    }
    return default
}

fn main() {
    let nums = []
    print(first_or(nums, 42))

    let strs = []
    print(first_or(strs, "fallback"))
}
```

### `tests/edge_cases/implementation_coverage.orus`
Exercise various language features together
```orus
// Exercise various language features together
struct Pair { a: i32, b: i32 }

impl Pair {
    fn sum(self) -> i32 {
        return self.a + self.b
    }
}

fn compute(n: i32) -> i32 {
    let mut total: i32 = 0
    for i in 0..n {
        total = total + i
    }
    return total
}

fn main() {
    let p: Pair = Pair{a: 2, b: 3}
    print(p.sum())
    print(compute(5))
    let arr: [i32; 2] = [7,8]
    print(arr[1])
}
```

### `tests/edge_cases/integer_overflow.orus`
Attempt integer overflow
```orus
// Attempt integer overflow
fn main() {
    let max: i32 = 2147483647
    print(max + 1)
}
```

### `tests/edge_cases/module_a.orus`
```orus
fn main() {
    print("Module A loaded")
}
```

### `tests/edge_cases/module_b.orus`
```orus
fn main() {
    print("Module B loaded")
}
```

### `tests/edge_cases/module_helper.orus`
```orus
fn main() {
    print("Helper loaded")
}
```

### `tests/edge_cases/module_imports.orus`
```orus
use tests::edge_cases::module_a
use tests::edge_cases::module_b

fn main() {
    // Modules are imported for side effects only
}
```

### `tests/edge_cases/nested_expressions.orus`
Deeply nested arithmetic expression
```orus
// Deeply nested arithmetic expression
fn main() {
    let result: i32 = ((((1 + 2) * 3) - 4) / 5) + (6 * (7 - (8 / 2)))
    print(result)
}
```

### `tests/edge_cases/nested_try_catch.orus`
Nested try/catch blocks
```orus
// Nested try/catch blocks
fn main() {
    try {
        try {
            let a = 10 / 0
            print(a)
        } catch inner {
            print("inner error: {}", inner)
        }
    } catch outer {
        print("outer error: {}", outer)
    }
}
```

### `tests/edge_cases/self_referential.orus`
Simple recursive function to test self references
```orus
// Simple recursive function to test self references
fn countdown(n: i32) {
    if n <= 0 {
        return
    }
    countdown(n - 1)
}

fn main() {
    countdown(5)
    print("done")
}
```

### `tests/edge_cases/slice_variants.orus`
```orus
fn main() {
    let nums = [1, 2, 3, 4]
    let a = nums[0..2]
    print(len(a))
    let b = nums[..2]
    print(len(b))
    let c = nums[2..]
    print(len(c))
    let d = nums[..]
    print(len(d))
}
```

### `tests/edge_cases/stack_overflow.orus`
Deep recursion to approach stack limits
```orus
// Deep recursion to approach stack limits
fn recurse(n: i32) {
    if n <= 0 {
        return
    }
    recurse(n - 1)
}

fn main() {
    recurse(20)
    print("done")
}
```

### `tests/edge_cases/string_concat_stress.orus`
Build up a large string through repeated concatenation
```orus
// Build up a large string through repeated concatenation
fn main() {
    let mut text: string = ""
    for i in 0..500 {
        text = text + "a"
    }
    print(len(text))
}
```

### `tests/edge_cases/string_edge_cases.orus`
Test empty and quoted strings
```orus
// Test empty and quoted strings
fn main() {
    let empty: string = ""
    print(empty)
    let quotes: string = "double and single quotes"
    print(quotes)
    let concat = "a" + "b" + "c" + "d" + "e"
    print(concat)
}
```

### `tests/edge_cases/struct_method_edges.orus`
Methods calling other methods
```orus
// Methods calling other methods
struct Counter { value: i32 }

impl Counter {
    fn new(v: i32) -> Counter {
        return Counter{value: v}
    }

    fn inc(self) {
        self.value = self.value + 1
    }

    fn get(self) -> i32 {
        return self.value
    }

    fn inc_and_get(self) -> i32 {
        self.inc()
        return self.get()
    }
}

fn main() {
    let c: Counter = Counter.new(0)
    print(c.inc_and_get())
}
```

### `tests/edge_cases/type_conversion.orus`
Mixed type arithmetic and string conversions
```orus
// Mixed type arithmetic and string conversions
fn main() {
    let i: i32 = 5
    let f: f64 = 2.5
    print((i as f64) + f)
    let text: string = "number:" + i
    print(text)
}
```

### `tests/errors/array_index_out_of_bounds.orus`
```orus
fn main() {
    let arr: [i32; 3] = [1, 2, 3]
    print(arr[3])
}
```

### `tests/errors/array_index_type.orus`
```orus
    fn main() {
    let arr: [i32; 2] = [1, 2, 3, 5]
    print(arr[true])
}
```

### `tests/errors/assign_f64_to_i32.orus`
```orus
fn main() {
    let a: f64 = 3.14
    let b: i32 = a
}
```

### `tests/errors/assign_i32_to_u32.orus`
```orus
fn main() {
    let x: i32 = -1
    let y: u32 = x
}
```

### `tests/errors/cast_from_nil.orus`
```orus
fn main() {
    let a: i32 = nil as i32
}

```

### `tests/errors/cast_from_string.orus`
```orus
fn main() {
    let a: i32 = "5" as i32
}

```

### `tests/errors/conditional_return_missing.orus`
```orus
fn baz(x: bool) -> i32 {
    if x {
        return 1
    }
}

fn main() {}
```

### `tests/errors/division_by_zero.orus`
Test division by zero error handling
```orus
// Test division by zero error handling

fn main() {
    let a: i32 = 10
    let b: i32 = 0

    print("Division by Zero Test:")
    print("Attempting to divide 10 by 0...")

    // This should produce a runtime error
    let result = a / b
}



```

### `tests/errors/generic_constraint_missing.orus`
```orus
fn add<T>(a: T, b: T) -> T {
    return a + b
}

fn main() {
    let r = add(1, 2)
    print(r)
}
```

### `tests/errors/immutable_assignment.orus`
```orus
fn main() {
    let x: i32 = 5
    x = 6
}
```

### `tests/errors/implicit_conversion.orus`
```orus
fn main() {
    let a: i32 = 5
    let b: f64 = 3.0
    let c = a + b
    print("{}", c)
}
```

### `tests/errors/import_cycle.orus`
```orus
use tests::modules::cycles::cycle_a

fn main() {
}
```

### `tests/errors/import_deprecated.orus`
```orus
import "tests/modules/hello_module.orus"

fn main() {
    
}
```

### `tests/errors/import_missing.orus`
```orus
use tests::modules::not_there

fn main() {
}
```

### `tests/errors/import_private.orus`
```orus
use tests::modules::pub_module

fn main() {
    private_fn()
}

```

### `tests/errors/import_twice.orus`
```orus
use tests::modules::hello_module
use tests::modules::hello_module

fn main() {
    print("done")
}

```

### `tests/errors/missing_export.orus`
```orus
use tests::modules::hello_module::{unknown}

fn main() {
}
```

### `tests/errors/missing_return.orus`
```orus
fn foo() -> i32 {
    let a = 1
}

fn main() {}
```

### `tests/errors/mixed_arithmetic_u32_i32.orus`
```orus
fn main() {
    let x: u32 = 10
    let y: i32 = 20
    let z = x + y
}
```

### `tests/errors/return_outside_function.orus`
```orus
return 42

fn main() {}

```

### `tests/errors/return_type_mismatch.orus`
```orus
fn bar() -> f64 {
    return 3
}

fn main() {}
```

### `tests/errors/scope_shadowing.orus`
Variable shadowing across scopes
```orus
// Variable shadowing across scopes
fn main() {
    let x: i32 = 5
    if true {
        let x_inner: i32 = 10
        print(x_inner)
    }
    print(x)
    print(x_inner)
}
```

### `tests/errors/string_to_bool.orus`
```orus
fn main() {
    let b: bool = "true" as bool
}

```

### `tests/errors/type_mismatch.orus`
Test type mismatch error handling
```orus
// Test type mismatch error handling

fn main() {
    let a: i32 = 10
    let b: bool = true

    print("Type Mismatch Test:")
    print("Attempting to add an integer and a boolean...")

    // This should produce a compilation error
    let result = a + b

    print("Using compatible types instead:")
    let c: i32 = 20
    let result = a + c
    print(result)
}


```

### `tests/errors/type_persistence.orus`
```orus
fn main() {
    let x = nil
    x = 1
    x = "oops"
}
```

### `tests/errors/undefined_field.orus`
```orus
struct Foo { a: i32 }
fn main() {
    let f: Foo = Foo{a: 1}
    print(f.b)
}
```

### `tests/errors/undefined_method.orus`
```orus
struct Foo {}
fn main() {
    let f: Foo = Foo{}
    f.bar()
}
```

### `tests/errors/undefined_variable.orus`
Test undefined variable error handling
```orus
// Test undefined variable error handling

fn main() {
    let a: i32 = 10

    print("Undefined Variable Test:")
    print("a is defined and its value is:")
    print(a)

    // The line below would cause a compilation error
    print(b)
}

```

### `tests/errors/use_twice.orus`
```orus
use tests::modules::hello_module
use tests::modules::hello_module::{greet}

fn main() {
    greet()
}
```

### `tests/errors/void_return_nonvoid.orus`
```orus
fn qux() -> string {
    return
}

fn main() {}
```

### `tests/functions/advanced_functions.orus`
Test advanced function features in Orus
```orus
// Test advanced function features in Orus

// Recursive function
fn factorial(n: i32) -> i32 {
    if n <= 1 {
        return 1
    } else {
        return n * factorial(n - 1)
    }
}

// Function with multiple return paths
fn max(a: i32, b: i32) -> i32 {
    if a > b {
        return a
    }
    return b
}

// Function that returns a boolean
fn isEven(num: i32) -> bool {
    return num % 2 == 0
}

fn main() {
    print("Advanced Function Tests:")

    // Test recursive function
    print("Factorial of 5:")
    let n5 = 5
    let fact5 = factorial(n5)
    print(fact5)

    // Test function with multiple return paths
    print("Maximum of 42 and 17:")
    let a1 = 42
    let b1 = 17
    let maxResult1 = max(a1, b1)
    print(maxResult1)

    print("Maximum of 13 and 37:")
    let a2 = 13
    let b2 = 37
    let maxResult2 = max(a2, b2)
    print(maxResult2)

    // Test boolean returning function
    print("Is 10 even?")
    let num1 = 10
    let even10 = isEven(num1)
    print(even10)

    print("Is 7 even?")
    let num2 = 7
    let even7 = isEven(num2)
    print(even7)

    // Test function composition
    print("Function composition - max(factorial(3), factorial(2)):")
    let n3 = 3
    let n2 = 2
    let fact3 = factorial(n3)
    let fact2 = factorial(n2)
    let composed = max(fact3, fact2)
    print(composed)
}
```

### `tests/functions/basic_functions.orus`
Test basic function support in Orus
```orus
// Test basic function support in Orus

// Simple function without parameters
fn sayHello() {
    print("Hello from a function!")
}

// Function with parameters
fn add(a: i32, b: i32) -> i32 {
    return a + b
}

// Function with multiple statements and return value
fn calculateSum(n: i32) -> i32 {
    let mut sum = 0
    for i in 1..n+1 {
        sum = sum + i
    }
    return sum
  }

fn main() {
  print("Basic Function Tests:")

  // Call the functions
  print(add(5, 7))
  print(calculateSum(5))
}

```

### `tests/functions/forward_call.orus`
Test calling a function before it is defined
```orus
// Test calling a function before it is defined
fn main() {
    print(foo(3))
}

fn foo(x: i32) -> i32 {
    return x * 2
}
```

### `tests/gc/stress.orus`
```orus
fn main() {
    for i in 0..200 {
        let arr: [i32; 20] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]
        let first: i32 = arr[0]
        if first < 0 { print(first) }
    }
}
```

### `tests/generics/advanced_generics.orus`
Advanced Generic Features Test
Tests more complex generic features and edge cases
```orus
// Advanced Generic Features Test
// Tests more complex generic features and edge cases

// Define a container for generic interface
struct Container<T> {
    value: T
}

// Generic function with return type inference
fn createContainer<T>(value: T) -> Container<T> {
    return Container<T>{value: value}
}

// Recursive generic structure
struct TreeNode<T> {
    value: T
    left: [TreeNode<T>] // Using array as optional
    right: [TreeNode<T>] // Using array as optional
}

fn createLeaf<T>(value: T) -> TreeNode<T> {
    return TreeNode<T>{
        value: value,
        left: [],
        right: []
    }
}

fn createNode<T>(value: T, left: TreeNode<T>, right: TreeNode<T>) -> TreeNode<T> {
    let node = TreeNode<T>{
        value: value,
        left: [],
        right: []
    }
    node.left.push(left)
    node.right.push(right)
    return node
}

// Function that traverses tree in-order
fn inorderTraversal<T>(node: TreeNode<T>, result: [T]) {
    if (len(node.left) > 0) {
        inorderTraversal(node.left[0], result)
    }
    
    result.push(node.value)
    
    if (len(node.right) > 0) {
        inorderTraversal(node.right[0], result)
    }
}

// Test nested generics
struct Wrapper<T> {
    data: Container<T>
}

// Generic with multiple parameters
struct Tuple<A, B, C> {
    first: A
    second: B
    third: C
}

fn main() {
    // Test container creation
    let strContainer = createContainer("Hello")
    let intContainer = createContainer(42)
    
    print("String container: {}", strContainer.value)
    print("Integer container: {}", intContainer.value)
    
    // Test binary tree
    let tree = createNode(2, createLeaf(1), createLeaf(3))
    
    let traversalResult: [i32] = []
    inorderTraversal<i32>(tree, traversalResult)
    print("In-order traversal: {}", traversalResult)
    
    // Test nested generics
    let wrapper = Wrapper<string>{
        data: Container<string>{value: "Nested generics"}
    }
    print("Nested container value: {}", wrapper.data.value)
    
    // Test tuple
    let tuple = Tuple<i32, string, bool>{
        first: 1,
        second: "two",
        third: true
    }
    print("Tuple values: {}, {}, {}", tuple.first, tuple.second, tuple.third)
    
    // Test containers with different types
    let containers = []
    containers.push(createContainer(100))
    containers.push(createContainer(200))
    containers.push(createContainer(300))
    
    print("Container values:")
    for i in 0..len(containers) {
        print("Container {}: {}", i, containers[i].value)
    }
}
```

### `tests/generics/comprehensive_generic_test.orus`
Comprehensive Generic Test
This test showcases generics with struct, impl, and functions
```orus
// Comprehensive Generic Test
// This test showcases generics with struct, impl, and functions

// Generic helper functions must be defined before use in Orus.
fn toString<T>(value: T) -> string {
    if (type_of(value) == "string") {
        return value
    }
    return "<" + type_of(value) + ">"
}

fn error(msg: string) {
    print("Error: " + msg)
}

// A basic generic wrapper type
struct Result<T> {
    value: T
    isError: bool
}

// Implementation for the Result type
impl Result {
    // Static method to create a success result
    fn ok<T>(value: T) -> Result<T> {
        return Result<T>{
            value: value,
            isError: false
        }
    }
    
    // Static method to create an error result
    fn err<T>(value: T) -> Result<T> {
        return Result<T>{
            value: value,
            isError: true
        }
    }
}

// Additional instance methods for `Result<T>`
impl Result<T> {
    fn isOk(self) -> bool {
        return not self.isError
    }

    fn isErr(self) -> bool {
        return self.isError
    }

    fn unwrapOr(self, defaultValue: T) -> T {
        if (self.isError) {
            return defaultValue
        } else {
            return self.value
        }
    }
}

fn unwrapOrElse<T>(result: Result<T>, defaultValue: T) -> T {
    if (result.isError) {
        return defaultValue
    } else {
        return result.value
    }
}

// A generic collection type with multiple type parameters
struct KeyValue<K, V> {
    key: K
    value: V
}

// Implementation for the KeyValue type
impl KeyValue<K, V> {
    // Static method to create a new key-value pair
    fn new<K, V>(key: K, value: V) -> KeyValue<K, V> {
        return KeyValue<K, V>{
            key: key,
            value: value
        }
    }
    
    // Instance method to convert to string representation
    fn toString(self) -> string {
        return "Key: " + toString(self.key) + ", Value: " + toString(self.value)
    }
}

// A generic data structure for a binary tree
struct TreeNode<T> {
    value: T
    left: [TreeNode<T>]  // Using array as optional
    right: [TreeNode<T>] // Using array as optional
}

// Implementation for the TreeNode type
impl TreeNode<T> {
    // Static method to create a leaf node (no children)
    fn leaf<T>(value: T) -> TreeNode<T> {
        return TreeNode<T>{
            value: value,
            left: [],
            right: []
        }
    }
    
    // Static method to create an internal node (with children)
    fn node<T>(value: T, left: TreeNode<T>, right: TreeNode<T>) -> TreeNode<T> {
        let node = TreeNode<T>{
            value: value,
            left: [],
            right: []
        }
        node.left.push(left)
        node.right.push(right)
        return node
    }
    
    // Instance method to check if node is a leaf
    fn isLeaf(self) -> bool {
        return len(self.left) == 0 and len(self.right) == 0
    }
    
    // Instance method to get value
    fn getValue(self) -> T {
        return self.value
    }
}

// Helper function for tree traversal used in the main demonstration
fn inOrderTraversal<T>(node: TreeNode<T>, result: [T]) {
    if (len(node.left) > 0) {
        inOrderTraversal(node.left[0], result)
    }

    result.push(node.value)

    if (len(node.right) > 0) {
        inOrderTraversal(node.right[0], result)
    }
}

// Calculate the sum of integer values in an array. Fully generic arithmetic is
// not currently supported in the language, so this helper is specialised for
// `i32` values.
fn sum(values: [i32]) -> i32 {
    if (len(values) == 0) {
        error("Cannot sum an empty array")
    }

    let mut result = values[0]
    for i in 1..len(values) {
        result = result + values[i]
    }
    return result
}

// Generic map function with callback
fn map<T, U>(items: [T], transform: (T) -> U) -> [U] {
    let result = []
    for i in 0..len(items) {
        result.push(transform(items[i]))
    }
    return result
}

// Generic filter function with predicate
fn filter<T>(items: [T], predicate: (T) -> bool) -> [T] {
    let result = []
    for i in 0..len(items) {
        if (predicate(items[i])) {
            result.push(items[i])
        }
    }
    return result
}

// Callback functions for testing
fn double(x: i32) -> i32 {
    return x * 2
}

fn isEven(x: i32) -> bool {
    return x % 2 == 0
}

// Main function to test all generic features
fn main() {
    print("Starting comprehensive generic test...")
    
    // Test the Result implementation
    let successResult = Result.ok(42)
    let errorResult = Result.err("Something went wrong")
    
    print("Success result: {}, isOk: {}", successResult.value, successResult.isOk())
    print("Error result: {}, isErr: {}", errorResult.value, errorResult.isErr())
    
    // Test unwrapOr method
    let defaultInt = 0
    print("Unwrap success: {}", unwrapOrElse<i32>(successResult, defaultInt))

    let defaultString = "Default message"
    print("Unwrap error with default: {}", unwrapOrElse<string>(errorResult, defaultString))
    
    // Test KeyValue implementation
    let userEntry = KeyValue.new(1, "John")
    let configEntry = KeyValue.new("server", "localhost:8080")
    
    print("User entry: {}", userEntry.toString())
    print("Config entry: {}", configEntry.toString())
    
    // Test TreeNode implementation
    let leaf1 = TreeNode.leaf(1)
    let leaf2 = TreeNode.leaf(3)
    let root = TreeNode.node(2, leaf1, leaf2)
    
    print("Root value: {}", root.getValue())
    print("Is leaf1 a leaf? {}", leaf1.isLeaf())
    print("Is root a leaf? {}", root.isLeaf())
    
    // Test traversing the tree (in-order)
    let result: [i32] = []
    inOrderTraversal<i32>(root, result)
    print("In-order traversal: {}", result)
    
    // Test generic array functions
    let numbers = [1, 2, 3, 4, 5]
    let sumResult = sum(numbers)
    print("Sum of numbers: {}", sumResult)
    
    // Test map function
    let doubled = map(numbers, double)
    print("Doubled numbers: {}", doubled)
    
    // Test filter function
    let evenNumbers = filter(numbers, isEven)
    print("Even numbers: {}", evenNumbers)
    
    // Test nested generics
    let nestedResult = Result.ok(KeyValue.new("api_key", "secret123"))
    if (nestedResult.isOk()) {
        print("Nested result: {}", nestedResult.value.toString())
    }
    
    // Test array of generic types
    let results: [Result<i32>] = []
    results.push(Result.ok(100))
    results.push(Result.ok(200))
    results.push(Result.err(300))
    
    print("Results array:")
    for i in 0..len(results) {
        let mut status = ""
        if results[i].isOk() {
            status = "OK"
        } else {
            status = "Error"
        }
        print("  {}: {} - {}", i, status, results[i].value)
    }
}
```

### `tests/generics/generic_arithmetic.orus`
Generic arithmetic operations test
```orus
// Generic arithmetic operations test
fn add<T: Numeric>(a: T, b: T) -> T {
    return a + b
}

fn subtract<T: Numeric>(a: T, b: T) -> T {
    return a - b
}

fn multiply<T: Numeric>(a: T, b: T) -> T {
    return a * b
}

fn divide<T: Numeric>(a: T, b: T) -> T {
    return a / b
}

fn greater<T: Comparable>(a: T, b: T) -> bool {
    return a > b
}

fn main() {
    print("add i32: {}", add(1, 2))
    print("add f64: {}", add(1.5, 2.5))
    print("sub i32: {}", subtract(5, 3))
    print("mul i32: {}", multiply(2, 4))
    print("div f64: {}", divide(5.0, 2.0))
    print("greater test: {}", greater(5, 3))
}
```

### `tests/generics/generic_collections.orus`
Generic Collections Test
Tests the implementation of generic collections
```orus
// Generic Collections Test
// Tests the implementation of generic collections

// Define a basic generic collection
struct List<T> {
    items: [T]
}

// Implementation for the List
fn add<T>(list: List<T>, item: T) {
    list.items.push(item)
}

fn get<T>(list: List<T>, index: i32) -> T {
    return list.items[index]
}

fn size<T>(list: List<T>) -> i32 {
    return len(list.items)
}

// Define a specialized collection for comparable items
struct SortedList<T> {
    items: [T]
}

// Create generic complex data structure with multiple type parameters
struct Pair<K, V> {
    key: K
    value: V
}

fn main() {
    // Test basic list operations
    let numbers = List<i32>{items: []}
    add(numbers, 10)
    add(numbers, 20)
    add(numbers, 30)
    
    print("List size: " + size(numbers))
    print("First item: " + get(numbers, 0))
    print("Second item: " + get(numbers, 1))
    
    // Test list with strings
    let names = List<string>{items: []}
    add(names, "Alice")
    add(names, "Bob")
    add(names, "Charlie")
    
    print("Names list:")
    for i in 0..size(names) {
        print(get(names, i))
    }
    
    // Test with complex data structure
    let dict = []
    dict.push(Pair<string, i32>{key: "one", value: 1})
    dict.push(Pair<string, i32>{key: "two", value: 2})
    dict.push(Pair<string, i32>{key: "three", value: 3})
    
    print("Dictionary entries:")
    for i in 0..len(dict) {
        print(dict[i].key + " -> " + dict[i].value)
    }
}
```

### `tests/generics/generic_forward_decl.orus`
Test generic function defined after its use
```orus
// Test generic function defined after its use
fn main() {
    print(identity<string>("hi"))
    print(identity<i32>(42))
}

fn identity<T>(val: T) -> T {
    return val
}
```

### `tests/generics/generic_functions.orus`
Generic Functions Test
Tests defining and using functions with generic type parameters
```orus
// Generic Functions Test
// Tests defining and using functions with generic type parameters

// Generic identity function
fn identity<T>(value: T) -> T {
    return value
}

// Simple error helper used for demonstration purposes
fn error(msg: string) {
    print("Error: " + msg)
}

// Function that works with any array type
fn firstElement<T>(arr: [T]) -> T {
    if (len(arr) == 0) {
        error("Array is empty")
    }
    return arr[0]
}

// Pair struct for zip results
struct Pair<A, B> {
    first: A
    second: B
}

// Function with multiple type parameters
fn zip<A, B>(arrA: [A], arrB: [B]) -> [Pair<A, B>] {
    let result = []
    let mut length = len(arrA)
    if (len(arrB) < length) {
        length = len(arrB)
    }
    
    for i in 0..length {
        let pair = Pair<A, B>{first: arrA[i], second: arrB[i]}
        result.push(pair)
    }
    
    return result
}

// Higher-order function with generic parameters
fn map<T, U>(arr: [T], callback: (T) -> U) -> [U] {
    let result = []
    for i in 0..len(arr) {
        result.push(callback(arr[i]))
    }
    return result
}

fn double(x: i32) -> i32 {
    return x * 2
}

fn toUpper(s: string) -> string {
    // This is a placeholder - assuming Orus has a built-in way to convert to uppercase
    // If not, this would need to be implemented differently
    return s + " (uppercase)"
}

fn main() {
    // Test the identity function
    let num = identity(42)
    print("Identity of 42: {}", num)
    
    let str = identity("Hello")
    print("Identity of 'Hello': {}", str)
    
    // Test firstElement function
    let numbers = [10, 20, 30, 40]
    print("First number: {}", firstElement(numbers))
    
    let names = ["Alice", "Bob", "Charlie"]
    print("First name: {}", firstElement(names))
    
    // Test zip function
    let ids = [1, 2, 3, 4]
    let users = ["user1", "user2", "user3"]
    let zipped = zip(ids, users)
    
    print("Zipped results:")
    for i in 0..len(zipped) {
        print("ID: {}, User: {}", zipped[i].first, zipped[i].second)
    }
    
    // Test map function
    let doubled = map(ids, double)
    print("Original numbers: {}", ids)
    print("Doubled numbers: {}", doubled)
    
    let uppercased = map(users, toUpper)
    print("Uppercased users: {}", uppercased)
}
```

### `tests/generics/generic_implementation.orus`
Tests for Generic Type Implementation
This test verifies the compiler and VM correctly handle generics
```orus
// Tests for Generic Type Implementation
// This test verifies the compiler and VM correctly handle generics

// Test the DEFINE_ARRAY_TYPE macro implementation
struct GenericTest<T> {
    array: [T]
}

// Function to test generic type instantiation
fn createGenericInstance<T>(value: T) -> GenericTest<T> {
    let instance = GenericTest<T>{
        array: []
    }
    instance.array.push(value)
    return instance
}

// Test interactions between different generic types
struct Pair<A, B> {
    first: A
    second: B
}

fn createPair<A, B>(a: A, b: B) -> Pair<A, B> {
    return Pair<A, B>{
        first: a,
        second: b
    }
}

// Test function with multiple generic type parameters
fn swapPair<A, B>(pair: Pair<A, B>) -> Pair<B, A> {
    return Pair<B, A>{
        first: pair.second,
        second: pair.first
    }
}

fn main() {
    // Test creating different instances of the same generic type
    let intTest = createGenericInstance(42)
    let stringTest = createGenericInstance("string value")
    let floatTest = createGenericInstance(3.14)
    let boolTest = createGenericInstance(true)
    
    print("Int generic: {}", intTest.array[0])
    print("String generic: {}", stringTest.array[0])
    print("Float generic: {}", floatTest.array[0])
    print("Bool generic: {}", boolTest.array[0])
    
    // Test pairs and swapping
    let numberStringPair = createPair(100, "hundred")
    print("Original pair: {}, {}", numberStringPair.first, numberStringPair.second)
    
    let swapped = swapPair(numberStringPair)
    print("Swapped pair: {}, {}", swapped.first, swapped.second)
    
    // Test nested generic types using simple values
    let nestedPair: Pair<GenericTest<i32>, GenericTest<string> > = createPair<GenericTest<i32>, GenericTest<string> >(createGenericInstance(1), createGenericInstance("a"))

    print("Nested first: {}", nestedPair.first.array[0])
    print("Nested second: {}", nestedPair.second.array[0])
}
```

### `tests/generics/generic_struct.orus`
Tests for generic structs
This test demonstrates defining and using a basic generic struct
```orus
// Tests for generic structs
// This test demonstrates defining and using a basic generic struct

// Define a generic Box structure with a single type parameter T
struct Box<T> {
    item: T
}

// Create boxes with different types
fn main() {
    // Integer box
    let intBox = Box<i32>{item: 42}
    print("Int box contains: " + intBox.item)
    
    // Float box
    let floatBox = Box<f64>{item: 3.14}
    print("Float box contains: " + floatBox.item)
    
    // String box
    let stringBox = Box<string>{item: "Hello, generics!"}
    print("String box contains: " + stringBox.item)
    
    // Test reassigning values
    intBox.item = 100
    print("Updated int box: " + intBox.item)
    
    // Test nested generic structures
    let nestedBox = Box<Box<i32> >{item: intBox}
    print("Nested box contains: " + nestedBox.item.item)
}
```

### `tests/generics/identity_generic.orus`
```orus
fn id<T>(x: T) -> T {
    return x
}

fn main() {
    print(id(1))
    print(id("hi"))
}
```

### `tests/generics/type_constraints.orus`
Generic Type Constraints and Edge Cases Test
Tests boundary conditions and edge cases for generic types
```orus
// Generic Type Constraints and Edge Cases Test
// Tests boundary conditions and edge cases for generic types

// Generic minimum function with numeric constraint
fn min<T: Comparable>(a: T, b: T) -> T {
    if (a < b) {
        return a
    } else {
        return b
    }
}

// Max function with similar constraints
fn max<T: Comparable>(a: T, b: T) -> T {
    if (a > b) {
        return a
    } else {
        return b
    }
}

// Generic equality test
fn equals<T: Comparable>(a: T, b: T) -> bool {
    return a == b
}

// A struct with generic default value
struct Optional<T> {
    hasValue: bool
    value: T
}

fn some<T>(value: T) -> Optional<T> {
    return Optional<T>{hasValue: true, value: value}
}

fn none<T>(defaultValue: T) -> Optional<T> {
    // Provide an explicit default value when none is present
    return Optional<T>{hasValue: false, value: defaultValue}
}

fn getValue<T>(opt: Optional<T>, defaultValue: T) -> T {
    if (opt.hasValue) {
        return opt.value
    } else {
        return defaultValue
    }
}

// Test using generics with arrays and nil values
struct NullableArray<T> {
    items: [T]
}

fn main() {
    // Test numeric constraints
    let minValue = min(5, 3)
    print("Min of 5 and 3: {}", minValue)
    
    let maxValue = max(5, 3)
    print("Max of 5 and 3: {}", maxValue)
    
    // Test with floats
    let minFloat = min(3.14, 2.71)
    print("Min of 3.14 and 2.71: {}", minFloat)
    
    // Test generic equality
    print("5 == 5: {}", equals(5, 5))
    print("5 == 3: {}", equals(5, 3))
    print("\"hello\" == \"hello\": {}", equals("hello", "hello"))
    print("\"hello\" == \"world\": {}", equals("hello", "world"))
    
    // Test optional values
    let optionalInt = some(42)
    print("Has value: {}", optionalInt.hasValue)
    print("Value: {}", getValue<i32>(optionalInt, 0))
    
    let emptyOpt = none<i32>(0)
    print("Has value: {}", emptyOpt.hasValue)
    print("Default value: {}", getValue<i32>(emptyOpt, -1))
    
    // Edge case: Empty array with generic type
    let emptyArray = NullableArray<string>{items: []}
    emptyArray.items.push("First item")
    print("Array after adding item: {}", emptyArray.items[0])
    
    // Test with booleans
    let boolOpt = some(true)
    print("Boolean optional: {}", getValue<bool>(boolOpt, false))
}
```

### `tests/main/basic_main.orus`
Verify that the interpreter calls main automatically
```orus
// Verify that the interpreter calls main automatically
fn main() {
    print("Hello from main")
}
```

### `tests/main/inventory_main.orus`
```orus
struct Inventory {
    shirts: [string; 3],
}

impl Inventory {
    fn most_stocked(self) -> string {
        let mut num_red: i32 = 0
        let mut num_blue: i32 = 0
        for i in 0..3 {
            let color: string = self.shirts[i]
            if color == "red" {
                num_red = num_red + 1
            } else {
                num_blue = num_blue + 1
            }
        }
        if num_red > num_blue {
            return "red"
        }
        return "blue"
    }

    fn giveaway(self, user_pref: string) -> string {
        if user_pref != "" {
            return user_pref
        }
        return most_stocked(self)
    }
}

fn main() {
    let store: Inventory = Inventory{
        shirts: ["blue", "red", "blue"],
    }

    let user_pref1: string = "red"
    let giveaway1: string = store.giveaway(user_pref1)
    print("The user with preference {} gets {}", user_pref1, giveaway1)

    let user_pref2: string = ""
    let giveaway2: string = store.giveaway(user_pref2)
    print("The user with preference {} gets {}", user_pref2, giveaway2)
}
```

### `tests/main/main_return.orus`
main may return a value which is ignored by the interpreter
```orus
// main may return a value which is ignored by the interpreter
fn main() -> i32 {
    print("Returning 42 from main")
    return 42
}
```

### `tests/main/module_main.orus`
Test calling functions from main and importing modules
```orus
// Test calling functions from main and importing modules
use tests::modules::hello_module

fn main() {
    print("Main ran")
}
```

### `tests/match/complex_match.orus`
```orus
fn main() {
    let count = 1
    match count {
        0 => print("zero"),
        1 => print("one"),
        _ => print("many")
    }

    let word = "maybe"
    match word {
        "yes" => print("yes"),
        "no" => print("no"),
        "maybe" => {
            print("maybe yes")
            print("maybe no")
        },
        _ => print("unknown")
    }
}
```

### `tests/match/simple_match.orus`
```orus
fn main() {
    let value = "yes"
    match value {
        "yes" => print("ok"),
        "no" => print("not ok"),
        _ => print("unknown")
    }
}
```

### `tests/modules/hello_module.orus`
```orus
pub fn greet() {
    let greeting = "Hello from module"
    print(greeting)
}

fn main() {

}


```

### `tests/modules/parse_use_alias.orus`
```orus
use tests::modules::hello_module as hm

fn main() {
    print("alias parsed")
}
```

### `tests/modules/parse_use_module.orus`
```orus
use tests::modules::hello_module

fn main() {
    print("module parsed")
}
```

### `tests/modules/parse_use_selective.orus`
```orus
use tests::modules::hello_module

fn main() {
    print("select parsed")
}
```

### `tests/modules/pub_module.orus`
```orus
pub fn public_fn() {
    let greeting = "public"
    print(greeting)
}

fn private_fn() {
    print("private")
}

fn main() {

}


```

### `tests/modules/pub_struct_module.orus`
```orus
pub struct Point {
    x: i32,
    y: i32,
}

pub fn make_point(x: i32, y: i32) -> Point {
    return Point{x: x, y: y}
}

fn main() {
    // empty main so module can compile standalone
}
```

### `tests/modules/use_bind.orus`
```orus
use tests::modules::hello_module

fn main() {
    hello_module.greet()
}
```

### `tests/modules/use_pub_function.orus`
```orus
use tests::modules::pub_module

fn main() {
    pub_module.public_fn()
}

```

### `tests/modules/use_pub_struct.orus`
```orus
use tests::modules::pub_struct_module

fn main() {
    let p = pub_struct_module.make_point(3, 4)
    print(p.x)
    print(p.y)
}
```

### `tests/modules/use_selective.orus`
```orus
use tests::modules::hello_module

fn main() {
    hello_module.greet()
}
```

### `tests/modules/cycles/cycle_a.orus`
```orus
use tests::modules::cycles::cycle_b

pub fn greet_a() {
    print("Hello from A")
}

fn main() {}
```

### `tests/modules/cycles/cycle_b.orus`
```orus
use tests::modules::cycles::cycle_a

pub fn greet_b() {
    print("Hello from B")
}

fn main() {}
```

### `tests/projects/cross_module_generics/src/main.orus`
```orus
use src::util

fn main() {
    print(util.identity<i32>(42))
    print(util.identity<string>("hello"))
}
```

### `tests/projects/cross_module_generics/src/util.orus`
```orus
pub fn identity<T>(val: T) -> T {
    return val
}
```

### `tests/projects/simple_project/src/helper.orus`
```orus
pub fn greet() {
    print("Hello from helper")
}
```

### `tests/projects/simple_project/src/main.orus`
```orus
use src::helper::{greet}

fn main() {
    greet()
}
```

### `tests/stdlib/datetime_module.orus`
```orus
use std::datetime

fn main() {
    let epoch = datetime.from_timestamp(0.0)
    print(epoch)
    print(datetime.timestamp(epoch))
    let current = datetime.now()
    print(current)
    let utc = datetime.utcnow()
    print(utc)
    print(datetime.format(epoch, "%Y-%m-%d %H:%M:%S"))
    let parsed = datetime.parse("2024-02-03 04:05:06", "%Y-%m-%d %H:%M:%S")
    print(parsed)
    print(datetime.to_string(parsed))
    print(datetime.DateTime_to_string(parsed))
    print(datetime.timestamp(parsed))
    let d = datetime.date(parsed)
    print(d.year)
    print(d.month)
    print(d.day)
    let t = datetime.time(parsed)
    print(t.hour)
    print(t.minute)
    print(t.second)
    print(t.microsecond)
}
```

### `tests/stdlib/math_module.orus`
```orus
use std::math

fn main() {
    print(math.abs(-5.0))
    print(math.clamp(15.0, 0.0, 10.0))
    print(math.pow(2.0, 8))
    print(math.sqrt(9.0))
    print(math.round(3.6))
    print(math.floor(3.9))
    print(math.ceil(3.1))
    print(math.sign(-2.0))
    let vals = [1.0, 2.0, 3.0]
    print(math.average(vals))
    let mids = [3.0, 1.0, 2.0]
    print(math.median(mids))
    print(math.mod(-3, 5))
    print(math.PI)
    print(math.E)
    print(math.TAU)
}
```

### `tests/stdlib/random_module.orus`
```orus
use std::random

fn main() {
    print(random.randint(1, 10))
    print(random.randint(1, 10))
    print(random.randint(1, 10))
    let colors = ["red", "green", "blue"]
    print(random.choice(colors))
    let nums = [1, 2, 3, 4]
    random.shuffle(nums)
    print(nums[0])
    print(nums[1])
    print(nums[2])
    print(nums[3])
}
```

### `tests/strings/cast_to_string.orus`
```orus
fn main() {
    let a: string = 42 as string
    let b: string = 3.5 as string
    let c: string = true as string
    let d: string = [1, 2, 3] as string
    print(a + "," + b + "," + c + "," + d)
}
```

### `tests/strings/escape_sequences.orus`
```orus
fn main() {
    print("Hello\nWorld")
    print("Column A\tColumn B")
    print("A backslash: \\")
    print("He said: \"Hello\"")
}

```

### `tests/strings/string_concat.orus`
```orus
fn main() {
    print("Hello " + "World")
    print("Number: " + 42 + ", pi=" + 3.14)
}
```

### `tests/strings/string_len.orus`
```orus
fn main() {
    print(len("hello"))
}
```

### `tests/strings/string_substring.orus`
```orus
fn main() {
    print(substring("hello", 1, 3))
    print(substring("world", 0, 5))
}
```

### `tests/structs/basic_struct.orus`
```orus
struct Empty {}
fn main() {
    print("Struct defined")
}
```

### `tests/structs/calculator_struct.orus`
Test for structs with both static (non-self) and instance (self) methods
```orus
// Test for structs with both static (non-self) and instance (self) methods

struct Calculator {
    current_value: i32,
    memory: i32,
    operations_count: i32
}

impl Calculator {
    // Static (non-self) factory method
    fn new(initial_value: i32) -> Calculator {
        return Calculator{
            current_value: initial_value,
            memory: 0,
            operations_count: 0
        }
    }
    
    // Static utility methods
    fn add(a: i32, b: i32) -> i32 {
        return a + b
    }
    
    fn subtract(a: i32, b: i32) -> i32 {
        return a - b
    }
    
    fn multiply(a: i32, b: i32) -> i32 {
        return a * b
    }
    
    fn divide(a: i32, b: i32) -> i32 {
        if b == 0 {
            return 0
        }
        return a / b
    }
    
    // Instance (self) methods
    fn add_to_current(self, value: i32) -> i32 {
        self.current_value = self.current_value + value
        self.operations_count = self.operations_count + 1
        return self.current_value
    }
    
    fn subtract_from_current(self, value: i32) -> i32 {
        self.current_value = self.current_value - value
        self.operations_count = self.operations_count + 1
        return self.current_value
    }
    
    fn multiply_current(self, value: i32) -> i32 {
        self.current_value = self.current_value * value
        self.operations_count = self.operations_count + 1
        return self.current_value
    }
    
    fn divide_current(self, value: i32) -> i32 {
        if value != 0 {
            self.current_value = self.current_value / value
            self.operations_count = self.operations_count + 1
        }
        return self.current_value
    }
    
    fn store_in_memory(self) -> i32 {
        self.memory = self.current_value
        return self.memory
    }
    
    fn recall_from_memory(self) -> i32 {
        self.current_value = self.memory
        return self.current_value
    }
    
    fn clear(self) -> i32 {
        self.current_value = 0
        return self.current_value
    }
    
    fn get_operations_count(self) -> i32 {
        return self.operations_count
    }
}

fn main() {
    // Test static methods
    print("Static add: {}",  Calculator.add(5, 3))
    print("Static subtract: {}",  Calculator.subtract(10, 4))
    print("Static multiply: {}",  Calculator.multiply(6, 7))
    print("Static divide: {}",  Calculator.divide(20, 5))

    // Test instance methods with calculator object
    let calc: Calculator = Calculator.new(10)
    print("Initial value: {}", calc.current_value)

    calc.add_to_current(5)
    print("After adding 5: {}", calc.current_value)

    calc.store_in_memory()
    calc.multiply_current(2)
    print("After multiplying by 2: {}", calc.current_value)

    calc.subtract_from_current(7)
    print("After subtracting 7: {}", calc.current_value)

    print("Value in memory: {}", calc.memory)
    calc.recall_from_memory()
    print("After memory recall: {}", calc.current_value)

    print("Operations performed: {}", calc.get_operations_count())
}
```

### `tests/structs/composition_test.orus`
Composition pattern test
```orus
// Composition pattern test
struct Shape {
    x: i32,
    y: i32,
    name: string
}

impl Shape {
    fn new(x: i32, y: i32, name: string) -> Shape {
        return Shape{x: x, y: y, name: name}
    }

    fn move_to(self, new_x: i32, new_y: i32) {
        self.x = new_x
        self.y = new_y
    }

    fn description(self) -> string {
        return self.name + " at (" + self.x + ", " + self.y + ")"
    }
}

struct Rectangle {
    shape: Shape,
    width: i32,
    height: i32
}

impl Rectangle {
    fn new(x: i32, y: i32, width: i32, height: i32) -> Rectangle {
        let shape: Shape = Shape.new(x, y, "Rectangle")
        return Rectangle{shape: shape, width: width, height: height}
    }

    fn area(self) -> i32 {
        return self.width * self.height
    }

    fn description(self) -> string {
        return self.shape.description() + " with width=" + self.width + ", height=" + self.height
    }
}

fn main() {
    let rect: Rectangle = Rectangle.new(1, 2, 3, 4)
    print(rect.description())
    rect.shape.move_to(5,6)
    print(rect.description())
}
```

### `tests/structs/impl_function.orus`
```orus
struct Math {}

impl Math {
    fn add2(n: i32) -> i32 {
        return n + 2
    }
}

fn main() {
    print(add2(3))
}
```

### `tests/structs/multi_impl.orus`
```orus
struct Math {}

impl Math {
    fn add(a: i32, b: i32) -> i32 {
        return a + b
    }

    fn sum3(a: i32, b: i32, c: i32) -> i32 {
        return add(add(a, b), c)
    }
}

fn main() {
    print(sum3(1, 2, 3))
}
```

### `tests/structs/rand.orus`
```orus
struct SeedBox {
    value: u32
}

fn rand(store: [SeedBox; 1]) -> f64 {

    let a: u32 = 1664525
    let c: u32 = 1013904223
    let m: u32 = 4294967295

    let current = store[0].value
    let next = (a * current + c) % m
    store[0].value = next

    let numerator: f64 = (next as f64) * 1.0
    return numerator / 4294967296.0
}

fn seed_random(store: [SeedBox; 1], new_seed: u32) {
    store[0].value = new_seed
}

fn rand_int(store: [SeedBox; 1], min: i32, max: i32) -> i32 {
    let range: f64 = ((max - min + 1) as f64) * 1.0
    return min + (((rand(store) * range) / 1.0) as i32)
}

fn main() {
    let store: [SeedBox; 1] = [SeedBox{ value: 123456789 }]

    print("Random float: {}", rand(store))

    let a = rand_int(store, 1, 10)
    let b = rand_int(store, 1, 10)
    let c = rand_int(store, 1, 10)

    print("Random ints: {}, {}, {}", a, b, c)

    seed_random(store, 42 as u32)
    print("Seeded float: {}", rand(store))
}
```

### `tests/structs/student_class.orus`
```orus
struct Student {
    name: string,
    id: i32,
    grades: [i32; 5],
}

impl Student {
    fn new(name: string, id: i32) -> Student {
        return Student{
            name: name,
            id: id,
            grades: [0, 0, 0, 0, 0]
        }
    }
    
    fn set_grade(self, index: i32, grade: i32) {
        if index >= 0 and index < 5 {
            self.grades[index] = grade
        }
    }
    
    fn average(self) -> i32 {
        let mut sum: i32 = 0
        for i in 0..5 {
            sum = sum + self.grades[i]
        }
        return sum / 5
    }
    
    fn highest(self) -> i32 {
        let mut max: i32 = self.grades[0]
        for i in 1..5 {
            if self.grades[i] > max {
                max = self.grades[i]
            }
        }
        return max
    }
}

struct Class {
    name: string,
    students: [Student; 3],
    count: i32,
}

impl Class {
    fn new(name: string) -> Class {
        let s1: Student = Student.new("", 0)
        let s2: Student = Student.new("", 0)
        let s3: Student = Student.new("", 0)
        
        return Class{
            name: name,
            students: [s1, s2, s3],
            count: 0
        }
    }
    
    fn add_student(self, student: Student) -> bool {
        if self.count < 3 {
            self.students[self.count] = student
            self.count = self.count + 1
            return true
        }
        return false
    }
    
    fn class_average(self) -> i32 {
        if self.count == 0 {
            return 0
        }
        
        let mut sum: i32 = 0
        for i in 0..self.count {
            sum = sum + self.students[i].average()
        }
        return sum / self.count
    }
}

fn main() {
    let alice: Student = Student.new("Alice", 101)
    alice.set_grade(0, 85)
    alice.set_grade(1, 90)
    alice.set_grade(2, 82)
    alice.set_grade(3, 88)
    alice.set_grade(4, 95)

    let bob: Student = Student.new("Bob", 102)
    bob.set_grade(0, 75)
    bob.set_grade(1, 82)
    bob.set_grade(2, 78)
    bob.set_grade(3, 80)
    bob.set_grade(4, 85)

    print("Alice's grades: {}", alice.grades)
    print("Alice's average: {}", alice.average())
    print("Alice's highest grade: {}", alice.highest())

    let math_class: Class = Class.new("Mathematics 101")
    math_class.add_student(alice)
    math_class.add_student(bob)

    print("Class name: {}", math_class.name)
    print("Number of students: {}", math_class.count)
    print("Class average: {}", math_class.class_average())
}
```

### `tests/types/array_index_assignment.orus`
Test writing to array elements
```orus
// Test writing to array elements
fn main() {
    let matrix: [[i32; 3]; 2] = [[1, 2, 3], [4, 5, 6]]

    print(matrix)
    print(matrix[0])
    print(matrix[1])
    print(matrix[0][2]) // Should be 3
    print(matrix[1][0]) // Should be 4

    matrix[1][1] = 99
    print(matrix)
}
```

### `tests/types/array_indexing.orus`
Test nested arrays (2D array)
```orus
// Test nested arrays (2D array)
fn main() {
    let matrix: [[i32; 3]; 2] = [[1, 2, 3], [4, 5, 6]]

    // Print the entire matrix
    print(matrix)

    // Print each row
    print(matrix[0])
    print(matrix[1])

    // Access and print individual elements
    print(matrix[0][2]) // Should be 3
    print(matrix[1][0]) // Should be 4
}
```

### `tests/types/array_literal.orus`
```orus
fn main() {
    let arr: [i32; 5] = [1, 2, 3, 4, 5]
    print(arr)
}
```

### `tests/types/array_of_bool.orus`
```orus
fn main() {
    let flags: [bool; 3] = [true, false, true]
    print(flags)
}
```

### `tests/types/array_operations.orus`
Test various array operations
```orus
// Test various array operations

// Array of different types
fn main() {
    let numbers: [i32; 5] = [10, 20, 30, 40, 50]
    let floats: [f64; 3] = [1.5, 2.5, 3.5]
    let mixed: [bool; 3] = [true, false, true]

    // Sum array elements
    let mut sum: i32 = 0
    for idx in 0..5 {
        sum = sum + numbers[idx]
    }
    print("Sum of numbers: {}", sum)

    // Find maximum value
    let mut max: i32 = numbers[0]
    for j in 1..5 {
        if numbers[j] > max {
            max = numbers[j]
        }
    }
    print("Maximum value: {}", max)

    // Create a new array from an existing one
    let mut doubled: [i32; 5] = [0, 0, 0, 0, 0]
    for k in 0..5 {
        doubled[k] = numbers[k] * 2
    }
    print("Doubled array: {}", doubled)
}
```

### `tests/types/array_return.orus`
```orus
fn make_numbers() -> [i32; 3] {
    let arr: [i32; 3] = [10, 20, 30]
    return arr
}

fn main() {
    let result = make_numbers()
    print(result)
}
```

### `tests/types/bool_cast.orus`
```orus
fn main() {
    let a: i32 = 3
    let b: bool = a as bool
    let c: i32 = b as i32
    let d: f64 = b as f64
    let e: bool = 0 as bool
    let f: i64 = b as i64
    let g: u64 = b as u64
    print("{} {} {} {} {} {}", b, c, d, e, f, g)
}
```

### `tests/types/dynamic_array.orus`
```orus
fn main() {
    let arr: [i32; 1] = [0]
    pop(arr)
    push(arr, 10)
    push(arr, 20)
    print(len(arr))
    let v: i32 = pop(arr)
    print(v)
    print(len(arr))
}
```

### `tests/types/dynamic_array_edge.orus`
```orus
fn main() {
    let arr: [i32; 1] = [1]
    push(arr, 2)
    push(arr, 3)
    print(len(arr))
    let a = pop(arr)
    print(a)
    let b = pop(arr)
    print(b)
    let c = pop(arr)
    print(c)
    let d = pop(arr)
    print(d)
    print(len(arr))
}
```

### `tests/types/explicit_cast.orus`
```orus
fn main() {
    let mut a: i32 = -42
    let b: u32 = a as u32
    let x: u32 = 123456
    let y: f64 = x as f64
    let z: f64 = -15.9
    let q: i32 = z as i32
    let r: u32 = z as u32
    print("{} {} {} {}", b, y, q, r)
}
```

### `tests/types/nested_array.orus`
```orus
fn main() {
    let matrix: [[i32; 2]; 2] = [[1, 2], [3, 4]]
    print(matrix)
}
```

### `tests/types/primitive_casts.orus`
```orus
fn main() {
    let a: i32 = -3
    let b: i64 = a as i64
    let c: u32 = a as u32
    let d: u64 = b as u64
    let e: i64 = d as i64
    let f: f64 = c as f64
    let g: bool = f as bool
    let h: i32 = g as i32
    let i: f64 = g as f64
    let s1: string = a as string
    let s2: string = g as string
    let s3: string = d as string
    let s4: string = f as string
    print("done")
}

```

### `tests/types/type_conversion.orus`
Test automatic type conversions
```orus
// Test automatic type conversions

fn main() {
    let i32_val: i32 = 42
    let u32_val: u32 = 100
    let f64_val: f64 = 3.14

    print("Type Conversion Test:")

    // i32 to f64 conversion
    let conv1 = (i32_val as f64) + f64_val
    print("i32 + f64 conversion (42 + 3.14): {}", conv1)

    // u32 to f64 conversion
    let conv2 = (u32_val as f64) + f64_val
    print("u32 + f64 conversion (100 + 3.14): {} ", conv2)

    // Fix: Convert u32_val to i32 before division
    let u32_as_i32: i32 = 100
    let div_result = i32_val / u32_as_i32
    print("Division operation (42 / 100): {} ", div_result)
}


```

### `tests/types/u64_cast.orus`
```orus
fn main() {
    let a: i32 = 5
    let u: u64 = a as u64
    let back_i32: i32 = u as i32
    let back_u32: u32 = u as u32
    let as_f64: f64 = u as f64
    let from_f64: u64 = 3.0 as u64
    print("{} {} {} {} {}", u, back_i32, back_u32, as_f64, from_f64)
}
```

### `tests/variables/assignment_test.orus`
Tests variable assignments
```orus
// Tests variable assignments
fn main() {
    let mut x: i32 = 10
    let mut y: i32 = 20

    print("Initial values x: {}, y: {}", x, y)

    x = 30
    print("After first assignment x: {}", x)

    y = 40
    print("After second assignment y: {}", y)

    // Advanced assignments 
    y = 50
    x = y
    print("After assignment x: {}, y: {}", x, y)

    x = x + 5
    print("After expression assignment x: {}", x)

    // Compound assignments
    x += 5
    print("After += x: {}", x)
    x -= 2
    print("After -= x: {}", x)
    x *= 3
    print("After *= x: {}", x)
    x /= 3
    print("After /= x: {}", x)
    x %= 4
    print("After %= x: {}", x)
}
```

### `tests/variables/declaration_i64.orus`
Tests variable declarations with i64 type
```orus
// Tests variable declarations with i64 type
fn main() {
    let i64_var = 3000000000
    let inferred_i64 = -4000000000

    print("i64 declaration test:")
    print(i64_var)
    print(inferred_i64)

    print("Types of vars:")
    print(type_of(i64_var))
    print(type_of(inferred_i64))
}
```

### `tests/variables/declaration_test.orus`
Tests variable declarations with different types
```orus
// Tests variable declarations with different types
fn main() {
    let i32_var: i32 = 42
    let u32_var: u32 = 100
    let f64_var: f64 = 3.14
    let bool_var: bool = true
    let inferred_i32 = 24
    let inferred_u32 = 200 as u32
    let inferred_f64 = 2.71

    print("Variable declaration test:")
    print(i32_var) 
    print(u32_var)
    print(f64_var)
    print(bool_var)
    print(inferred_i32)
    print(inferred_u32)
    print(inferred_f64)

    print("Types of vars: ")
    print(type_of(i32_var))
    print(type_of(u32_var))
    print(type_of(f64_var))
    print(type_of(bool_var))
    print(type_of(inferred_i32))
    print(type_of(inferred_u32))
    print(type_of(inferred_f64))
}
```

### `tests/variables/declaration_u64.orus`
Tests variable declarations with u64 type
```orus
// Tests variable declarations with u64 type
fn main() {
    let u64_var: u64 = 12345678901234567890
    let inferred_u64 = 42

    print("u64 declaration test:")
    print(u64_var)
    print(inferred_u64)

    print("Types of vars:")
    print(type_of(u64_var))
    print(type_of(inferred_u64))
}
```

### `tests/variables/mutable_variable.orus`
```orus
fn main() {
    let mut x: i32 = 5
    x = 10
    print(x)
}
```

### `tests/variables/nil_inferred.orus`
```orus
fn main() {
    let mut x = nil
    x = 42
    print(x)
}
```
