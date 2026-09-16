#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>

namespace joana {
struct Span { std::size_t line=1, column=1, offset=0; };
enum class Severity { Error, Warning };
struct Diagnostic { Severity severity; std::string code, message; Span span; };
enum class TokenKind { Identifier, Integer, String, True, False, Operation, Flow, Let, Emit, TypeInt, TypeBool, TypeString, TypeUnit, LParen,RParen,LBrace,RBrace,Colon,Comma,Semicolon,Equals,Arrow,Plus,Minus,Star,Slash,Bang,EqualEqual,NotEqual,Less,LessEqual,Greater,GreaterEqual,AndAnd,OrOr, End };
struct Token { TokenKind kind; std::string text; Span span; };
enum class Type { Int, Bool, String, Unit, Invalid };
struct Expr { enum class Kind { Int, Bool, String, Name, Call, Unary, Binary }; Kind kind; Span span; std::string text; std::int64_t integer=0; bool boolean=false; std::vector<std::shared_ptr<Expr>> args; std::shared_ptr<Expr> left,right; };
struct Parameter { std::string name; Type type; Span span; };
struct Statement { enum class Kind { Let, Emit }; Kind kind; std::string name; std::shared_ptr<Expr> expr; Span span; };
struct Operation { std::string name; std::vector<Parameter> parameters; Type result; std::shared_ptr<Expr> body; Span span; };
struct Flow { std::string name; std::vector<Parameter> parameters; Type result; std::vector<Statement> statements; Span span; };
struct Program { std::vector<Operation> operations; std::vector<Flow> flows; };
struct ParseResult { Program program; std::vector<Diagnostic> diagnostics; };
struct Analysis { Program program; const Flow* main=nullptr; std::vector<Diagnostic> diagnostics; };
using NodeId=std::uint32_t;
struct InputPort { std::string name; Type type; bool connected=false; };
struct OutputPort { std::string name="result"; Type type; };
struct GraphInput { std::string name; Type type; };
struct GraphOutput { Type type; std::string source; };
struct GraphNode { NodeId id; const Operation* operation; std::string operation_id; Span span; std::vector<InputPort> inputs; OutputPort output; std::vector<std::shared_ptr<Expr>> arguments; };
struct GraphEdge { std::string source; NodeId destination; std::string destination_port; Type type; };
enum class GraphStatus { Unverified, Verified };
struct FlowGraph { std::string flow_name; GraphStatus status=GraphStatus::Unverified; std::vector<GraphInput> inputs; std::vector<GraphNode> nodes; std::vector<GraphEdge> edges; std::vector<GraphOutput> outputs; std::vector<NodeId> order; std::vector<Diagnostic> diagnostics; };
using VerifiedFlowGraph=FlowGraph;
using Value=std::variant<std::int64_t,bool,std::string>;
struct CompileResult { std::optional<VerifiedFlowGraph> graph; std::vector<Diagnostic> diagnostics; };
std::vector<Token> lex(std::string_view source, std::vector<Diagnostic>& diagnostics);
ParseResult parse(std::string_view source);
Analysis analyze(Program program);
CompileResult compile(std::string_view source);
std::string type_name(Type type);
std::string dump_graph(const VerifiedFlowGraph& graph);
struct RuntimeResult { bool ok=false; Value value; std::string error; };
RuntimeResult execute(const VerifiedFlowGraph& graph, const std::unordered_map<std::string,Value>& inputs);
std::string value_string(const Value& value);
}
