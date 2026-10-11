#include "parser.h"
#include <stdexcept>
#include <cassert>

namespace minic {

Parser::Parser(Lexer& lexer, DiagnosticEngine& diag)
    : lexer_(lexer), diag_(diag), current_(lexer.nextToken()) {}

// ─── Token Management ────────────────────────────────────────────────

Token Parser::advance() {
    Token prev = current_;
    current_ = lexer_.nextToken();
    return prev;
}

Token Parser::expect(TokenKind kind) {
    if (current_.kind == kind) return advance();
    diag_.error(current_.loc, "expected '" + std::string(tokenKindToString(kind))
                + "' but got '" + std::string(tokenKindToString(current_.kind)) + "'");
    return current_;
}

bool Parser::match(TokenKind kind) {
    if (current_.kind == kind) { advance(); return true; }
    return false;
}

bool Parser::check(TokenKind kind) const {
    return current_.kind == kind;
}

void Parser::synchronize() {
    while (current_.kind != TokenKind::Eof) {
        if (current_.kind == TokenKind::Semicolon) { advance(); return; }
        if (current_.kind == TokenKind::RBrace) return;
        if (isTypeStart() || current_.kind == TokenKind::KW_if ||
            current_.kind == TokenKind::KW_while || current_.kind == TokenKind::KW_for ||
            current_.kind == TokenKind::KW_return) return;
        advance();
    }
}

// ─── Type Parsing ────────────────────────────────────────────────────

bool Parser::isTypeStart() const {
    return current_.kind == TokenKind::KW_int ||
           current_.kind == TokenKind::KW_char ||
           current_.kind == TokenKind::KW_void;
}

TypeInfo Parser::parseTypeSpec() {
    TypeInfo ti;
    switch (current_.kind) {
        case TokenKind::KW_int:  ti.base = TypeInfo::Int;  advance(); break;
        case TokenKind::KW_char: ti.base = TypeInfo::Char; advance(); break;
        case TokenKind::KW_void: ti.base = TypeInfo::Void; advance(); break;
        default:
            diag_.error(current_.loc, "expected type specifier");
            ti.base = TypeInfo::Int;
    }
    while (current_.kind == TokenKind::Star) {
        ++ti.pointer_depth;
        advance();
    }
    return ti;
}

// ─── Program ─────────────────────────────────────────────────────────

std::unique_ptr<Program> Parser::parse() {
    auto prog = std::make_unique<Program>();
    prog->loc = current_.loc;
    while (current_.kind != TokenKind::Eof) {
        auto node = parseTopLevel();
        if (node) prog->decls.push_back(std::move(node));
        else synchronize();
    }
    return prog;
}

std::unique_ptr<ASTNode> Parser::parseTopLevel() {
    if (!isTypeStart()) {
        diag_.error(current_.loc, "expected type at top level");
        return nullptr;
    }
    SourceLoc loc = current_.loc;
    TypeInfo type = parseTypeSpec();

    if (current_.kind != TokenKind::Identifier) {
        diag_.error(current_.loc, "expected identifier after type");
        return nullptr;
    }
    std::string name = current_.lexeme;
    advance();

    // Function definition or declaration
    if (current_.kind == TokenKind::LParen) {
        return parseFunctionDef(type, name, loc);
    }
    // Global variable
    return parseGlobalDecl(type, name, loc);
}

std::unique_ptr<FunctionDef> Parser::parseFunctionDef(TypeInfo ret_type, const std::string& name, SourceLoc loc) {
    auto func = std::make_unique<FunctionDef>();
    func->loc = loc;
    func->return_type = ret_type;
    func->name = name;

    expect(TokenKind::LParen);

    // Parse parameters
    if (!check(TokenKind::RParen)) {
        do {
            Param p;
            p.loc = current_.loc;
            p.type = parseTypeSpec();
            if (check(TokenKind::Identifier)) {
                p.name = current_.lexeme;
                advance();
            }
            func->params.push_back(p);
        } while (match(TokenKind::Comma));
    }
    expect(TokenKind::RParen);

    // Forward declaration (ends with semicolon) vs definition (has body)
    if (check(TokenKind::Semicolon)) {
        advance();
        func->is_declaration = true;
        func->body = nullptr;
        return func;
    }

    // Parse body
    auto body = parseBlock();
    func->body.reset(static_cast<BlockStmt*>(body.release()));
    return func;
}

std::unique_ptr<GlobalDecl> Parser::parseGlobalDecl(TypeInfo type, const std::string& name, SourceLoc loc) {
    auto decl = std::make_unique<GlobalDecl>();
    decl->loc = loc;
    decl->type = type;
    decl->name = name;

    // Array declaration
    if (match(TokenKind::LBracket)) {
        decl->is_array = true;
        if (check(TokenKind::IntLiteral)) {
            decl->array_size = static_cast<int>(current_.int_value);
            advance();
        }
        expect(TokenKind::RBracket);
    }

    // Optional initializer
    if (match(TokenKind::Assign)) {
        decl->init = parseExpression();
    }

    expect(TokenKind::Semicolon);
    return decl;
}

// ─── Statements ──────────────────────────────────────────────────────

StmtPtr Parser::parseStatement() {
    if (isTypeStart()) return parseDeclaration();
    if (check(TokenKind::LBrace)) return parseBlock();
    if (check(TokenKind::KW_if)) return parseIfStatement();
    if (check(TokenKind::KW_while)) return parseWhileStatement();
    if (check(TokenKind::KW_for)) return parseForStatement();
    if (check(TokenKind::KW_return)) return parseReturnStatement();
    return parseExprStatement();
}

StmtPtr Parser::parseDeclaration() {
    auto stmt = std::make_unique<DeclStmt>();
    stmt->loc = current_.loc;
    stmt->type = parseTypeSpec();

    if (current_.kind != TokenKind::Identifier) {
        diag_.error(current_.loc, "expected variable name");
        synchronize();
        return stmt;
    }
    stmt->name = current_.lexeme;
    advance();

    // Array
    if (match(TokenKind::LBracket)) {
        stmt->is_array = true;
        if (check(TokenKind::IntLiteral)) {
            stmt->array_size = static_cast<int>(current_.int_value);
            advance();
        }
        expect(TokenKind::RBracket);
    }

    // Optional initializer
    if (match(TokenKind::Assign)) {
        stmt->init = parseExpression();
    }

    expect(TokenKind::Semicolon);
    return stmt;
}

StmtPtr Parser::parseBlock() {
    auto block = std::make_unique<BlockStmt>();
    block->loc = current_.loc;
    expect(TokenKind::LBrace);
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        auto s = parseStatement();
        if (s) block->stmts.push_back(std::move(s));
        else synchronize();
    }
    expect(TokenKind::RBrace);
    return block;
}

