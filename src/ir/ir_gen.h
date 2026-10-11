#pragma once
#include "tac.h"
#include "parser/ast.h"
#include "sema/symbol_table.h"
#include <string>
#include <unordered_map>

namespace minic {

class IRGenerator : public ASTVisitor {
public:
    IRGenerator();
    TACProgram generate(Program& program);

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
    std::string newTemp();
    std::string newLabel();
    void emit(TACOp op, const std::string& result = "",
              const std::string& arg1 = "", const std::string& arg2 = "",
              SourceLoc loc = {});

    TACOp binOpToTACOp(BinOp op);

    TACProgram program_;
    int temp_counter_ = 0;
    int label_counter_ = 0;
    std::string last_result_;
    std::string current_func_;

    // Scope-aware variable renaming
    int scope_id_counter_ = 0;
    std::vector<std::unordered_map<std::string, std::string>> scope_stack_;

    void pushScope() { scope_stack_.push_back({}); }
    void popScope() { if (!scope_stack_.empty()) scope_stack_.pop_back(); }
    std::string renameVar(const std::string& name) {
        std::string renamed = name + "." + std::to_string(scope_id_counter_++);
        if (!scope_stack_.empty()) scope_stack_.back()[name] = renamed;
        return renamed;
    }
    std::string lookupVar(const std::string& name) {
        for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return name;
    }
};

} // namespace minic
