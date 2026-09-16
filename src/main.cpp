#include "joana/joana.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace joana {
namespace {

void add_error(std::vector<Diagnostic>& diagnostics,
               const std::string& code,
               const std::string& message,
               const SourceSpan& span)
{
    diagnostics.push_back({Severity::Error, code, message, span});
}

void add_warning(std::vector<Diagnostic>& diagnostics,
                 const std::string& code,
                 const std::string& message,
                 const SourceSpan& span)
{
    diagnostics.push_back({Severity::Warning, code, message, span});
}

bool has_error(const std::vector<Diagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& d) {
        return d.severity == Severity::Error;
    });
}

std::string normalized_text(const std::string& text)
{
    std::string out;
    for (char ch : text) {
        if (ch == '\n') {
            out += "\\n";
        } else if (ch == '\t') {
            out += "\\t";
        } else {
            out += ch;
        }
    }
    return out;
}

}  // namespace

std::string type_name(Type type)
{
    switch (type) {
        case Type::Int: return "Int";
        case Type::Bool: return "Bool";
        case Type::String: return "String";
        case Type::Unit: return "Unit";
        default: return "Invalid";
    }
}

std::vector<Token> lex(std::string_view source, std::vector<Diagnostic>& diagnostics)
{
    std::vector<Token> tokens;
    std::size_t index = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    auto make_span = [&](std::size_t start_offset) {
        return SourceSpan{line, column, start_offset};
    };

    auto advance = [&](char ch) {
        if (ch == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        ++index;
    };

    while (index < source.size()) {
        const char ch = source[index];
        const std::size_t start_offset = index;
        const SourceSpan start_span{line, column, start_offset};

        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            advance(ch);
            continue;
        }

        if (ch == '/' && index + 1 < source.size() && source[index + 1] == '/') {
            while (index < source.size() && source[index] != '\n') {
                advance(source[index]);
            }
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_') {
            std::string text;
            while (index < source.size() &&
                   (std::isalnum(static_cast<unsigned char>(source[index])) != 0 || source[index] == '_')) {
                text += source[index];
                advance(source[index]);
            }

            TokenKind kind = TokenKind::Identifier;
            if (text == "operation") kind = TokenKind::Operation;
            else if (text == "flow") kind = TokenKind::Flow;
            else if (text == "let") kind = TokenKind::Let;
            else if (text == "emit") kind = TokenKind::Emit;
            else if (text == "true") kind = TokenKind::True;
            else if (text == "false") kind = TokenKind::False;
            else if (text == "Int") kind = TokenKind::TypeInt;
            else if (text == "Bool") kind = TokenKind::TypeBool;
            else if (text == "String") kind = TokenKind::TypeString;
            else if (text == "Unit") kind = TokenKind::TypeUnit;

            tokens.push_back({kind, text, start_span});
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            std::string text;
            while (index < source.size() && std::isdigit(static_cast<unsigned char>(source[index])) != 0) {
                text += source[index];
                advance(source[index]);
            }
            tokens.push_back({TokenKind::Integer, text, start_span});
            continue;
        }

        if (ch == '"') {
            std::string text;
            advance(ch);
            while (index < source.size()) {
                char cur = source[index];
                if (cur == '"') {
                    advance(cur);
                    break;
                }
                if (cur == '\\' && index + 1 < source.size()) {
                    char next = source[index + 1];
                    if (next == 'n') text += '\n';
                    else if (next == 't') text += '\t';
                    else if (next == '"') text += '"';
                    else if (next == '\\') text += '\\';
                    else text += next;
                    advance(source[index]);
                    advance(source[index]);
                    continue;
                }
                text += cur;
                advance(cur);
            }
            if (index > source.size() || source[index - 1] != '"') {
                add_error(diagnostics, "UNTERMINATED_STRING", "unterminated string literal", start_span);
            }
            tokens.push_back({TokenKind::String, text, start_span});
            continue;
        }

        auto match_two = [&](char first, char second, TokenKind kind) -> bool {
            if (index + 1 < source.size() && source[index] == first && source[index + 1] == second) {
                std::string text;
                text += first;
                text += second;
                tokens.push_back({kind, text, start_span});
                advance(source[index]);
                advance(source[index]);
                return true;
            }
            return false;
        };

        if (match_two('-', '>', TokenKind::Arrow)) continue;
        if (match_two('=', '=', TokenKind::EqualEqual)) continue;
        if (match_two('!', '=', TokenKind::NotEqual)) continue;
        if (match_two('<', '=', TokenKind::LessEqual)) continue;
        if (match_two('>', '=', TokenKind::GreaterEqual)) continue;
        if (match_two('&', '&', TokenKind::AndAnd)) continue;
        if (match_two('|', '|', TokenKind::OrOr)) continue;

        TokenKind kind = TokenKind::EndOfFile;
        switch (ch) {
            case '(': kind = TokenKind::LParen; break;
            case ')': kind = TokenKind::RParen; break;
            case '{': kind = TokenKind::LBrace; break;
            case '}': kind = TokenKind::RBrace; break;
            case ':': kind = TokenKind::Colon; break;
            case ',': kind = TokenKind::Comma; break;
            case ';': kind = TokenKind::Semicolon; break;
            case '=': kind = TokenKind::Equals; break;
            case '+': kind = TokenKind::Plus; break;
            case '-': kind = TokenKind::Minus; break;
            case '*': kind = TokenKind::Star; break;
            case '/': kind = TokenKind::Slash; break;
            case '!': kind = TokenKind::Bang; break;
            case '<': kind = TokenKind::Less; break;
            case '>': kind = TokenKind::Greater; break;
            default:
                add_error(diagnostics, "INVALID_CHARACTER",
                          std::string("invalid character '") + ch + "'",
                          start_span);
                advance(ch);
                continue;
        }

        tokens.push_back({kind, std::string(1, ch), start_span});
        advance(ch);
    }

    tokens.push_back({TokenKind::EndOfFile, "", SourceSpan{line, column, index}});
    return tokens;
}