StmtPtr Parser::parseIfStatement() {
    auto stmt = std::make_unique<IfStmt>();
    stmt->loc = current_.loc;
    expect(TokenKind::KW_if);
    expect(TokenKind::LParen);
    stmt->cond = parseExpression();
    expect(TokenKind::RParen);
    stmt->then_branch = parseStatement();
    if (match(TokenKind::KW_else)) {
        stmt->else_branch = parseStatement();
    }
    return stmt;
}

StmtPtr Parser::parseWhileStatement() {
    auto stmt = std::make_unique<WhileStmt>();
    stmt->loc = current_.loc;
    expect(TokenKind::KW_while);
    expect(TokenKind::LParen);
    stmt->cond = parseExpression();
    expect(TokenKind::RParen);
    stmt->body = parseStatement();
    return stmt;
}

StmtPtr Parser::parseForStatement() {
    auto stmt = std::make_unique<ForStmt>();
    stmt->loc = current_.loc;
    expect(TokenKind::KW_for);
    expect(TokenKind::LParen);

    // Init
    if (check(TokenKind::Semicolon)) {
        advance();
        stmt->init = nullptr;
    } else if (isTypeStart()) {
        stmt->init = parseDeclaration();
    } else {
        stmt->init = parseExprStatement();
    }

    // Condition
    if (!check(TokenKind::Semicolon)) {
        stmt->cond = parseExpression();
    }
    expect(TokenKind::Semicolon);

    // Increment
    if (!check(TokenKind::RParen)) {
        stmt->incr = parseExpression();
    }
    expect(TokenKind::RParen);

    stmt->body = parseStatement();
    return stmt;
}

StmtPtr Parser::parseReturnStatement() {
    auto stmt = std::make_unique<ReturnStmt>();
    stmt->loc = current_.loc;
    expect(TokenKind::KW_return);
    if (!check(TokenKind::Semicolon)) {
        stmt->value = parseExpression();
    }
    expect(TokenKind::Semicolon);
    return stmt;
}

