#include "sema.h"

namespace minic {

SemanticAnalyzer::SemanticAnalyzer(DiagnosticEngine& diag) : diag_(diag) {}

void SemanticAnalyzer::analyze(Program& program) {
    symtab_.pushScope(); // global scope

    // Register printf as a built-in
    Symbol printf_sym;
    printf_sym.name = "printf";
    printf_sym.is_function = true;
    printf_sym.is_global = true;
    printf_sym.is_defined = true;
    printf_sym.type = {TypeInfo::Int, 0};
    symtab_.declare(printf_sym);

    program.accept(*this);
}

void SemanticAnalyzer::visit(Program& n) {
    for (auto& d : n.decls) d->accept(*this);
}

void SemanticAnalyzer::visit(IntLiteralExpr& n) {
    n.resolved_type = {TypeInfo::Int, 0};
}

void SemanticAnalyzer::visit(CharLiteralExpr& n) {
    n.resolved_type = {TypeInfo::Char, 0};
}

void SemanticAnalyzer::visit(StringLiteralExpr& n) {
    n.resolved_type = {TypeInfo::Char, 1}; // char*
}

void SemanticAnalyzer::visit(IdentExpr& n) {
    Symbol* sym = symtab_.lookup(n.name);
    if (!sym) {
        diag_.error(n.loc, "undeclared variable '" + n.name + "'");
        n.resolved_type = {TypeInfo::Int, 0};
        return;
    }
    n.resolved_type = sym->type;
    if (sym->is_array) {
        // Array decays to pointer
        n.resolved_type = sym->type.addr();
    }
}

void SemanticAnalyzer::visit(BinaryExpr& n) {
    n.lhs->accept(*this);
    n.rhs->accept(*this);
    TypeInfo lt = n.lhs->resolved_type;
    TypeInfo rt = n.rhs->resolved_type;

    // Pointer arithmetic: ptr + int or int + ptr
    if (n.op == BinOp::Add || n.op == BinOp::Sub) {
        if (lt.isPointer() && rt.isIntegral()) {
            n.resolved_type = lt;
            return;
        }
        if (rt.isPointer() && lt.isIntegral() && n.op == BinOp::Add) {
            n.resolved_type = rt;
            return;
        }
        if (lt.isPointer() && rt.isPointer() && n.op == BinOp::Sub) {
            n.resolved_type = {TypeInfo::Int, 0}; // pointer difference
            return;
        }
    }

    // Comparison operators always yield int
    if (n.op == BinOp::Eq || n.op == BinOp::NEq || n.op == BinOp::Lt ||
        n.op == BinOp::Gt || n.op == BinOp::LEq || n.op == BinOp::GEq) {
        n.resolved_type = {TypeInfo::Int, 0};
        return;
    }

    // Logical operators yield int
    if (n.op == BinOp::LogAnd || n.op == BinOp::LogOr) {
        n.resolved_type = {TypeInfo::Int, 0};
        return;
    }

    // Arithmetic: both operands should be integral
    if (!lt.isIntegral() || !rt.isIntegral()) {
        if (!lt.isPointer() && !rt.isPointer()) {
            diag_.error(n.loc, "invalid operands to binary '" +
                        std::string(binOpToString(n.op)) + "' (" +
                        lt.toString() + " and " + rt.toString() + ")");
        }
    }
    // Promote char to int
    n.resolved_type = {TypeInfo::Int, 0};
}

void SemanticAnalyzer::visit(UnaryExpr& n) {
    n.operand->accept(*this);
    TypeInfo ot = n.operand->resolved_type;

    switch (n.op) {
        case UnaryOp::Neg:
        case UnaryOp::BitNot:
            if (!ot.isIntegral()) {
                diag_.error(n.loc, "unary '" + std::string(unaryOpToString(n.op)) +
                            "' requires integral type");
            }
            n.resolved_type = {TypeInfo::Int, 0};
            break;
        case UnaryOp::LogNot:
            n.resolved_type = {TypeInfo::Int, 0};
            break;
        case UnaryOp::Deref:
            if (!ot.isPointer()) {
                diag_.error(n.loc, "cannot dereference non-pointer type '" + ot.toString() + "'");
                n.resolved_type = {TypeInfo::Int, 0};
            } else {
                n.resolved_type = ot.deref();
            }
            break;
        case UnaryOp::AddrOf:
            // Check that operand is an lvalue
            if (!dynamic_cast<IdentExpr*>(n.operand.get()) &&
                !dynamic_cast<ArraySubscriptExpr*>(n.operand.get()) &&
                !dynamic_cast<UnaryExpr*>(n.operand.get())) {
                diag_.error(n.loc, "cannot take address of non-lvalue");
            }
            n.resolved_type = ot.addr();
            break;
    }
}

void SemanticAnalyzer::visit(CallExpr& n) {
    // Special handling for printf (variadic)
    if (n.callee == "printf") {
        for (auto& a : n.args) a->accept(*this);
        n.resolved_type = {TypeInfo::Int, 0};
        return;
    }

    Symbol* sym = symtab_.lookup(n.callee);
    if (!sym) {
        diag_.error(n.loc, "undeclared function '" + n.callee + "'");
        symtab_.addExternalRef(n.callee);
        n.resolved_type = {TypeInfo::Int, 0};
        for (auto& a : n.args) a->accept(*this);
        return;
    }
    if (!sym->is_function) {
        diag_.error(n.loc, "'" + n.callee + "' is not a function");
    }

    // Check argument count
    if (!sym->param_types.empty() && n.args.size() != sym->param_types.size()) {
        diag_.error(n.loc, "function '" + n.callee + "' expects " +
                    std::to_string(sym->param_types.size()) + " arguments, got " +
                    std::to_string(n.args.size()));
    }

    for (size_t i = 0; i < n.args.size(); ++i) {
        n.args[i]->accept(*this);
        if (i < sym->param_types.size()) {
            TypeInfo expected = sym->param_types[i];
            TypeInfo actual = n.args[i]->resolved_type;
            if (!expected.isCompatibleWith(actual)) {
                diag_.error(n.args[i]->loc, "argument type mismatch: expected '" +
                            expected.toString() + "', got '" + actual.toString() + "'");
            }
        }
    }

    n.resolved_type = sym->type;
}

void SemanticAnalyzer::visit(ArraySubscriptExpr& n) {
    n.array->accept(*this);
    n.index->accept(*this);
    TypeInfo at = n.array->resolved_type;
    if (!at.isPointer()) {
        diag_.error(n.loc, "subscript requires pointer or array type, got '" + at.toString() + "'");
        n.resolved_type = {TypeInfo::Int, 0};
    } else {
        n.resolved_type = at.deref();
    }
    if (!n.index->resolved_type.isIntegral()) {
        diag_.error(n.index->loc, "array index must be integral type");
    }
}

void SemanticAnalyzer::visit(AssignExpr& n) {
    n.target->accept(*this);
    n.value->accept(*this);

    // Check lvalue
    if (!dynamic_cast<IdentExpr*>(n.target.get()) &&
        !dynamic_cast<ArraySubscriptExpr*>(n.target.get()) &&
        !dynamic_cast<UnaryExpr*>(n.target.get())) {
        diag_.error(n.loc, "invalid assignment target (not an lvalue)");
    }

    TypeInfo lt = n.target->resolved_type;
    TypeInfo rt = n.value->resolved_type;

    if (!lt.isCompatibleWith(rt)) {
        diag_.error(n.loc, "type mismatch in assignment: '" + lt.toString() +
                    "' = '" + rt.toString() + "'");
    }
    n.resolved_type = lt;
}

void SemanticAnalyzer::visit(CastExpr& n) {
    n.operand->accept(*this);
    n.resolved_type = n.target_type;
}

void SemanticAnalyzer::visit(SizeofExpr& n) {
    n.resolved_type = {TypeInfo::Int, 0};
}

void SemanticAnalyzer::visit(DeclStmt& n) {
    if (n.init) {
        n.init->accept(*this);
        if (!n.type.isCompatibleWith(n.init->resolved_type)) {
            diag_.error(n.loc, "type mismatch in declaration of '" + n.name +
                        "': '" + n.type.toString() + "' vs '" +
                        n.init->resolved_type.toString() + "'");
        }
    }
    Symbol sym;
    sym.name = n.name;
    sym.type = n.type;
    sym.is_array = n.is_array;
    sym.array_size = n.array_size;
    sym.decl_loc = n.loc;
    sym.is_global = (symtab_.currentDepth() == 0);
    if (!symtab_.declare(sym)) {
        diag_.error(n.loc, "redeclaration of '" + n.name + "'");
    }
}

void SemanticAnalyzer::visit(ExprStmt& n) {
    if (n.expr) n.expr->accept(*this);
}

void SemanticAnalyzer::visit(BlockStmt& n) {
    symtab_.pushScope();
    for (auto& s : n.stmts) s->accept(*this);
    symtab_.popScope();
}

void SemanticAnalyzer::visit(IfStmt& n) {
    n.cond->accept(*this);
    n.then_branch->accept(*this);
    if (n.else_branch) n.else_branch->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt& n) {
    n.cond->accept(*this);
    n.body->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt& n) {
    symtab_.pushScope();
    if (n.init) n.init->accept(*this);
    if (n.cond) n.cond->accept(*this);
    if (n.incr) n.incr->accept(*this);
    n.body->accept(*this);
    symtab_.popScope();
}

void SemanticAnalyzer::visit(ReturnStmt& n) {
    if (n.value) {
        n.value->accept(*this);
        if (!current_return_type_.isCompatibleWith(n.value->resolved_type) &&
            !current_return_type_.isVoid()) {
            diag_.error(n.loc, "return type mismatch: expected '" +
                        current_return_type_.toString() + "', got '" +
                        n.value->resolved_type.toString() + "'");
        }
    } else if (!current_return_type_.isVoid()) {
        diag_.warning(n.loc, "non-void function should return a value");
    }
}

void SemanticAnalyzer::visit(FunctionDef& n) {
    // Declare the function in the current (global) scope
    Symbol sym;
    sym.name = n.name;
    sym.type = n.return_type;
    sym.is_function = true;
    sym.is_global = true;
    sym.is_defined = !n.is_declaration;
    sym.decl_loc = n.loc;
    for (auto& p : n.params) sym.param_types.push_back(p.type);

    // Check for redeclaration vs redefinition
    Symbol* existing = symtab_.lookup(n.name);
    if (existing) {
        if (existing->is_function && !n.is_declaration && existing->is_defined) {
            diag_.error(n.loc, "redefinition of function '" + n.name + "'");
        }
        if (existing->is_function && !n.is_declaration) {
            existing->is_defined = true;
        }
    } else {
        symtab_.declare(sym);
    }

    if (n.is_declaration) return;

    // Process body
    current_return_type_ = n.return_type;
    current_function_ = n.name;
    symtab_.pushScope();

    // Declare parameters
    for (auto& p : n.params) {
        Symbol psym;
        psym.name = p.name;
        psym.type = p.type;
        psym.decl_loc = p.loc;
        if (!symtab_.declare(psym)) {
            diag_.error(p.loc, "duplicate parameter name '" + p.name + "'");
        }
    }

    if (n.body) {
        // Visit body statements directly (don't call block which pushes another scope)
        for (auto& s : n.body->stmts) s->accept(*this);
    }

    symtab_.popScope();
}

void SemanticAnalyzer::visit(GlobalDecl& n) {
    if (n.init) {
        n.init->accept(*this);
    }
    Symbol sym;
    sym.name = n.name;
    sym.type = n.type;
    sym.is_global = true;
    sym.is_array = n.is_array;
    sym.array_size = n.array_size;
    sym.decl_loc = n.loc;
    if (!symtab_.declare(sym)) {
        diag_.error(n.loc, "redeclaration of global '" + n.name + "'");
    }
}

} // namespace minic
