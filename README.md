#include "joana/joana.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace joana;

static void expect_ok(const std::string& source)
{
    auto result = compile(source);
    if (!result.graph.has_value()) {
        for (const auto& diagnostic : result.diagnostics) {
            std::cerr << diagnostic.code << ": " << diagnostic.message << "\n";
        }
        assert(false && "expected compilation to succeed");
    }
    assert(result.graph->status == GraphStatus::Verified);
}

static void expect_fail(const std::string& source, const std::string& code)
{
    auto result = compile(source);
    bool found = false;
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            found = true;
            break;
        }
    }
    if (!found) {
        for (const auto& diagnostic : result.diagnostics) {
            std::cerr << diagnostic.code << ": " << diagnostic.message << "\n";
        }
    }
    assert(found && "expected specific compiler diagnostic");
}

int main()
{
    expect_ok(R"(
flow main() -> String {
    emit "Hello, World!";
}
)"
    );

    expect_ok(R"(
operation greet(name: String) -> String = "Hello, " + name;

flow main(name: String) -> String {
    let message = greet(name);
    emit message;
}
)"
    );

    expect_ok(R"(
operation normalize(name: String) -> String = name;
operation greet(name: String) -> String = "Hello, " + name;
operation finish(message: String) -> String = message + "!";

flow main(name: String) -> String {
    let n = normalize(name);
    let g = greet(n);
    let result = finish(g);
    emit result;
}
)"
    );

    expect_ok(R"(
operation double(value: Int) -> Int = value + value;
operation negate(value: Int) -> Int = 0 - value;
operation add(left: Int, right: Int) -> Int = left + right;

flow main(value: Int) -> Int {
    let a = double(value);
    let b = negate(value);
    let result = add(a, b);
    emit result;
}
)"
    );

    expect_ok(R"(
operation add(left: Int, right: Int) -> Int = left + right;

flow main() -> Int {
    let result = add(2, 3);
    emit result;
}
)"
    );

    expect_fail(R"(
flow main() -> String {
    emit missing;
}
)"
    , "UNDEFINED_VALUE");

    expect_fail(R"(
flow main(value: Int) -> Int {
    let result = unknown(value);
    emit result;
}
)"
    , "UNDEFINED_OPERATION");

    expect_fail(R"(
operation greet(name: String) -> String = "Hello, " + name;

flow main(value: Int) -> String {
    let message = greet(value);
    emit message;
}
)"
    , "TYPE_MISMATCH");

    expect_fail(R"(
operation make_number() -> Int = 42;

flow main() -> String {
    let value = make_number();
    emit value;
}
)"
    , "OUTPUT_TYPE_MISMATCH");

    expect_fail(R"(
operation loop(value: Int) -> Int = loop(value);

flow main(value: Int) -> Int {
    let result = loop(value);
    emit result;
}
)"
    , "GRAPH_CYCLE");

    std::cout << "joana_tests: all tests passed\n";
    return 0;
}
