# Joana Language

Joana Language is an experimental statically typed data-flow programming language built around a simple idea: operation invocations are compiled into typed, inspectable, and statically verified execution graphs.

The project is intentionally small and research-oriented. It is not production-ready and does not claim formal correctness beyond the implemented compiler checks.

## What Joana is

Joana is a C++20 experimental prototype in which:

- operations are reusable typed computations
- operation invocations become real graph nodes
- dependencies between values become typed graph edges
- the compiler verifies the graph before execution
- the runtime executes only a verified graph

The central technical artifact is the verified flow graph.

## Core semantic model

```text
Source
  -> Lexer
  -> Parser / AST
  -> Semantic Analysis
  -> Graph Construction
  -> Graph Verification
  -> Verified Graph
  -> Topological Ordering
  -> Runtime
```

Joana does not treat the graph as a mere diagram. The graph is a semantic compiler artifact used to determine valid dependencies and execution order.

## Example

```joana
operation greet(name: String) -> String = "Hello, " + name;

flow main(name: String) -> String {
    let message = greet(name);
    emit message;
}
```

## Architecture

The public compiler/runtime API is exposed through `include/joana/joana.hpp`.

The project is structured as:

```text
joana-language/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
├── docs/
├── examples/
├── include/
├── src/
├── tests/
└── assets/
```

## Verified graph

A `GraphNode` represents one operation invocation. A `GraphEdge` represents a typed dependency from a producer value to a consumer input. Flow inputs and the flow output are special graph ports. The runtime may execute only a graph whose status is `Verified`.

## AI-native boundary

Joana does not include an autonomous AI runtime. The implemented model supports future structured AI tooling that proposes changes to operation or graph structure, but such proposals must pass the same validation pipeline and human review before becoming executable.

## Current MVP status

Implemented:
- C++20 CMake project
- lexer
- parser and AST
- semantic validation
- graph construction
- graph verification
- cycle detection
- topological ordering
- deterministic sequential runtime
- CLI
- graph dump
- examples
- automated tests

Future work:
- conditionals
- loops
- recursion
- effects and external side effects
- concurrency
- richer type systems
- persistent graph identity
- autonomous AI execution

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Run examples

```bash
./build/joana examples/hello.joana
./build/joana --check examples/greet.joana
./build/joana --graph examples/dependencies.joana
```

## Research question

Can a programming language make program dependencies explicit enough that compiler-generated execution graphs become a first-class artifact for human inspection, verification, and future AI-assisted development?

## License

MIT.