namespace {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    ParseResult parse()
    {
        while (!at(TokenKind::EndOfFile)) {
            if (match(TokenKind::Operation)) {
                OperationDecl operation = parse_operation();
                program_.operations.push_back(std::move(operation));
            } else if (match(TokenKind::Flow)) {
                FlowDecl flow = parse_flow();
                program_.flows.push_back(std::move(flow));
            } else {
                add_error(diagnostics_, "SYNTAX", "expected 'operation' or 'flow'", current().span);
                advance();
            }
        }

        return {program_, diagnostics_};
    }

private:
    const Token& current() const
    {
        return tokens_[index_];
    }

    bool at(TokenKind kind) const
    {
        return current().kind == kind;
    }

    bool match(TokenKind kind)
    {
        if (at(kind)) {
            ++index_;
            return true;
        }
        return false;
    }

    void expect(TokenKind kind, const std::string& message)
    {
        if (!match(kind)) {
            add_error(diagnostics_, "SYNTAX", message, current().span);
        }
    }

    std::string parse_identifier(const std::string& message)
    {
        if (!at(TokenKind::Identifier)) {
            add_error(diagnostics_, "SYNTAX", message, current().span);
            return {};
        }
        std::string name = current().text;
        ++index_;
        return name;
    }

    Type parse_type()
    {
        if (at(TokenKind::TypeInt)) { ++index_; return Type::Int; }
        if (at(TokenKind::TypeBool)) { ++index_; return Type::Bool; }
        if (at(TokenKind::TypeString)) { ++index_; return Type::String; }
        if (at(TokenKind::TypeUnit)) { ++index_; return Type::Unit; }
        add_error(diagnostics_, "SYNTAX", "expected type", current().span);
        return Type::Invalid;
    }

    std::vector<Parameter> parse_parameters()
    {
        std::vector<Parameter> out;
        if (at(TokenKind::RParen)) {
            return out;
        }

        do {
            const SourceSpan span = current().span;
            std::string name = parse_identifier("expected parameter name");
            expect(TokenKind::Colon, "expected ':' after parameter name");
            Type type = parse_type();
            out.push_back({name, type, span});
        } while (match(TokenKind::Comma));

        return out;
    }

    std::shared_ptr<Expr> parse_primary()
    {
        if (at(TokenKind::Integer)) {
            auto expr = std::make_shared<Expr>();
            expr->kind = Expr::Kind::Integer;
            expr->span = current().span;
            expr->text = current().text;
            expr->integer_value = std::stoll(current().text);
            ++index_;
            return expr;
        }

        if (at(TokenKind::String)) {
            auto expr = std::make_shared<Expr>();
            expr->kind = Expr::Kind::StringLiteral;
            expr->span = current().span;
            expr->text = current().text;
            ++index_;
            return expr;
        }

        if (at(TokenKind::True)) {
            auto expr = std::make_shared<Expr>();
            expr->kind = Expr::Kind::Boolean;
            expr->span = current().span;
            expr->bool_value = true;
            ++index_;
            return expr;
        }

        if (at(TokenKind::False)) {
            auto expr = std::make_shared<Expr>();
            expr->kind = Expr::Kind::Boolean;
            expr->span = current().span;
            expr->bool_value = false;
            ++index_;
            return expr;
        }

        if (match(TokenKind::LParen)) {
            auto expr = parse_expression();
            expect(TokenKind::RParen, "expected ')' after expression");
            return expr;
        }

        if (at(TokenKind::Identifier)) {
            auto expr = std::make_shared<Expr>();
            expr->kind = Expr::Kind::Identifier;
            expr->span = current().span;
            expr->text = current().text;
            ++index_;

            if (match(TokenKind::LParen)) {
                expr->kind = Expr::Kind::Call;
                if (!at(TokenKind::RParen)) {
                    do {
                        expr->arguments.push_back(parse_expression());
                    } while (match(TokenKind::Comma));
                }
                expect(TokenKind::RParen, "expected ')' after call arguments");
            }
            return expr;
        }

        add_error(diagnostics_, "SYNTAX", "expected expression", current().span);
        return std::make_shared<Expr>();
    }

