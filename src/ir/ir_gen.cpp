#include "ir_gen.h"

namespace minic {

IRGenerator::IRGenerator() {}

std::string IRGenerator::newTemp() {
    return "_t" + std::to_string(temp_counter_++);
}

std::string IRGenerator::newLabel() {
    return ".L" + std::to_string(label_counter_++);
}

void IRGenerator::emit(TACOp op, const std::string& result,
                       const std::string& arg1, const std::string& arg2,
                       SourceLoc loc) {
    program_.emit({op, result, arg1, arg2, loc, 0});
}

TACOp IRGenerator::binOpToTACOp(BinOp op) {
    switch (op) {
        case BinOp::Add:    return TACOp::Add;
        case BinOp::Sub:    return TACOp::Sub;
        case BinOp::Mul:    return TACOp::Mul;
        case BinOp::Div:    return TACOp::Div;
        case BinOp::Mod:    return TACOp::Mod;
        case BinOp::BitAnd: return TACOp::BitAnd;
        case BinOp::BitOr:  return TACOp::BitOr;
        case BinOp::BitXor: return TACOp::BitXor;
        case BinOp::ShiftL: return TACOp::ShiftL;
        case BinOp::ShiftR: return TACOp::ShiftR;
        case BinOp::LogAnd: return TACOp::LogAnd;
        case BinOp::LogOr:  return TACOp::LogOr;
        case BinOp::Eq:     return TACOp::Eq;
        case BinOp::NEq:    return TACOp::NEq;
        case BinOp::Lt:     return TACOp::Lt;
        case BinOp::Gt:     return TACOp::Gt;
        case BinOp::LEq:    return TACOp::LEq;
        case BinOp::GEq:    return TACOp::GEq;
    }
    return TACOp::Add;
}

TACProgram IRGenerator::generate(Program& program) {
    program.accept(*this);
    return std::move(program_);
}

void IRGenerator::visit(Program& n) {
    for (auto& d : n.decls) d->accept(*this);
}

void IRGenerator::visit(IntLiteralExpr& n) {
    std::string t = newTemp();
    emit(TACOp::LoadImm, t, std::to_string(n.value), "", n.loc);
    last_result_ = t;
}

void IRGenerator::visit(CharLiteralExpr& n) {
    std::string t = newTemp();
    emit(TACOp::LoadImm, t, std::to_string(static_cast<int>(n.value)), "", n.loc);
    last_result_ = t;
}

void IRGenerator::visit(StringLiteralExpr& n) {
    std::string label = ".Lstr" + std::to_string(program_.string_counter++);
    program_.string_literals.push_back({label, n.value});
    std::string t = newTemp();
    emit(TACOp::LoadImm, t, label, "", n.loc);
    last_result_ = t;
}

void IRGenerator::visit(IdentExpr& n) {
    last_result_ = lookupVar(n.name);
}

void IRGenerator::visit(BinaryExpr& n) {
    // Short-circuit for logical operators
    if (n.op == BinOp::LogAnd) {
        std::string result = newTemp();
        std::string lfalse = newLabel();
        std::string lend = newLabel();

        n.lhs->accept(*this);
        emit(TACOp::JumpIfZero, lfalse, last_result_, "", n.loc);
        n.rhs->accept(*this);
        emit(TACOp::JumpIfZero, lfalse, last_result_, "", n.loc);
        emit(TACOp::LoadImm, result, "1", "", n.loc);
        emit(TACOp::Jump, lend, "", "", n.loc);
        emit(TACOp::Label, lfalse, "", "", n.loc);
        emit(TACOp::LoadImm, result, "0", "", n.loc);
        emit(TACOp::Label, lend, "", "", n.loc);
        last_result_ = result;
        return;
    }
    if (n.op == BinOp::LogOr) {
        std::string result = newTemp();
        std::string ltrue = newLabel();
        std::string lend = newLabel();

        n.lhs->accept(*this);
        emit(TACOp::JumpIfNotZero, ltrue, last_result_, "", n.loc);
        n.rhs->accept(*this);
        emit(TACOp::JumpIfNotZero, ltrue, last_result_, "", n.loc);
        emit(TACOp::LoadImm, result, "0", "", n.loc);
        emit(TACOp::Jump, lend, "", "", n.loc);
        emit(TACOp::Label, ltrue, "", "", n.loc);
        emit(TACOp::LoadImm, result, "1", "", n.loc);
        emit(TACOp::Label, lend, "", "", n.loc);
        last_result_ = result;
        return;
    }

    // Handle pointer arithmetic scaling
    if ((n.op == BinOp::Add || n.op == BinOp::Sub) && n.lhs->resolved_type.isPointer()) {
        n.lhs->accept(*this);
        std::string lhs = last_result_;
        n.rhs->accept(*this);
        std::string rhs = last_result_;

        int scale = n.lhs->resolved_type.pointeeSize();
        if (scale > 1) {
            std::string scaled = newTemp();
            std::string scale_t = newTemp();
            emit(TACOp::LoadImm, scale_t, std::to_string(scale), "", n.loc);
            emit(TACOp::Mul, scaled, rhs, scale_t, n.loc);
            rhs = scaled;
        }
        std::string result = newTemp();
        emit(binOpToTACOp(n.op), result, lhs, rhs, n.loc);
        last_result_ = result;
        return;
    }

    n.lhs->accept(*this);
    std::string lhs = last_result_;
    n.rhs->accept(*this);
    std::string rhs = last_result_;

    std::string result = newTemp();
    emit(binOpToTACOp(n.op), result, lhs, rhs, n.loc);
    last_result_ = result;
}

void IRGenerator::visit(UnaryExpr& n) {
    n.operand->accept(*this);
    std::string operand = last_result_;

    switch (n.op) {
        case UnaryOp::Neg: {
            std::string t = newTemp();
            std::string zero = newTemp();
            emit(TACOp::LoadImm, zero, "0", "", n.loc);
            emit(TACOp::Sub, t, zero, operand, n.loc);
            last_result_ = t;
            break;
        }
        case UnaryOp::LogNot: {
            std::string t = newTemp();
            emit(TACOp::LogNot, t, operand, "", n.loc);
            last_result_ = t;
            break;
        }
        case UnaryOp::BitNot: {
            std::string t = newTemp();
            emit(TACOp::BitNot, t, operand, "", n.loc);
            last_result_ = t;
            break;
        }
        case UnaryOp::Deref: {
            std::string t = newTemp();
            emit(TACOp::Load, t, operand, "", n.loc);
            last_result_ = t;
            break;
        }
        case UnaryOp::AddrOf: {
            std::string t = newTemp();
            emit(TACOp::AddressOf, t, operand, "", n.loc);
            last_result_ = t;
            break;
        }
    }
}

void IRGenerator::visit(CallExpr& n) {
    // Evaluate arguments and emit PARAM instructions
    std::vector<std::string> arg_temps;
    for (auto& a : n.args) {
        a->accept(*this);
        arg_temps.push_back(last_result_);
    }
    for (auto& at : arg_temps) {
        emit(TACOp::Param, "", at, "", n.loc);
    }
    std::string result = newTemp();
    emit(TACOp::Call, result, n.callee, std::to_string(n.args.size()), n.loc);
    last_result_ = result;
}

void IRGenerator::visit(ArraySubscriptExpr& n) {
    n.array->accept(*this);
    std::string arr = last_result_;
    n.index->accept(*this);
    std::string idx = last_result_;

    // Scale index by element size
    int scale = n.array->resolved_type.pointeeSize();
    if (scale > 1) {
        std::string scaled = newTemp();
        std::string scale_t = newTemp();
        emit(TACOp::LoadImm, scale_t, std::to_string(scale), "", n.loc);
        emit(TACOp::Mul, scaled, idx, scale_t, n.loc);
        idx = scaled;
    }

    std::string addr = newTemp();
    emit(TACOp::Add, addr, arr, idx, n.loc);
    std::string result = newTemp();
    emit(TACOp::Load, result, addr, "", n.loc);
    last_result_ = result;
}

void IRGenerator::visit(AssignExpr& n) {
    n.value->accept(*this);
    std::string val = last_result_;

    // Handle compound assignment
    if (n.op != AssignOp::Assign) {
        // First, get current value of target
        n.target->accept(*this);
        std::string cur = last_result_;
        std::string temp = newTemp();
        TACOp tac_op;
        switch (n.op) {
            case AssignOp::AddAssign: tac_op = TACOp::Add; break;
            case AssignOp::SubAssign: tac_op = TACOp::Sub; break;
            case AssignOp::MulAssign: tac_op = TACOp::Mul; break;
            case AssignOp::DivAssign: tac_op = TACOp::Div; break;
            default: tac_op = TACOp::Add;
        }
        emit(tac_op, temp, cur, val, n.loc);
        val = temp;
    }

    // Check if target is a dereference (*ptr = val -> STORE)
    if (auto* deref = dynamic_cast<UnaryExpr*>(n.target.get())) {
        if (deref->op == UnaryOp::Deref) {
            deref->operand->accept(*this);
            std::string ptr = last_result_;
            emit(TACOp::Store, ptr, val, "", n.loc);
            last_result_ = val;
            return;
        }
    }

    // Check if target is array subscript (arr[i] = val)
    if (auto* sub = dynamic_cast<ArraySubscriptExpr*>(n.target.get())) {
        sub->array->accept(*this);
        std::string arr = last_result_;
        sub->index->accept(*this);
        std::string idx = last_result_;
        int scale = sub->array->resolved_type.pointeeSize();
        if (scale > 1) {
            std::string scaled = newTemp();
            std::string scale_t = newTemp();
            emit(TACOp::LoadImm, scale_t, std::to_string(scale), "", n.loc);
            emit(TACOp::Mul, scaled, idx, scale_t, n.loc);
            idx = scaled;
        }
        std::string addr = newTemp();
        emit(TACOp::Add, addr, arr, idx, n.loc);
        emit(TACOp::Store, addr, val, "", n.loc);
        last_result_ = val;
        return;
    }

    // Simple variable assignment
    if (auto* ident = dynamic_cast<IdentExpr*>(n.target.get())) {
        std::string target_name = lookupVar(ident->name);
        emit(TACOp::Assign, target_name, val, "", n.loc);
        last_result_ = target_name;
        return;
    }

    // fallback
    n.target->accept(*this);
    std::string target = last_result_;
    emit(TACOp::Assign, target, val, "", n.loc);
    last_result_ = target;
}

void IRGenerator::visit(CastExpr& n) {
    n.operand->accept(*this);
    // Type casts are mostly no-ops at the IR level for our subset
}

void IRGenerator::visit(SizeofExpr& n) {
    std::string t = newTemp();
    emit(TACOp::LoadImm, t, std::to_string(n.query_type.size()), "", n.loc);
    last_result_ = t;
}

void IRGenerator::visit(DeclStmt& n) {
    std::string ir_name = renameVar(n.name);
    if (n.is_array) {
        if (n.init) {
            n.init->accept(*this);
            emit(TACOp::Assign, ir_name, last_result_, "", n.loc);
        }
    } else if (n.init) {
        n.init->accept(*this);
        emit(TACOp::Assign, ir_name, last_result_, "", n.loc);
    }
}

void IRGenerator::visit(ExprStmt& n) {
    if (n.expr) n.expr->accept(*this);
}

void IRGenerator::visit(BlockStmt& n) {
    pushScope();
    for (auto& s : n.stmts) s->accept(*this);
    popScope();
}

void IRGenerator::visit(IfStmt& n) {
    std::string lelse = newLabel();
    std::string lend = newLabel();

    n.cond->accept(*this);
    emit(TACOp::JumpIfZero, n.else_branch ? lelse : lend, last_result_, "", n.loc);

    n.then_branch->accept(*this);

    if (n.else_branch) {
        emit(TACOp::Jump, lend, "", "", n.loc);
        emit(TACOp::Label, lelse, "", "", n.loc);
        n.else_branch->accept(*this);
    }

    emit(TACOp::Label, lend, "", "", n.loc);
}

void IRGenerator::visit(WhileStmt& n) {
    std::string lstart = newLabel();
    std::string lend = newLabel();

    emit(TACOp::Label, lstart, "", "", n.loc);
    n.cond->accept(*this);
    emit(TACOp::JumpIfZero, lend, last_result_, "", n.loc);
    n.body->accept(*this);
    emit(TACOp::Jump, lstart, "", "", n.loc);
    emit(TACOp::Label, lend, "", "", n.loc);
}

void IRGenerator::visit(ForStmt& n) {
    std::string lstart = newLabel();
    std::string lend = newLabel();

    if (n.init) n.init->accept(*this);
    emit(TACOp::Label, lstart, "", "", n.loc);
    if (n.cond) {
        n.cond->accept(*this);
        emit(TACOp::JumpIfZero, lend, last_result_, "", n.loc);
    }
    n.body->accept(*this);
    if (n.incr) n.incr->accept(*this);
    emit(TACOp::Jump, lstart, "", "", n.loc);
    emit(TACOp::Label, lend, "", "", n.loc);
}

void IRGenerator::visit(ReturnStmt& n) {
    if (n.value) {
        n.value->accept(*this);
        emit(TACOp::Return, "", last_result_, "", n.loc);
    } else {
        emit(TACOp::Return, "", "", "", n.loc);
    }
}

void IRGenerator::visit(FunctionDef& n) {
    if (n.is_declaration) return;

    current_func_ = n.name;
    scope_stack_.clear();
    pushScope();
    emit(TACOp::Label, n.name, "", "", n.loc);

    // Register parameters in scope and store renamed names
    std::vector<std::string> renamed_params;
    for (auto& p : n.params) {
        renamed_params.push_back(renameVar(p.name));
    }
    program_.func_param_names[n.name] = renamed_params;

    if (n.body) {
        for (auto& s : n.body->stmts) s->accept(*this);
    }

    if (n.return_type.isVoid()) {
        emit(TACOp::Return, "", "", "", n.loc);
    }
    popScope();
}

void IRGenerator::visit(GlobalDecl& n) {
    // Global declarations are handled separately at codegen (in .data/.bss)
}

} // namespace minic