StmtPtr Parser::parseExprStatement() {
    auto stmt = std::make_unique<ExprStmt>();
    stmt->loc = current_.loc;
    stmt->expr = parseExpression();
    expect(TokenKind::Semicolon);
    return stmt;
}

// ─── Pratt Expression Parser ──────────────────────────────────────────

int Parser::getPrefixBP(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Minus:
        case TokenKind::Bang:
        case TokenKind::Tilde:
        case TokenKind::Star:      // dereference
        case TokenKind::Ampersand: // address-of
            return 120;
        default: return -1;
    }
}

int Parser::getInfixBP(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Assign:
        case TokenKind::PlusAssign:
        case TokenKind::MinusAssign:
        case TokenKind::StarAssign:
        case TokenKind::SlashAssign:
            return 10;
        case TokenKind::PipePipe:  return 20;
        case TokenKind::AmpAmp:    return 30;
        case TokenKind::Pipe:      return 40;
        case TokenKind::Caret:     return 50;
        case TokenKind::Ampersand: return 60;
        case TokenKind::EqEq:
        case TokenKind::BangEq:    return 70;
        case TokenKind::Less:
        case TokenKind::Greater:
        case TokenKind::LessEq:
        case TokenKind::GreaterEq: return 80;
        case TokenKind::ShiftLeft:
        case TokenKind::ShiftRight: return 85;
        case TokenKind::Plus:
        case TokenKind::Minus:     return 90;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:   return 100;
        default: return -1;
    }
}

bool Parser::isRightAssociative(TokenKind kind) const {
    return kind == TokenKind::Assign || kind == TokenKind::PlusAssign ||
           kind == TokenKind::MinusAssign || kind == TokenKind::StarAssign ||
           kind == TokenKind::SlashAssign;
}

BinOp Parser::tokenToBinOp(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Plus:       return BinOp::Add;
        case TokenKind::Minus:      return BinOp::Sub;
        case TokenKind::Star:       return BinOp::Mul;
        case TokenKind::Slash:      return BinOp::Div;
        case TokenKind::Percent:    return BinOp::Mod;
        case TokenKind::Ampersand:  return BinOp::BitAnd;
        case TokenKind::Pipe:       return BinOp::BitOr;
        case TokenKind::Caret:      return BinOp::BitXor;
        case TokenKind::ShiftLeft:  return BinOp::ShiftL;
        case TokenKind::ShiftRight: return BinOp::ShiftR;
        case TokenKind::AmpAmp:     return BinOp::LogAnd;
        case TokenKind::PipePipe:   return BinOp::LogOr;
        case TokenKind::EqEq:       return BinOp::Eq;
        case TokenKind::BangEq:     return BinOp::NEq;
        case TokenKind::Less:       return BinOp::Lt;
        case TokenKind::Greater:    return BinOp::Gt;
        case TokenKind::LessEq:     return BinOp::LEq;
        case TokenKind::GreaterEq:  return BinOp::GEq;
        default: return BinOp::Add; // unreachable
    }
}

ExprPtr Parser::parseExpression(int minBP) {
    ExprPtr left = parsePrefixExpr();
    if (!left) return nullptr;

    while (true) {
        // Postfix: function call and array subscript
        if (check(TokenKind::LParen) || check(TokenKind::LBracket)) {
            left = parsePostfixExpr(std::move(left));
            continue;
        }

        int bp = getInfixBP(current_.kind);
        if (bp < 0 || bp < minBP) break;

        // Assignment operators
        if (current_.kind == TokenKind::Assign || current_.kind == TokenKind::PlusAssign ||
            current_.kind == TokenKind::MinusAssign || current_.kind == TokenKind::StarAssign ||
            current_.kind == TokenKind::SlashAssign) {
            Token op_tok = advance();
            AssignOp aop;
            switch (op_tok.kind) {
                case TokenKind::Assign:      aop = AssignOp::Assign; break;
                case TokenKind::PlusAssign:  aop = AssignOp::AddAssign; break;
                case TokenKind::MinusAssign: aop = AssignOp::SubAssign; break;
                case TokenKind::StarAssign:  aop = AssignOp::MulAssign; break;
                case TokenKind::SlashAssign: aop = AssignOp::DivAssign; break;
                default: aop = AssignOp::Assign;
            }
            auto assign = std::make_unique<AssignExpr>();
            assign->loc = op_tok.loc;
            assign->op = aop;
            assign->target = std::move(left);
            assign->value = parseExpression(bp); // right-associative
            left = std::move(assign);
            continue;
        }

        Token op_tok = advance();
        int next_bp = isRightAssociative(op_tok.kind) ? bp : bp + 1;
        left = parseInfixExpr(std::move(left), op_tok, next_bp);
    }

    return left;
}