    std::shared_ptr<Expr> parse_unary()
    {
        if (match(TokenKind::Minus) || match(TokenKind::Bang)) {
            auto expr = std::make_shared<Expr>();
            expr->kind = Expr::Kind::Unary;
            expr->span = current().span;
            expr->text = (tokens_[index_ - 1].kind == TokenKind::Minus) ? "-" : "!";
            expr->left = parse_unary();
            return expr;
        }
        return parse_primary();
    }

    int precedence(TokenKind kind) const
    {
        switch (kind) {
            case TokenKind::OrOr: return 1;
            case TokenKind::AndAnd: return 2;
            case TokenKind::EqualEqual:
            case TokenKind::NotEqual: return 3;
            case TokenKind::Less:
            case TokenKind::LessEqual:
            case TokenKind::Greater:
            case TokenKind::GreaterEqual: return 4;
            case TokenKind::Plus:
            case TokenKind::Minus: return 5;
            case TokenKind::Star:
            case TokenKind::Slash: return 6;
            default: return -1;
        }
    }

    std::shared_ptr<Expr> parse_expression(int min_precedence = 0)
    {
        auto left = parse_unary();

        while (true) {
            const TokenKind current_kind = current().kind;
            const int prec = precedence(current_kind);
            if (prec < min_precedence) break;

            std::string op_text = current().text;
            ++index_;
            auto right = parse_expression(prec + 1);
            auto binary = std::make_shared<Expr>();
            binary->kind = Expr::Kind::Binary;
            binary->span = left->span;
            binary->text = op_text;
            binary->left = std::move(left);
            binary->right = std::move(right);
            left = binary;
        }

        return left;
    }

    Statement parse_statement()
    {
        if (match(TokenKind::Let)) {
            Statement stmt;
            stmt.kind = Statement::Kind::Let;
            stmt.span = tokens_[index_ - 1].span;
            stmt.name = parse_identifier("expected binding name");
            expect(TokenKind::Equals, "expected '=' after binding name");
            stmt.expr = parse_expression();
            expect(TokenKind::Semicolon, "expected ';' after let statement");
            return stmt;
        }

        if (match(TokenKind::Emit)) {
            Statement stmt;
            stmt.kind = Statement::Kind::Emit;
            stmt.span = tokens_[index_ - 1].span;
            stmt.expr = parse_expression();
            expect(TokenKind::Semicolon, "expected ';' after emit statement");
            return stmt;
        }

        add_error(diagnostics_, "SYNTAX", "expected 'let' or 'emit'", current().span);
        return {};
    }

    OperationDecl parse_operation()
    {
        OperationDecl operation;
        operation.span = tokens_[index_ - 1].span;
        operation.name = parse_identifier("expected operation name");
        expect(TokenKind::LParen, "expected '(' after operation name");
        operation.parameters = parse_parameters();
        expect(TokenKind::RParen, "expected ')' after parameter list");
        expect(TokenKind::Arrow, "expected '->' after parameter list");
        operation.result_type = parse_type();
        expect(TokenKind::Equals, "expected '=' after return type");
        operation.body = parse_expression();
        expect(TokenKind::Semicolon, "expected ';' after operation body");
        return operation;
    }

    FlowDecl parse_flow()
    {
        FlowDecl flow;
        flow.span = tokens_[index_ - 1].span;
        flow.name = parse_identifier("expected flow name");
        expect(TokenKind::LParen, "expected '(' after flow name");
        flow.parameters = parse_parameters();
        expect(TokenKind::RParen, "expected ')' after flow parameters");
        expect(TokenKind::Arrow, "expected '->' after flow parameter list");
        flow.result_type = parse_type();
        expect(TokenKind::LBrace, "expected '{' to start flow body");

        while (!at(TokenKind::RBrace) && !at(TokenKind::EndOfFile)) {
            flow.statements.push_back(parse_statement());
        }

        expect(TokenKind::RBrace, "expected '}' to end flow body");
        return flow;
    }

