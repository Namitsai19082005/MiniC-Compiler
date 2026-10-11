#include "ast_printer.h"

namespace minic {

const char* binOpToString(BinOp op) {
    switch (op) {
        case BinOp::Add: return "+"; case BinOp::Sub: return "-";
        case BinOp::Mul: return "*"; case BinOp::Div: return "/";
        case BinOp::Mod: return "%"; case BinOp::BitAnd: return "&";
        case BinOp::BitOr: return "|"; case BinOp::BitXor: return "^";
        case BinOp::ShiftL: return "<<"; case BinOp::ShiftR: return ">>";
        case BinOp::LogAnd: return "&&"; case BinOp::LogOr: return "||";
        case BinOp::Eq: return "=="; case BinOp::NEq: return "!=";
        case BinOp::Lt: return "<"; case BinOp::Gt: return ">";
        case BinOp::LEq: return "<="; case BinOp::GEq: return ">=";
    }
    return "?";
}

const char* unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::Neg: return "-"; case UnaryOp::LogNot: return "!";
        case UnaryOp::BitNot: return "~"; case UnaryOp::Deref: return "*";
        case UnaryOp::AddrOf: return "&";
    }
    return "?";
}

void ASTPrinter::print(ASTNode& node) { node.accept(*this); }

void ASTPrinter::indent() {
    for (int i = 0; i < depth_; ++i) os_ << "  ";
}

void ASTPrinter::visit(IntLiteralExpr& n)    { indent(); os_ << "IntLiteral(" << n.value << ")\n"; }
void ASTPrinter::visit(CharLiteralExpr& n)   { indent(); os_ << "CharLiteral('" << n.value << "')\n"; }
void ASTPrinter::visit(StringLiteralExpr& n) { indent(); os_ << "StringLiteral(\"" << n.value << "\")\n"; }
void ASTPrinter::visit(IdentExpr& n)         { indent(); os_ << "Ident(" << n.name << ")\n"; }

void ASTPrinter::visit(BinaryExpr& n) {
    indent(); os_ << "BinaryExpr(" << binOpToString(n.op) << ")\n";
    ++depth_; n.lhs->accept(*this); n.rhs->accept(*this); --depth_;
}

void ASTPrinter::visit(UnaryExpr& n) {
    indent(); os_ << "UnaryExpr(" << unaryOpToString(n.op) << ")\n";
    ++depth_; n.operand->accept(*this); --depth_;
}

void ASTPrinter::visit(CallExpr& n) {
    indent(); os_ << "Call(" << n.callee << ")\n";
    ++depth_;
    for (auto& a : n.args) a->accept(*this);
    --depth_;
}

void ASTPrinter::visit(ArraySubscriptExpr& n) {
    indent(); os_ << "ArraySubscript\n";
    ++depth_; n.array->accept(*this); n.index->accept(*this); --depth_;
}

void ASTPrinter::visit(AssignExpr& n) {
    indent(); os_ << "Assign\n";
    ++depth_; n.target->accept(*this); n.value->accept(*this); --depth_;
}

void ASTPrinter::visit(CastExpr& n) {
    indent(); os_ << "Cast(" << n.target_type.toString() << ")\n";
    ++depth_; n.operand->accept(*this); --depth_;
}

void ASTPrinter::visit(SizeofExpr& n) {
    indent(); os_ << "Sizeof(" << n.query_type.toString() << ")\n";
}

void ASTPrinter::visit(DeclStmt& n) {
    indent(); os_ << "Decl(" << n.type.toString() << " " << n.name;
    if (n.is_array) os_ << "[" << n.array_size << "]";
    os_ << ")\n";
    if (n.init) { ++depth_; n.init->accept(*this); --depth_; }
}

void ASTPrinter::visit(ExprStmt& n) {
    indent(); os_ << "ExprStmt\n";
    ++depth_; n.expr->accept(*this); --depth_;
}

void ASTPrinter::visit(BlockStmt& n) {
    indent(); os_ << "Block\n";
    ++depth_;
    for (auto& s : n.stmts) s->accept(*this);
    --depth_;
}

void ASTPrinter::visit(IfStmt& n) {
    indent(); os_ << "If\n";
    ++depth_; n.cond->accept(*this); n.then_branch->accept(*this);
    if (n.else_branch) n.else_branch->accept(*this);
    --depth_;
}

void ASTPrinter::visit(WhileStmt& n) {
    indent(); os_ << "While\n";
    ++depth_; n.cond->accept(*this); n.body->accept(*this); --depth_;
}

void ASTPrinter::visit(ForStmt& n) {
    indent(); os_ << "For\n";
    ++depth_;
    if (n.init) n.init->accept(*this);
    if (n.cond) n.cond->accept(*this);
    if (n.incr) n.incr->accept(*this);
    n.body->accept(*this);
    --depth_;
}

void ASTPrinter::visit(ReturnStmt& n) {
    indent(); os_ << "Return\n";
    if (n.value) { ++depth_; n.value->accept(*this); --depth_; }
}

void ASTPrinter::visit(FunctionDef& n) {
    indent(); os_ << "FuncDef(" << n.return_type.toString() << " " << n.name << "(";
    for (size_t i = 0; i < n.params.size(); ++i) {
        if (i) os_ << ", ";
        os_ << n.params[i].type.toString() << " " << n.params[i].name;
    }
    os_ << "))" << (n.is_declaration ? " [decl]" : "") << "\n";
    if (n.body) { ++depth_; n.body->accept(*this); --depth_; }
}

void ASTPrinter::visit(GlobalDecl& n) {
    indent(); os_ << "GlobalDecl(" << n.type.toString() << " " << n.name << ")\n";
    if (n.init) { ++depth_; n.init->accept(*this); --depth_; }
}

void ASTPrinter::visit(Program& n) {
    os_ << "Program\n";
    ++depth_;
    for (auto& d : n.decls) d->accept(*this);
    --depth_;
}

} // namespace minic
