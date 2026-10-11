#include "constant_folding.h"
#include <unordered_map>

namespace minic {

static bool isConstant(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

static int64_t toConst(const std::string& s) {
    return std::stoll(s);
}

bool ConstantFolding::run(FunctionCFG& cfg) {
    bool changed = false;

    for (auto& bb : cfg.blocks) {
        std::unordered_map<std::string, int64_t> constants;

        for (auto& instr : bb->instructions) {
            // Track constants
            if (instr.op == TACOp::LoadImm && isConstant(instr.arg1)) {
                constants[instr.result] = toConst(instr.arg1);
            }

            // Fold binary operations
            if (instr.op >= TACOp::Add && instr.op <= TACOp::GEq) {
                bool a1_const = isConstant(instr.arg1) || constants.count(instr.arg1);
                bool a2_const = isConstant(instr.arg2) || constants.count(instr.arg2);

                if (a1_const && a2_const) {
                    int64_t v1 = isConstant(instr.arg1) ? toConst(instr.arg1) : constants[instr.arg1];
                    int64_t v2 = isConstant(instr.arg2) ? toConst(instr.arg2) : constants[instr.arg2];
                    int64_t result = 0;
                    bool folded = true;

                    switch (instr.op) {
                        case TACOp::Add: result = v1 + v2; break;
                        case TACOp::Sub: result = v1 - v2; break;
                        case TACOp::Mul: result = v1 * v2; break;
                        case TACOp::Div:
                            if (v2 == 0) { folded = false; break; }
                            result = v1 / v2; break;
                        case TACOp::Mod:
                            if (v2 == 0) { folded = false; break; }
                            result = v1 % v2; break;
                        case TACOp::BitAnd: result = v1 & v2; break;
                        case TACOp::BitOr:  result = v1 | v2; break;
                        case TACOp::BitXor: result = v1 ^ v2; break;
                        case TACOp::ShiftL: result = v1 << v2; break;
                        case TACOp::ShiftR: result = v1 >> v2; break;
                        case TACOp::Eq:  result = (v1 == v2) ? 1 : 0; break;
                        case TACOp::NEq: result = (v1 != v2) ? 1 : 0; break;
                        case TACOp::Lt:  result = (v1 < v2)  ? 1 : 0; break;
                        case TACOp::Gt:  result = (v1 > v2)  ? 1 : 0; break;
                        case TACOp::LEq: result = (v1 <= v2) ? 1 : 0; break;
                        case TACOp::GEq: result = (v1 >= v2) ? 1 : 0; break;
                        default: folded = false;
                    }

                    if (folded) {
                        instr.op = TACOp::LoadImm;
                        instr.arg1 = std::to_string(result);
                        instr.arg2.clear();
                        constants[instr.result] = result;
                        changed = true;
                    }
                }
            }

            // Fold unary operations
            if (instr.op == TACOp::LogNot || instr.op == TACOp::BitNot) {
                bool a1_const = isConstant(instr.arg1) || constants.count(instr.arg1);
                if (a1_const) {
                    int64_t v = isConstant(instr.arg1) ? toConst(instr.arg1) : constants[instr.arg1];
                    int64_t result = (instr.op == TACOp::LogNot) ? (v == 0 ? 1 : 0) : ~v;
                    instr.op = TACOp::LoadImm;
                    instr.arg1 = std::to_string(result);
                    instr.arg2.clear();
                    constants[instr.result] = result;
                    changed = true;
                }
            }

            // Any non-constant assignment kills the constant
            if (instr.op == TACOp::Assign || instr.op == TACOp::Call ||
                instr.op == TACOp::Load) {
                constants.erase(instr.result);
            }
        }
    }

    return changed;
}

} // namespace minic