    std::vector<Token> tokens_;
    std::size_t index_ = 0;
    Program program_;
    std::vector<Diagnostic> diagnostics_;
};

}  // namespace

ParseResult parse(std::string_view source)
{
    std::vector<Token> tokens = lex(source, ParseResult{}.diagnostics);
    Parser parser(std::move(tokens));
    return parser.parse();
}

namespace {

const OperationDecl* find_operation(const Program& program, const std::string& name)
{
    for (const auto& op : program.operations) {
        if (op.name == name) {
            return &op;
        }
    }
    return nullptr;
}

bool is_numeric(Type type)
{
    return type == Type::Int;
}

Type expression_type(const Program& program,
                     const std::shared_ptr<Expr>& expr,
                     const std::unordered_map<std::string, Type>& locals,
                     std::vector<Diagnostic>& diagnostics,
                     std::unordered_set<std::string>& active_calls)
{
    if (!expr) {
        return Type::Invalid;
    }

    switch (expr->kind) {
        case Expr::Kind::Integer: return Type::Int;
        case Expr::Kind::StringLiteral: return Type::String;
        case Expr::Kind::Boolean: return Type::Bool;
        case Expr::Kind::Identifier: {
            auto it = locals.find(expr->text);
            if (it == locals.end()) {
                add_error(diagnostics, "UNDEFINED_VALUE", "value '" + expr->text + "' is not defined", expr->span);
                return Type::Invalid;
            }
            return it->second;
        }
        case Expr::Kind::Unary: {
            Type inner = expression_type(program, expr->left, locals, diagnostics, active_calls);
            if (expr->text == "-") {
                if (inner != Type::Int) {
                    add_error(diagnostics, "TYPE_MISMATCH", "unary '-' requires Int", expr->span);
                    return Type::Invalid;
                }
                return Type::Int;
            }
            if (expr->text == "!") {
                if (inner != Type::Bool) {
                    add_error(diagnostics, "TYPE_MISMATCH", "unary '!' requires Bool", expr->span);
                    return Type::Invalid;
                }
                return Type::Bool;
            }
            return Type::Invalid;
        }
        case Expr::Kind::Call: {
            const OperationDecl* operation = find_operation(program, expr->text);
            if (!operation) {
                add_error(diagnostics, "UNDEFINED_OPERATION", "operation '" + expr->text + "' is not declared", expr->span);
                return Type::Invalid;
            }
            if (active_calls.find(operation->name) != active_calls.end()) {
                add_error(diagnostics, "GRAPH_CYCLE", "recursive operation dependency involving '" + operation->name + "'", expr->span);
                return Type::Invalid;
            }
            if (expr->arguments.size() != operation->parameters.size()) {
                add_error(diagnostics, "ARGUMENT_COUNT",
                          "operation '" + operation->name + "' expects " + std::to_string(operation->parameters.size()) +
                              " arguments but got " + std::to_string(expr->arguments.size()),
                          expr->span);
                return Type::Invalid;
            }

            std::unordered_set<std::string> next_active = active_calls;
            next_active.insert(operation->name);
            for (std::size_t i = 0; i < expr->arguments.size(); ++i) {
                Type arg_type = expression_type(program, expr->arguments[i], locals, diagnostics, next_active);
                if (arg_type != operation->parameters[i].type) {
                    add_error(diagnostics, "TYPE_MISMATCH",
                              "argument " + std::to_string(i + 1) + " of operation '" + operation->name +
                                  "' expects " + type_name(operation->parameters[i].type) +
                                  " but received " + type_name(arg_type),
                              expr->arguments[i]->span);
                }
            }
            return operation->result_type;
        }
        case Expr::Kind::Binary: {
            Type left_type = expression_type(program, expr->left, locals, diagnostics, active_calls);
            Type right_type = expression_type(program, expr->right, locals, diagnostics, active_calls);
            const std::string& op = expr->text;

            if (op == "+") {
                if (left_type == Type::String && right_type == Type::String) return Type::String;
                if (left_type == Type::Int && right_type == Type::Int) return Type::Int;
                add_error(diagnostics, "TYPE_MISMATCH", "operator '+' requires Int + Int or String + String", expr->span);
                return Type::Invalid;
            }
            if (op == "-" || op == "*" || op == "/") {
                if (left_type == Type::Int && right_type == Type::Int) return Type::Int;
                add_error(diagnostics, "TYPE_MISMATCH", "operator '" + op + "' requires Int and Int", expr->span);
                return Type::Invalid;
            }
            if (op == "==" || op == "!=") {
                if (left_type == right_type) return Type::Bool;
                add_error(diagnostics, "TYPE_MISMATCH", "operator '" + op + "' requires matching operand types", expr->span);
                return Type::Invalid;
            }
            if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                if (left_type == Type::Int && right_type == Type::Int) return Type::Bool;
                add_error(diagnostics, "TYPE_MISMATCH", "comparison operator requires Int and Int", expr->span);
                return Type::Invalid;
            }
            if (op == "&&" || op == "||") {
                if (left_type == Type::Bool && right_type == Type::Bool) return Type::Bool;
                add_error(diagnostics, "TYPE_MISMATCH", "logical operator requires Bool and Bool", expr->span);
                return Type::Invalid;
            }
            add_error(diagnostics, "TYPE_MISMATCH", "unsupported binary operator", expr->span);
            return Type::Invalid;
        }
        default: return Type::Invalid;
    }
}

}  // namespace

