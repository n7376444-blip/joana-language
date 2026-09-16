# Architecture

The compiler pipeline is:

source -> lexer -> parser -> AST -> semantic analysis -> graph construction -> graph verification -> topological ordering -> runtime

The runtime accepts only verified graphs.
