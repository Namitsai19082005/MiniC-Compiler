#pragma once
#include "token.h"
#include "common/error.h"
#include <string>
#include <unordered_map>

namespace minic {

class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename, DiagnosticEngine& diag);

    Token nextToken();
    const std::string& source() const { return source_; }

private:
    char current() const;
    char peek() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespaceAndComments();
    void skipLineComment();
    void skipBlockComment();

    Token makeToken(TokenKind kind, const std::string& lexeme);
    Token makeToken(TokenKind kind, const std::string& lexeme, SourceLoc loc);

    Token lexNumber();
    Token lexIdentifierOrKeyword();
    Token lexCharLiteral();
    Token lexStringLiteral();

    std::string source_;
    std::string filename_;
    DiagnosticEngine& diag_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    static std::unordered_map<std::string, TokenKind> keywords_;
};

} // namespace minic