AnalysisResult analyze(const Program& program)
{
    AnalysisResult result;
    result.program = program;

    const FlowDecl* main_flow = nullptr;
    for (const auto& flow : program.flows) {
        if (flow.name == "main") {
            if (main_flow != nullptr) {
                add_error(result.diagnostics, "DUPLICATE_FLOW", "duplicate main flow", flow.span);
            }
            main_flow = &flow;
        }
    }
    if (!main_flow) {
        add_error(result.diagnostics, "MISSING_MAIN", "program must define a flow named 'main'", SourceSpan{});
        result.main_flow = nullptr;
        return result;
    }
    result.main_flow = main_flow;

    std::unordered_map<std::string, Type> locals;
    for (const auto& parameter : main_flow->parameters) {
        if (locals.find(parameter.name) != locals.end()) {
            add_error(result.diagnostics, "DUPLICATE_BINDING", "duplicate binding '" + parameter.name + "'", parameter.span);
        }
        locals[parameter.name] = parameter.type;
    }

    bool emitted = false;
    for (const auto& statement : main_flow->statements) {
        if (statement.kind == Statement::Kind::Let) {
            if (locals.find(statement.name) != locals.end()) {
                add_error(result.diagnostics, "DUPLICATE_BINDING", "duplicate binding '" + statement.name + "'", statement.span);
            }
            if (!statement.expr || statement.expr->kind != Expr::Kind::Call) {
                add_error(result.diagnostics, "INVALID_BINDING",
                          "a let initializer must be an operation invocation",
                          statement.span);
                continue;
            }
            std::unordered_set<std::string> active_calls;
            Type inferred = expression_type(program, statement.expr, locals, result.diagnostics, active_calls);
            locals[statement.name] = inferred;
        } else if (statement.kind == Statement::Kind::Emit) {
            if (emitted) {
                add_error(result.diagnostics, "INVALID_OUTPUT", "only one emit is permitted per flow", statement.span);
            }
            emitted = true;
            std::unordered_set<std::string> active_calls;
            Type emit_type = expression_type(program, statement.expr, locals, result.diagnostics, active_calls);
            if (emit_type != main_flow->result_type) {
                add_error(result.diagnostics, "OUTPUT_TYPE_MISMATCH",
                          "flow returns " + type_name(main_flow->result_type) +
                              " but emitted value has type " + type_name(emit_type),
                          statement.span);
            }
        }
    }

    if (!emitted) {
        add_error(result.diagnostics, "INVALID_OUTPUT", "flow must contain exactly one emit statement", main_flow->span);
    }

    return result;
}

