#include "lexer.h"
#include <cctype>

namespace minic {

std::unordered_map<std::string, TokenKind> Lexer::keywords_ = {
    {"int",    TokenKind::KW_int},
    {"char",   TokenKind::KW_char},
    {"void",   TokenKind::KW_void},
    {"if",     TokenKind::KW_if},
    {"else",   TokenKind::KW_else},
    {"while",  TokenKind::KW_while},
    {"for",    TokenKind::KW_for},
    {"return", TokenKind::KW_return},
};

const char* tokenKindToString(TokenKind kind) {
    switch (kind) {
        case TokenKind::IntLiteral:    return "IntLiteral";
        case TokenKind::CharLiteral:   return "CharLiteral";
        case TokenKind::StringLiteral: return "StringLiteral";
        case TokenKind::Identifier:    return "Identifier";
        case TokenKind::KW_int:        return "int";
        case TokenKind::KW_char:       return "char";
        case TokenKind::KW_void:       return "void";
        case TokenKind::KW_if:         return "if";
        case TokenKind::KW_else:       return "else";
        case TokenKind::KW_while:      return "while";
        case TokenKind::KW_for:        return "for";
        case TokenKind::KW_return:     return "return";
        case TokenKind::Plus:          return "+";
        case TokenKind::Minus:         return "-";
        case TokenKind::Star:          return "*";
        case TokenKind::Slash:         return "/";
        case TokenKind::Percent:       return "%";
        case TokenKind::Ampersand:     return "&";
        case TokenKind::Pipe:          return "|";
        case TokenKind::Caret:         return "^";
        case TokenKind::Tilde:         return "~";
        case TokenKind::Bang:          return "!";
        case TokenKind::ShiftLeft:     return "<<";
        case TokenKind::ShiftRight:    return ">>";
        case TokenKind::AmpAmp:        return "&&";
        case TokenKind::PipePipe:      return "||";
        case TokenKind::EqEq:          return "==";
        case TokenKind::BangEq:        return "!=";
        case TokenKind::Less:          return "<";
        case TokenKind::Greater:       return ">";
        case TokenKind::LessEq:        return "<=";
        case TokenKind::GreaterEq:     return ">=";
        case TokenKind::Assign:        return "=";
        case TokenKind::PlusAssign:    return "+=";
        case TokenKind::MinusAssign:   return "-=";
        case TokenKind::StarAssign:    return "*=";
        case TokenKind::SlashAssign:   return "/=";
        case TokenKind::LParen:        return "(";
        case TokenKind::RParen:        return ")";
        case TokenKind::LBrace:        return "{";
        case TokenKind::RBrace:        return "}";
        case TokenKind::LBracket:      return "[";
        case TokenKind::RBracket:      return "]";
        case TokenKind::Semicolon:     return ";";
        case TokenKind::Comma:         return ",";
        case TokenKind::Eof:           return "EOF";
        case TokenKind::Error:         return "Error";
    }
    return "Unknown";
}

std::ostream& operator<<(std::ostream& os, const Token& tok) {
    os << tokenKindToString(tok.kind);
    if (!tok.lexeme.empty()) os << "('" << tok.lexeme << "')";
    return os;
}

Lexer::Lexer(const std::string& source, const std::string& filename, DiagnosticEngine& diag)
    : source_(source), filename_(filename), diag_(diag) {}

char Lexer::current() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::peek() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else { ++col_; }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = current();
        if (std::isspace(c)) {
            advance();
        } else if (c == '/' && peek() == '/') {
            skipLineComment();
        } else if (c == '/' && peek() == '*') {
            skipBlockComment();
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    advance(); advance(); // skip //
    while (!isAtEnd() && current() != '\n') advance();
}

void Lexer::skipBlockComment() {
    SourceLoc start = {filename_, line_, col_};
    advance(); advance(); // skip /*
    while (!isAtEnd()) {
        if (current() == '*' && peek() == '/') {
            advance(); advance();
            return;
        }
        advance();
    }
    diag_.error(start, "unterminated block comment");
}

Token Lexer::makeToken(TokenKind kind, const std::string& lexeme) {
    return Token{kind, lexeme, {filename_, line_, col_}};
}

Token Lexer::makeToken(TokenKind kind, const std::string& lexeme, SourceLoc loc) {
    return Token{kind, lexeme, loc};
}

Token Lexer::lexNumber() {
    SourceLoc loc = {filename_, line_, col_};
    std::string num;
    while (!isAtEnd() && std::isdigit(current())) {
        num += current();
        advance();
    }
    Token tok = makeToken(TokenKind::IntLiteral, num, loc);
    tok.int_value = std::stoll(num);
    return tok;
}

Token Lexer::lexIdentifierOrKeyword() {
    SourceLoc loc = {filename_, line_, col_};
    std::string ident;
    while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
        ident += current();
        advance();
    }
    auto it = keywords_.find(ident);
    if (it != keywords_.end()) {
        return makeToken(it->second, ident, loc);
    }
    return makeToken(TokenKind::Identifier, ident, loc);
}