ExprPtr Parser::parsePrefixExpr() {
    // Unary prefix operators
    if (current_.kind == TokenKind::Minus || current_.kind == TokenKind::Bang ||
        current_.kind == TokenKind::Tilde) {
        Token op = advance();
        auto expr = std::make_unique<UnaryExpr>();
        expr->loc = op.loc;
        switch (op.kind) {
            case TokenKind::Minus: expr->op = UnaryOp::Neg; break;
            case TokenKind::Bang:  expr->op = UnaryOp::LogNot; break;
            case TokenKind::Tilde: expr->op = UnaryOp::BitNot; break;
            default: break;
        }
        expr->operand = parseExpression(getPrefixBP(op.kind));
        return expr;
    }
    // Dereference
    if (current_.kind == TokenKind::Star) {
        Token op = advance();
        auto expr = std::make_unique<UnaryExpr>();
        expr->loc = op.loc;
        expr->op = UnaryOp::Deref;
        expr->operand = parseExpression(getPrefixBP(op.kind));
        return expr;
    }
    // Address-of
    if (current_.kind == TokenKind::Ampersand) {
        Token op = advance();
        auto expr = std::make_unique<UnaryExpr>();
        expr->loc = op.loc;
        expr->op = UnaryOp::AddrOf;
        expr->operand = parseExpression(getPrefixBP(op.kind));
        return expr;
    }
    return parsePrimary();
}

ExprPtr Parser::parseInfixExpr(ExprPtr left, Token op_tok, int bp) {
    auto expr = std::make_unique<BinaryExpr>();
    expr->loc = op_tok.loc;
    expr->op = tokenToBinOp(op_tok.kind);
    expr->lhs = std::move(left);
    expr->rhs = parseExpression(bp);
    return expr;
}

ExprPtr Parser::parsePostfixExpr(ExprPtr left) {
    // Function call
    if (check(TokenKind::LParen)) {
        advance();
        auto call = std::make_unique<CallExpr>();
        call->loc = left->loc;
        // Extract function name from IdentExpr
        if (auto* ident = dynamic_cast<IdentExpr*>(left.get())) {
            call->callee = ident->name;
        } else {
            diag_.error(left->loc, "expected function name before '('");
            call->callee = "<error>";
        }
        if (!check(TokenKind::RParen)) {
            do {
                call->args.push_back(parseExpression(0));
            } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RParen);
        return call;
    }
    // Array subscript
    if (check(TokenKind::LBracket)) {
        advance();
        auto sub = std::make_unique<ArraySubscriptExpr>();
        sub->loc = left->loc;
        sub->array = std::move(left);
        sub->index = parseExpression();
        expect(TokenKind::RBracket);
        return sub;
    }
    return left;
}

ExprPtr Parser::parsePrimary() {
    switch (current_.kind) {
        case TokenKind::IntLiteral: {
            auto expr = std::make_unique<IntLiteralExpr>();
            expr->loc = current_.loc;
            expr->value = current_.int_value;
            advance();
            return expr;
        }
        case TokenKind::CharLiteral: {
            auto expr = std::make_unique<CharLiteralExpr>();
            expr->loc = current_.loc;
            expr->value = current_.char_value;
            advance();
            return expr;
        }
        case TokenKind::StringLiteral: {
            auto expr = std::make_unique<StringLiteralExpr>();
            expr->loc = current_.loc;
            expr->value = current_.lexeme;
            advance();
            return expr;
        }
        case TokenKind::Identifier: {
            auto expr = std::make_unique<IdentExpr>();
            expr->loc = current_.loc;
            expr->name = current_.lexeme;
            advance();
            return expr;
        }
        case TokenKind::LParen: {
            advance();
            auto expr = parseExpression();
            expect(TokenKind::RParen);
            return expr;
        }
        default:
            diag_.error(current_.loc, "expected expression, got '" +
                        std::string(tokenKindToString(current_.kind)) + "'");
            advance();
            auto err = std::make_unique<IntLiteralExpr>();
            err->loc = current_.loc;
            err->value = 0;
            return err;
    }
}

} // namespace minic