namespace {

std::shared_ptr<Expr> clone_expr(const std::shared_ptr<Expr>& expr)
{
    if (!expr) return nullptr;
    auto copy = std::make_shared<Expr>();
    *copy = *expr;
    if (expr->left) copy->left = clone_expr(expr->left);
    if (expr->right) copy->right = clone_expr(expr->right);
    for (const auto& arg : expr->arguments) {
        copy->arguments.push_back(clone_expr(arg));
    }
    return copy;
}

std::string source_name_for_expression(const std::shared_ptr<Expr>& expr,
                                      const std::unordered_map<std::string, std::string>& value_sources)
{
    if (!expr) {
        return "";
    }
    if (expr->kind == Expr::Kind::Identifier) {
        auto it = value_sources.find(expr->text);
        if (it != value_sources.end()) {
            return it->second;
        }
        return "";
    }
    if (expr->kind == Expr::Kind::Integer || expr->kind == Expr::Kind::StringLiteral || expr->kind == Expr::Kind::Boolean) {
        return "const:" + normalized_text(expr->text);
    }
    return "";
}

std::vector<GraphOutput> build_outputs(const FlowDecl& flow,
                                      const std::unordered_map<std::string, std::string>& value_sources,
                                      const std::vector<Statement>& statements,
                                      std::vector<Diagnostic>& diagnostics)
{
    std::vector<GraphOutput> outputs;
    for (const auto& statement : statements) {
        if (statement.kind != Statement::Kind::Emit) continue;
        if (statement.expr->kind == Expr::Kind::Identifier) {
            auto it = value_sources.find(statement.expr->text);
            if (it == value_sources.end()) {
                add_error(diagnostics, "UNDEFINED_VALUE", "value '" + statement.expr->text + "' is not defined", statement.expr->span);
                continue;
            }
            outputs.push_back({it->second, flow.result_type, clone_expr(statement.expr)});
        } else {
            outputs.push_back({"const:" + normalized_text(statement.expr->text), flow.result_type, clone_expr(statement.expr)});
        }
    }
    return outputs;
}

}  // namespace

CompileResult compile(std::string_view source)
{
    ParseResult parse_result = parse(source);
    if (has_error(parse_result.diagnostics)) {
        return {std::nullopt, parse_result.diagnostics};
    }

    AnalysisResult analysis = analyze(parse_result.program);
    if (has_error(analysis.diagnostics)) {
        return {std::nullopt, analysis.diagnostics};
    }

    const FlowDecl* flow = analysis.main_flow;
    if (!flow) {
        return {std::nullopt, analysis.diagnostics};
    }

    VerifiedFlowGraph graph;
    graph.flow_name = flow->name;
    graph.status = GraphStatus::Unverified;

    for (const auto& parameter : flow->parameters) {
        graph.inputs.push_back({parameter.name, parameter.type});
    }

    std::unordered_map<std::string, std::string> value_sources;
    for (const auto& parameter : flow->parameters) {
        value_sources[parameter.name] = "flow." + parameter.name;
    }

    std::size_t node_index = 0;
    for (const auto& statement : flow->statements) {
        if (statement.kind != Statement::Kind::Let) continue;
        if (!statement.expr || statement.expr->kind != Expr::Kind::Call) {
            add_error(graph.diagnostics, "INVALID_BINDING", "let statement must invoke an operation", statement.span);
            continue;
        }

        const OperationDecl* op = find_operation(parse_result.program, statement.expr->text);
        if (!op) {
            add_error(graph.diagnostics, "UNDEFINED_OPERATION", "operation '" + statement.expr->text + "' is not declared", statement.expr->span);
            continue;
        }

        GraphNode node;
        node.node_id = flow->name + ".N" + std::to_string(node_index++);
        node.operation_name = op->name;
        node.output_type = op->result_type;
        node.span = statement.span;
        node.body = clone_expr(op->body);
        node.parameter_names.resize(op->parameters.size());
        for (std::size_t i = 0; i < op->parameters.size(); ++i) {
            node.inputs.push_back({op->parameters[i].name, op->parameters[i].type});
            node.parameter_names[i] = op->parameters[i].name;
            node.argument_exprs.push_back(clone_expr(statement.expr->arguments[i]));
            std::string source = source_name_for_expression(statement.expr->arguments[i], value_sources);
            if (source.empty()) {
                add_error(graph.diagnostics, "UNDEFINED_VALUE", "missing source for operation argument", statement.expr->arguments[i]->span);
            } else {
                graph.edges.push_back({source, node.node_id, op->parameters[i].name, op->parameters[i].type});
            }
        }
        value_sources[statement.name] = node.node_id + ".result";
        graph.nodes.push_back(node);
    }

    graph.outputs = build_outputs(*flow, value_sources, flow->statements, graph.diagnostics);
    if (graph.outputs.empty()) {
        add_error(graph.diagnostics, "INVALID_OUTPUT", "flow has no output", flow->span);
    }

    std::unordered_map<std::string, std::size_t> node_index_by_id;
    for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
        node_index_by_id[graph.nodes[i].node_id] = i;
    }

    std::vector<std::size_t> indegree(graph.nodes.size(), 0);
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    for (const auto& edge : graph.edges) {
        if (edge.destination.rfind(flow->name + ".N", 0) == 0) {
            if (node_index_by_id.find(edge.destination) == node_index_by_id.end()) {
                add_error(graph.diagnostics, "INVALID_EDGE", "destination node not found", SourceSpan{});
                continue;
            }
            indegree[node_index_by_id[edge.destination]] += 1;
            dependents[edge.source].push_back(edge.destination);
        }
    }

    std::vector<std::size_t> ready;
    for (std::size_t i = 0; i < indegree.size(); ++i) {
        if (indegree[i] == 0) {
            ready.push_back(i);
        }
    }

    std::vector<std::size_t> order_index;
    while (!ready.empty()) {
        std::size_t pos = ready.back();
        ready.pop_back();
        order_index.push_back(pos);
        for (const auto& edge : graph.edges) {
            if (edge.destination.rfind(flow->name + ".N", 0) == 0 && edge.source == graph.nodes[pos].node_id + ".result") {
                auto it = node_index_by_id.find(edge.destination);
                if (it != node_index_by_id.end()) {
                    std::size_t idx = it->second;
                    indegree[idx] -= 1;
                    if (indegree[idx] == 0) {
                        ready.push_back(idx);
                    }
                }
            }
        }
    }

    if (order_index.size() != graph.nodes.size()) {
        add_error(graph.diagnostics, "GRAPH_CYCLE", "graph contains a cycle", flow->span);
    } else {
        for (std::size_t idx : order_index) {
            graph.topological_order.push_back(graph.nodes[idx].node_id);
        }
    }

    if (graph.outputs.empty()) {
        add_error(graph.diagnostics, "INVALID_OUTPUT", "missing flow output", flow->span);
    }

    if (!has_error(graph.diagnostics)) {
        graph.status = GraphStatus::Verified;
    }

    std::unordered_set<std::string> reachable_nodes;
    for (const auto& out : graph.outputs) {
        if (out.source.rfind(flow->name + ".N", 0) == 0) {
            reachable_nodes.insert(out.source.substr(0, out.source.find(".result")));
        }
    }
    for (const auto& node : graph.nodes) {
        if (reachable_nodes.find(node.node_id) == reachable_nodes.end()) {
            add_warning(graph.diagnostics, "UNREACHABLE_NODE",
                        "operation invocation does not contribute to the flow output",
                        node.span);
        }
    }

    if (has_error(graph.diagnostics)) {
        return {std::nullopt, graph.diagnostics};
    }

    return {{graph}, graph.diagnostics};
}

