#include "tac.h"

namespace minic {

const char* tacOpToString(TACOp op) {
    switch (op) {
        case TACOp::Add: return "ADD"; case TACOp::Sub: return "SUB";
        case TACOp::Mul: return "MUL"; case TACOp::Div: return "DIV";
        case TACOp::Mod: return "MOD";
        case TACOp::BitAnd: return "BAND"; case TACOp::BitOr: return "BOR";
        case TACOp::BitXor: return "BXOR"; case TACOp::BitNot: return "BNOT";
        case TACOp::ShiftL: return "SHL"; case TACOp::ShiftR: return "SHR";
        case TACOp::Eq: return "EQ"; case TACOp::NEq: return "NEQ";
        case TACOp::Lt: return "LT"; case TACOp::Gt: return "GT";
        case TACOp::LEq: return "LEQ"; case TACOp::GEq: return "GEQ";
        case TACOp::LogAnd: return "LAND"; case TACOp::LogOr: return "LOR";
        case TACOp::LogNot: return "LNOT";
        case TACOp::Assign: return "ASSIGN"; case TACOp::LoadImm: return "LOADI";
        case TACOp::Load: return "LOAD"; case TACOp::Store: return "STORE";
        case TACOp::AddressOf: return "ADDROF";
        case TACOp::Label: return "LABEL"; case TACOp::Jump: return "JMP";
        case TACOp::JumpIfZero: return "JZ"; case TACOp::JumpIfNotZero: return "JNZ";
        case TACOp::Call: return "CALL"; case TACOp::Param: return "PARAM";
        case TACOp::Return: return "RET"; case TACOp::Nop: return "NOP";
    }
    return "??";
}

std::string TACInstr::toString() const {
    std::string s;
    if (op == TACOp::Label) {
        return result + ":";
    }
    s = "  ";
    if (!result.empty() && op != TACOp::Jump && op != TACOp::JumpIfZero &&
        op != TACOp::JumpIfNotZero && op != TACOp::Store && op != TACOp::Param &&
        op != TACOp::Return && op != TACOp::Nop) {
        s += result + " = ";
    }
    s += tacOpToString(op);
    if (!arg1.empty()) s += " " + arg1;
    if (!arg2.empty()) s += ", " + arg2;
    if (op == TACOp::Jump || op == TACOp::JumpIfZero || op == TACOp::JumpIfNotZero) {
        s += " " + result;
        if (!arg1.empty() && op != TACOp::Jump) s = "  " + std::string(tacOpToString(op)) + " " + arg1 + ", " + result;
    }
    return s;
}

void TACProgram::emit(TACInstr instr) {
    instr.id = nextInstrId();
    instructions.push_back(std::move(instr));
}

void TACProgram::print(std::ostream& os) const {
    for (auto& instr : instructions) {
        os << instr.toString() << "\n";
    }
}

} // namespace minic
