#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace joana {

struct SourceSpan {
    std::size_t line = 1;
    std::size_t column = 1;
    std::size_t offset = 0;
};

enum class Severity { Error, Warning };

struct Diagnostic {
    Severity severity = Severity::Error;
    std::string code;
    std::string message;
    SourceSpan span{};
};

enum class TokenKind {
    Identifier,
    Integer,
    String,
    True,
    False,
    Operation,
    Flow,
    Let,
    Emit,
    TypeInt,
    TypeBool,
    TypeString,
    TypeUnit,
    LParen,
    RParen,
    LBrace,
    RBrace,
    Colon,
    Comma,
    Semicolon,
    Equals,
    Arrow,
    Plus,
    Minus,
    Star,
    Slash,
    Bang,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    AndAnd,
    OrOr,
    EndOfFile
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string text;
    SourceSpan span{};
};

enum class Type {
    Invalid,
    Int,
    Bool,
    String,
    Unit
};

struct Expr {
    enum class Kind {
        Integer,
        StringLiteral,
        Boolean,
        Identifier,
        Call,
        Unary,
        Binary
    };

    Kind kind = Kind::Identifier;
    SourceSpan span{};
    std::string text;
    std::int64_t integer_value = 0;
    bool bool_value = false;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;
    std::vector<std::shared_ptr<Expr>> arguments;
};

struct Parameter {
    std::string name;
    Type type = Type::Invalid;
    SourceSpan span{};
};

struct Statement {
    enum class Kind { Let, Emit };
    Kind kind = Kind::Let;
    std::string name;
    std::shared_ptr<Expr> expr;
    SourceSpan span{};
};

struct OperationDecl {
    std::string name;
    std::vector<Parameter> parameters;
    Type result_type = Type::Invalid;
    std::shared_ptr<Expr> body;
    SourceSpan span{};
};

struct FlowDecl {
    std::string name;
    std::vector<Parameter> parameters;
    Type result_type = Type::Invalid;
    std::vector<Statement> statements;
    SourceSpan span{};
};

struct Program {
    std::vector<OperationDecl> operations;
    std::vector<FlowDecl> flows;
};

struct ParseResult {
    Program program;
    std::vector<Diagnostic> diagnostics;
};

struct AnalysisResult {
    Program program;
    const FlowDecl* main_flow = nullptr;
    std::vector<Diagnostic> diagnostics;
};

enum class GraphStatus { Unverified, Verified };

struct GraphPort {
    std::string name;
    Type type = Type::Invalid;
};

struct GraphInput {
    std::string name;
    Type type = Type::Invalid;
};

struct GraphNode {
    std::string node_id;
    std::string operation_name;
    Type output_type = Type::Invalid;
    SourceSpan span{};
    std::vector<GraphPort> inputs;
    std::vector<std::shared_ptr<Expr>> argument_exprs;
    std::shared_ptr<Expr> body;
    std::vector<std::string> parameter_names;
};

struct GraphEdge {
    std::string source;
    std::string destination;
    std::string destination_port;
    Type type = Type::Invalid;
};

struct GraphOutput {
    std::string source;
    Type type = Type::Invalid;
    std::shared_ptr<Expr> expr;
};

struct VerifiedFlowGraph {
    std::string flow_name;
    GraphStatus status = GraphStatus::Unverified;
    std::vector<GraphInput> inputs;
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    std::vector<GraphOutput> outputs;
    std::vector<std::string> topological_order;
    std::vector<Diagnostic> diagnostics;
};

using Value = std::variant<std::int64_t, bool, std::string>;

struct CompileResult {
    std::optional<VerifiedFlowGraph> graph;
    std::vector<Diagnostic> diagnostics;
};

struct RuntimeResult {
    bool ok = false;
    Value value = std::int64_t{0};
    std::string error;
};

std::vector<Token> lex(std::string_view source, std::vector<Diagnostic>& diagnostics);
ParseResult parse(std::string_view source);
AnalysisResult analyze(const Program& program);
CompileResult compile(std::string_view source);

std::string type_name(Type type);
std::string dump_graph(const VerifiedFlowGraph& graph);
RuntimeResult execute(const VerifiedFlowGraph& graph,
                     const std::unordered_map<std::string, Value>& inputs);
std::string value_to_string(const Value& value);

}  // namespace joana