namespace {

Value evaluate_expression(const std::shared_ptr<Expr>& expr,
                         const std::unordered_map<std::string, Value>& env,
                         std::vector<Diagnostic>& diagnostics)
{
    if (!expr) {
        return std::int64_t{0};
    }

    switch (expr->kind) {
        case Expr::Kind::Integer:
            return Value{expr->integer_value};
        case Expr::Kind::StringLiteral:
            return Value{expr->text};
        case Expr::Kind::Boolean:
            return Value{expr->bool_value};
        case Expr::Kind::Identifier: {
            auto it = env.find(expr->text);
            if (it == env.end()) {
                add_error(diagnostics, "UNDEFINED_VALUE", "value '" + expr->text + "' is not defined", expr->span);
                return std::int64_t{0};
            }
            return it->second;
        }
        case Expr::Kind::Unary: {
            Value value = evaluate_expression(expr->left, env, diagnostics);
            if (expr->text == "-") {
                return Value{std::int64_t(-std::get<std::int64_t>(value))};
            }
            if (expr->text == "!") {
                return Value{!std::get<bool>(value)};
            }
            return std::int64_t{0};
        }
        case Expr::Kind::Binary: {
            Value left = evaluate_expression(expr->left, env, diagnostics);
            Value right = evaluate_expression(expr->right, env, diagnostics);
            const std::string& op = expr->text;
            if (op == "+") {
                if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
                    return Value{std::get<std::string>(left) + std::get<std::string>(right)};
                }
                return Value{std::get<std::int64_t>(left) + std::get<std::int64_t>(right)};
            }
            if (op == "-") return Value{std::get<std::int64_t>(left) - std::get<std::int64_t>(right)};
            if (op == "*") return Value{std::get<std::int64_t>(left) * std::get<std::int64_t>(right)};
            if (op == "/") return Value{std::get<std::int64_t>(left) / std::get<std::int64_t>(right)};
            if (op == "==") return Value{left == right};
            if (op == "!=") return Value{left != right};
            if (op == "<") return Value{std::get<std::int64_t>(left) < std::get<std::int64_t>(right)};
            if (op == "<=") return Value{std::get<std::int64_t>(left) <= std::get<std::int64_t>(right)};
            if (op == ">") return Value{std::get<std::int64_t>(left) > std::get<std::int64_t>(right)};
            if (op == ">=") return Value{std::get<std::int64_t>(left) >= std::get<std::int64_t>(right)};
            if (op == "&&") return Value{std::get<bool>(left) && std::get<bool>(right)};
            if (op == "||") return Value{std::get<bool>(left) || std::get<bool>(right)};
            return std::int64_t{0};
        }
        case Expr::Kind::Call: {
            // The runtime supports operation invocation only when it is represented as a node.
            // Call expressions are not executed directly in the runtime.
            return std::int64_t{0};
        }
        default: return std::int64_t{0};
    }
}

