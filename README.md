# Joana Language

An experimental statically typed data-flow language with verified execution graphs.

## What is Joana?

Joana is a small C++20 research prototype in which operation invocations are compiled into typed, inspectable, statically verified execution graphs. It is not production-ready, formally verified, or an autonomous AI system.

## Core idea

```text
source → lexer → parser/AST → semantic analysis → graph construction
       → graph verification → topological order → runtime
```

Operation declarations describe pure reusable behavior. Each operation invocation in `main` becomes a real graph node; references between values become typed dependency edges. The runtime accepts only a graph marked `Verified`.

```joana
operation greet(name: String) -> String = "Hello, " + name;

flow main(name: String) -> String {
    let message = greet(name);
    emit message;
}
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The executable is `build/joana` (or `build/Debug/joana` on some generators).

## Run

```sh
build/joana examples/hello.joana
build/joana --check examples/greet.joana
build/joana --graph examples/dependencies.joana
```

For flows with inputs, values are read from standard input in declared order.

## Graph model

A graph node is an operation invocation (`main.N0`, `main.N1`, ...). Ports carry `Int`, `Bool`, or `String` values. An edge means that a producer value must be available before its consumer operation executes. Flow inputs and outputs are value sources/sinks, not operation nodes. Graph construction preserves source locations and deterministic source-order node IDs; execution uses a deterministic topological order.

## Verification

The implementation checks names, operation existence, arity, types, missing inputs, invalid output types, duplicate bindings, recursive operation calls, graph endpoints, cycles, and unreachable computations. Unreachable computations are warnings; the other listed violations are errors. “Verified” means these static rules succeeded, not that arbitrary program correctness was proved.

## Runtime

The runtime is deterministic and sequential. It binds graph inputs, evaluates nodes in topological order, stores values, and returns the graph output. It receives `VerifiedFlowGraph` only; unverified graphs are rejected by the public execution API.

## AI-native boundary

No AI or LLM is executed by the MVP. The future research direction is a structured proposal interface through which a human or AI tool could propose typed operation/graph changes. Such proposals would pass through the same compiler validation and human review before execution.

## Current MVP status

Implemented: lexer, source locations, parser and AST, semantic checks, typed graph construction, verification, topological ordering, sequential pure runtime, diagnostics, CLI, graph dump, examples, and automated tests.

Proposed/future: conditionals, loops, effects, concurrency, persistent graph identity, structured AI proposals, and richer type systems.

## Repository

`include/joana/` contains the public compiler/runtime API; `src/` contains implementation and CLI; `tests/` contains executable tests; `examples/` contains programs; `docs/` records the model and limitations.

## Research question

Can a programming language make program dependencies explicit enough that compiler-generated execution graphs become a first-class artifact for human inspection, verification, and future AI-assisted development?

## License

MIT. See `LICENSE`.