Token Lexer::lexCharLiteral() {
    SourceLoc loc = {filename_, line_, col_};
    advance(); // skip opening '
    char val = '\0';
    if (!isAtEnd() && current() == '\\') {
        advance();
        switch (current()) {
            case 'n': val = '\n'; break;
            case 't': val = '\t'; break;
            case '\\': val = '\\'; break;
            case '\'': val = '\''; break;
            case '0': val = '\0'; break;
            default:
                diag_.error(loc, std::string("unknown escape sequence: \\") + current());
                val = current();
        }
        advance();
    } else if (!isAtEnd()) {
        val = current();
        advance();
    }
    if (isAtEnd() || current() != '\'') {
        diag_.error(loc, "unterminated character literal");
    } else {
        advance(); // skip closing '
    }
    Token tok = makeToken(TokenKind::CharLiteral, std::string(1, val), loc);
    tok.char_value = val;
    return tok;
}

Token Lexer::lexStringLiteral() {
    SourceLoc loc = {filename_, line_, col_};
    advance(); // skip opening "
    std::string str;
    while (!isAtEnd() && current() != '"') {
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case '\\': str += '\\'; break;
                case '"': str += '"'; break;
                case '0': str += '\0'; break;
                default:
                    diag_.error({filename_, line_, col_}, std::string("unknown escape: \\") + current());
                    str += current();
            }
        } else {
            str += current();
        }
        advance();
    }
    if (isAtEnd()) {
        diag_.error(loc, "unterminated string literal");
    } else {
        advance(); // skip closing "
    }
    return makeToken(TokenKind::StringLiteral, str, loc);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();
    if (isAtEnd()) return makeToken(TokenKind::Eof, "");

    SourceLoc loc = {filename_, line_, col_};
    char c = current();

    // Numbers
    if (std::isdigit(c)) return lexNumber();
    // Identifiers / keywords
    if (std::isalpha(c) || c == '_') return lexIdentifierOrKeyword();
    // Char literal
    if (c == '\'') return lexCharLiteral();
    // String literal
    if (c == '"') return lexStringLiteral();

    // Operators and delimiters
    advance();
    switch (c) {
        case '(': return makeToken(TokenKind::LParen, "(", loc);
        case ')': return makeToken(TokenKind::RParen, ")", loc);
        case '{': return makeToken(TokenKind::LBrace, "{", loc);
        case '}': return makeToken(TokenKind::RBrace, "}", loc);
        case '[': return makeToken(TokenKind::LBracket, "[", loc);
        case ']': return makeToken(TokenKind::RBracket, "]", loc);
        case ';': return makeToken(TokenKind::Semicolon, ";", loc);
        case ',': return makeToken(TokenKind::Comma, ",", loc);
        case '~': return makeToken(TokenKind::Tilde, "~", loc);
        case '^': return makeToken(TokenKind::Caret, "^", loc);
        case '%': return makeToken(TokenKind::Percent, "%", loc);
        case '+':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::PlusAssign, "+=", loc); }
            return makeToken(TokenKind::Plus, "+", loc);
        case '-':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::MinusAssign, "-=", loc); }
            return makeToken(TokenKind::Minus, "-", loc);
        case '*':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::StarAssign, "*=", loc); }
            return makeToken(TokenKind::Star, "*", loc);
        case '/':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::SlashAssign, "/=", loc); }
            return makeToken(TokenKind::Slash, "/", loc);
        case '=':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::EqEq, "==", loc); }
            return makeToken(TokenKind::Assign, "=", loc);
        case '!':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::BangEq, "!=", loc); }
            return makeToken(TokenKind::Bang, "!", loc);
        case '<':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::LessEq, "<=", loc); }
            if (!isAtEnd() && current() == '<') { advance(); return makeToken(TokenKind::ShiftLeft, "<<", loc); }
            return makeToken(TokenKind::Less, "<", loc);
        case '>':
            if (!isAtEnd() && current() == '=') { advance(); return makeToken(TokenKind::GreaterEq, ">=", loc); }
            if (!isAtEnd() && current() == '>') { advance(); return makeToken(TokenKind::ShiftRight, ">>", loc); }
            return makeToken(TokenKind::Greater, ">", loc);
        case '&':
            if (!isAtEnd() && current() == '&') { advance(); return makeToken(TokenKind::AmpAmp, "&&", loc); }
            return makeToken(TokenKind::Ampersand, "&", loc);
        case '|':
            if (!isAtEnd() && current() == '|') { advance(); return makeToken(TokenKind::PipePipe, "||", loc); }
            return makeToken(TokenKind::Pipe, "|", loc);
        default:
            diag_.error(loc, std::string("unexpected character: '") + c + "'");
            return makeToken(TokenKind::Error, std::string(1, c), loc);
    }
}

} // namespace minic
