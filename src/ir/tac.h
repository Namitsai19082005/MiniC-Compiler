#pragma once
#include "common/source_location.h"
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

namespace minic {

enum class TACOp {
    // Arithmetic
    Add, Sub, Mul, Div, Mod,
    // Bitwise
    BitAnd, BitOr, BitXor, BitNot, ShiftL, ShiftR,
    // Comparison
    Eq, NEq, Lt, Gt, LEq, GEq,
    // Logical
    LogAnd, LogOr, LogNot,
    // Data movement
    Assign,     // x = y
    LoadImm,    // x = #literal
    // Pointer ops
    Load,       // x = *y
    Store,      // *x = y
    AddressOf,  // x = &y
    // Control flow
    Label,
    Jump,
    JumpIfZero,
    JumpIfNotZero,
    // Functions
    Call,       // x = call f, n
    Param,      // param x
    Return,
    // Special
    Nop
};

const char* tacOpToString(TACOp op);

struct TACInstr {
    TACOp op;
    std::string result;
    std::string arg1;
    std::string arg2;       // second operand, label target, or immediate
    SourceLoc src_loc;
    int id = 0;

    std::string toString() const;
};

class TACProgram {
public:
    std::vector<TACInstr> instructions;
    int string_counter = 0;
    std::vector<std::pair<std::string, std::string>> string_literals; // label → value
    // Per-function renamed parameter names (func_name -> [renamed_param_names])
    std::unordered_map<std::string, std::vector<std::string>> func_param_names;

    int nextInstrId() { return static_cast<int>(instructions.size()); }
    void emit(TACInstr instr);
    void print(std::ostream& os) const;

    // Get instruction count for optimization stats
    int instructionCount() const { return static_cast<int>(instructions.size()); }
};

} // namespace minic
