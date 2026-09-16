# Compiler architecture

The compiler separates parsing, semantic checks, graph construction, graph verification, topological ordering, and runtime execution. The public runtime API accepts only a graph whose status is `Verified`.
