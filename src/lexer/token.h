#pragma once
#include "common/source_location.h"
#include <string>
#include <iostream>

namespace minic {

enum class TokenKind {
    // Literals
    IntLiteral, CharLiteral, StringLiteral, Identifier,
    // Keywords
    KW_int, KW_char, KW_void, KW_if, KW_else, KW_while,
    KW_for, KW_return,
    // Operators
    Plus, Minus, Star, Slash, Percent,
    Ampersand, Pipe, Caret, Tilde, Bang,
    ShiftLeft, ShiftRight,
    AmpAmp, PipePipe,
    EqEq, BangEq, Less, Greater, LessEq, GreaterEq,
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign,
    // Delimiters
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Semicolon, Comma,
    // Special
    Eof, Error
};

const char* tokenKindToString(TokenKind kind);

struct Token {
    TokenKind kind;
    std::string lexeme;
    SourceLoc loc;
    int64_t int_value = 0;    // for IntLiteral
    char char_value = 0;      // for CharLiteral
};

std::ostream& operator<<(std::ostream& os, const Token& tok);

} // namespace minic
