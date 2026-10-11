#pragma once
#include "parser/ast.h"
#include <iostream>
#include <string>

namespace minic {

class ASTPrinter : public ASTVisitor {
public:
    explicit ASTPrinter(std::ostream& os = std::cout) : os_(os) {}
    void print(ASTNode& node);

    void visit(IntLiteralExpr& n) override;
    void visit(CharLiteralExpr& n) override;
    void visit(StringLiteralExpr& n) override;
    void visit(IdentExpr& n) override;
    void visit(BinaryExpr& n) override;
    void visit(UnaryExpr& n) override;
    void visit(CallExpr& n) override;
    void visit(ArraySubscriptExpr& n) override;
    void visit(AssignExpr& n) override;
    void visit(CastExpr& n) override;
    void visit(SizeofExpr& n) override;
    void visit(DeclStmt& n) override;
    void visit(ExprStmt& n) override;
    void visit(BlockStmt& n) override;
    void visit(IfStmt& n) override;
    void visit(WhileStmt& n) override;
    void visit(ForStmt& n) override;
    void visit(ReturnStmt& n) override;
    void visit(FunctionDef& n) override;
    void visit(GlobalDecl& n) override;
    void visit(Program& n) override;

private:
    void indent();
    std::ostream& os_;
    int depth_ = 0;
};

} // namespace minic
