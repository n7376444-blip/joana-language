#include "joana/joana.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace joana;

static void print_diagnostics(const std::vector<Diagnostic>& diagnostics)
{
    for (const auto& diagnostic : diagnostics) {
        const char* severity = diagnostic.severity == Severity::Error ? "error" : "warning";
        std::cerr << severity << "[" << diagnostic.code << "] " << diagnostic.message;
        if (diagnostic.span.line > 0) {
            std::cerr << " at " << diagnostic.span.line << ":" << diagnostic.span.column;
        }
        std::cerr << '\n';
    }
}

int main(int argc, char** argv)
{
    bool check_mode = false;
    bool graph_mode = false;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--check") {
            check_mode = true;
        } else if (arg == "--graph") {
            graph_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "usage: joana [--check|--graph] <file.joana>\n";
            return 0;
        } else if (path.empty()) {
            path = arg;
        }
    }

    if (path.empty()) {
        std::cerr << "usage: joana [--check|--graph] <file.joana>\n";
        return 2;
    }

    std::ifstream input(path);
    if (!input.is_open()) {
        std::cerr << "unable to open file: " << path << '\n';
        return 2;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    auto result = compile(buffer.str());
    if (!result.graph.has_value()) {
        print_diagnostics(result.diagnostics);
        return 1;
    }

    if (graph_mode) {
        std::cout << dump_graph(*result.graph);
        return 0;
    }

    if (check_mode) {
        std::cout << "Compilation successful.\nGraph verified.\n";
        return 0;
    }

    std::unordered_map<std::string, Value> values;
    for (const auto& input_decl : result.graph->inputs) {
        std::cout << input_decl.name << ": ";
        std::string raw;
        std::getline(std::cin, raw);
        if (input_decl.type == Type::Int) {
            values[input_decl.name] = std::stoll(raw);
        } else if (input_decl.type == Type::Bool) {
            values[input_decl.name] = raw == "true";
        } else {
            values[input_decl.name] = raw;
        }
    }

    RuntimeResult runtime = execute(*result.graph, values);
    if (!runtime.ok) {
        std::cerr << "runtime error: " << runtime.error << '\n';
        return 1;
    }

    std::cout << value_to_string(runtime.value) << '\n';
    return 0;
}
