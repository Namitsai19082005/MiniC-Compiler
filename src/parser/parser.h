#pragma once
#include "ast.h"
#include "lexer/lexer.h"
#include "common/error.h"
#include <memory>

namespace minic {

class Parser {
public:
    Parser(Lexer& lexer, DiagnosticEngine& diag);
    std::unique_ptr<Program> parse();

private:
    // Token management
    Token advance();
    Token expect(TokenKind kind);
    bool match(TokenKind kind);
    bool check(TokenKind kind) const;
    void synchronize();

    // Type parsing
    TypeInfo parseTypeSpec();
    bool isTypeStart() const;

    // Top-level
    std::unique_ptr<ASTNode> parseTopLevel();
    std::unique_ptr<FunctionDef> parseFunctionDef(TypeInfo ret_type, const std::string& name, SourceLoc loc);
    std::unique_ptr<GlobalDecl> parseGlobalDecl(TypeInfo type, const std::string& name, SourceLoc loc);

    // Statements
    StmtPtr parseStatement();
    StmtPtr parseDeclaration();
    StmtPtr parseBlock();
    StmtPtr parseIfStatement();
    StmtPtr parseWhileStatement();
    StmtPtr parseForStatement();
    StmtPtr parseReturnStatement();
    StmtPtr parseExprStatement();

    // Expressions (Pratt parser)
    ExprPtr parseExpression(int minBP = 0);
    ExprPtr parsePrefixExpr();
    ExprPtr parseInfixExpr(ExprPtr left, Token op_tok, int bp);
    ExprPtr parsePostfixExpr(ExprPtr left);
    ExprPtr parsePrimary();
    int getPrefixBP(TokenKind kind) const;
    int getInfixBP(TokenKind kind) const;
    bool isRightAssociative(TokenKind kind) const;
    BinOp tokenToBinOp(TokenKind kind) const;

    Lexer& lexer_;
    DiagnosticEngine& diag_;
    Token current_;
};

} // namespace minic