RuntimeResult execute_operation(const GraphNode& node,
                               const std::unordered_map<std::string, Value>& env,
                               const std::unordered_map<std::string, std::shared_ptr<Expr>>& body_by_name)
{
    RuntimeResult result;
    std::unordered_map<std::string, Value> locals;
    for (std::size_t i = 0; i < node.parameter_names.size(); ++i) {
        std::string name = node.parameter_names[i];
        const auto& expr = node.argument_exprs[i];
        locals[name] = evaluate_expression(expr, env, result.error.empty() ? std::vector<Diagnostic>{} : std::vector<Diagnostic>{});
    }
    if (node.body) {
        Value value = evaluate_expression(node.body, locals, std::vector<Diagnostic>{});
        result.ok = true;
        result.value = value;
    }
    return result;
}

}  // namespace

RuntimeResult execute(const VerifiedFlowGraph& graph,
                     const std::unordered_map<std::string, Value>& inputs)
{
    if (graph.status != GraphStatus::Verified) {
        return {false, std::int64_t{0}, "graph is not verified"};
    }

    std::unordered_map<std::string, Value> env;
    for (const auto& input : graph.inputs) {
        auto it = inputs.find(input.name);
        if (it == inputs.end()) {
            return {false, std::int64_t{0}, "missing input '" + input.name + "'"};
        }
        env[input.name] = it->second;
    }

    std::unordered_map<std::string, Value> values;
    for (const auto& node : graph.nodes) {
        std::vector<Diagnostic> parse_diags;
        std::unordered_map<std::string, Value> local_env = env;
        for (std::size_t i = 0; i < node.parameter_names.size(); ++i) {
            const auto& expr = node.argument_exprs[i];
            local_env[node.parameter_names[i]] = evaluate_expression(expr, env, parse_diags);
        }

        if (node.body) {
            Value result = evaluate_expression(node.body, local_env, parse_diags);
            values[node.node_id + ".result"] = result;
            env[node.node_id + ".result"] = result;
        }
    }

    for (const auto& out : graph.outputs) {
        if (out.expr) {
            std::vector<Diagnostic> parse_diags;
            Value value = evaluate_expression(out.expr, env, parse_diags);
            return {true, value, {}};
        }
        auto it = env.find(out.source);
        if (it != env.end()) {
            return {true, it->second, {}};
        }
    }

    return {false, std::int64_t{0}, "missing graph output"};
}

std::string value_to_string(const Value& value)
{
    if (std::holds_alternative<std::int64_t>(value)) {
        return std::to_string(std::get<std::int64_t>(value));
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    return "";
}

std::string dump_graph(const VerifiedFlowGraph& graph)
{
    std::ostringstream out;
    out << "Flow: " << graph.flow_name << '\n';
    out << "Status: " << (graph.status == GraphStatus::Verified ? "VERIFIED" : "UNVERIFIED") << '\n';
    out << "\nInputs:\n";
    for (const auto& input : graph.inputs) {
        out << "  " << input.name << ": " << type_name(input.type) << '\n';
    }
    out << "\nNodes:\n";
    for (const auto& node : graph.nodes) {
        out << "  " << node.node_id << "\n";
        out << "    operation: " << node.operation_name << '\n';
        for (const auto& port : node.inputs) {
            out << "    input " << port.name << ": " << type_name(port.type) << '\n';
        }
        out << "    output: " << type_name(node.output_type) << '\n';
        out << "    source: line " << node.span.line << ":" << node.span.column << '\n';
    }
    out << "\nEdges:\n";
    for (const auto& edge : graph.edges) {
        out << "  " << edge.source << " -> " << edge.destination << "." << edge.destination_port << " : " << type_name(edge.type) << '\n';
    }
    out << "\nOutputs:\n";
    for (const auto& output : graph.outputs) {
        out << "  " << output.source << " : " << type_name(output.type) << '\n';
    }
    out << "\nTopological order:\n";
    for (const auto& id : graph.topological_order) {
        out << "  " << id << '\n';
    }
    return out.str();
}

}  // namespace joana
