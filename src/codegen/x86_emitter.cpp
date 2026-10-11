#include "x86_emitter.h"
#include <cassert>
#include <algorithm>

namespace minic {

const std::vector<std::string> X86Emitter::kArgRegs = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

X86Emitter::X86Emitter(const std::string& source_filename)
    : source_filename_(source_filename) {
    // Emit file-level header
    emitLine("    .file 1 \"" + source_filename_ + "\"");
    emitLine("    .text");
}

void X86Emitter::emitLine(const std::string& line) {
    output_ << line << "\n";
    ++asm_line_;
}

void X86Emitter::emitComment(const std::string& comment) {
    emitLine("    # " + comment);
}

void X86Emitter::emitLabel(const std::string& label) {
    output_ << label << ":\n";
    ++asm_line_;
}

bool X86Emitter::isConstant(const std::string& s) const {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

std::string X86Emitter::getVarLoc(const std::string& var, const RegAllocResult& alloc,
                                   const FunctionInfo& info) {
    // PRIORITY 1: Parameters always live on the stack (saved in prologue)
    auto param_it = param_index_.find(var);
    if (param_it != param_index_.end()) {
        int idx = param_it->second;
        if (idx < 6) {
            int offset = -(idx + 1) * 8;
            return std::to_string(offset) + "(%rbp)";
        } else {
            int offset = 16 + (idx - 6) * 8;
            return std::to_string(offset) + "(%rbp)";
        }
    }

    // PRIORITY 2: Use explicit stack slot map if already assigned
    auto slot_it = stack_slots_.find(var);
    if (slot_it != stack_slots_.end()) {
        return std::to_string(slot_it->second) + "(%rbp)";
    }

    // PRIORITY 3: Assign a new unique stack slot for this variable
    // Parameters take slots -(1*8) through -(6*8) = -8..-48
    // Local vars start at -56 and go downward
    int base = 56 + info.num_params * 8;  // start past param area
    int slot_offset = -(base + next_stack_slot_ * 8);
    next_stack_slot_++;
    stack_slots_[var] = slot_offset;
    return std::to_string(slot_offset) + "(%rbp)";
}

std::string X86Emitter::loadToReg(const std::string& var, const std::string& preferred_reg,
                                   const RegAllocResult& alloc, const FunctionInfo& info) {
    if (isConstant(var)) {
        emitLine("    movq    $" + var + ", " + preferred_reg);
        return preferred_reg;
    }
    if (var[0] == '.') {
        // String literal label
        emitLine("    leaq    " + var + "(%rip), " + preferred_reg);
        return preferred_reg;
    }

    std::string loc = getVarLoc(var, alloc, info);
    if (loc[0] == '%') {
        // Already in a register
        if (loc != preferred_reg) {
            emitLine("    movq    " + loc + ", " + preferred_reg);
            return preferred_reg;
        }
        return loc;
    }

    // Load from memory
    emitLine("    movq    " + loc + ", " + preferred_reg);
    return preferred_reg;
}

void X86Emitter::storeFromReg(const std::string& reg, const std::string& var,
                               const RegAllocResult& alloc, const FunctionInfo& info) {
    std::string loc = getVarLoc(var, alloc, info);
    if (loc[0] == '%') {
        if (loc != reg) {
            emitLine("    movq    " + reg + ", " + loc);
        }
    } else {
        emitLine("    movq    " + reg + ", " + loc);
    }
}

void X86Emitter::emitPrologue(const FunctionInfo& info, const RegAllocResult& /*alloc*/) {
    emitLine("    .globl  " + info.name);
    emitLine("    .type   " + info.name + ", @function");
    emitLabel(info.name);

    emitLine("    pushq   %rbp");
    emitLine("    movq    %rsp, %rbp");

    // Allocate frame: 1024 bytes covers ~120 stack-allocated variables
    // Pure stack-based codegen — no callee-saved registers needed
    emitLine("    subq    $1024, %rsp");

    // Store parameters from registers to stack
    for (int i = 0; i < info.num_params && i < 6; ++i) {
        int offset = -(i + 1) * 8;
        emitLine("    movq    " + kArgRegs[i] + ", " + std::to_string(offset) + "(%rbp)");
    }
}

void X86Emitter::emitEpilogue(const FunctionInfo& info, const RegAllocResult& /*alloc*/) {
    emitLabel(".L" + info.name + "_epilogue");

    emitLine("    movq    %rbp, %rsp");
    emitLine("    popq    %rbp");
    emitLine("    ret");
    emitLine("    .size   " + info.name + ", .-" + info.name);
}

void X86Emitter::emitInstruction(const TACInstr& instr, const RegAllocResult& alloc,
                                  const FunctionInfo& info) {
    // Emit .loc directive for DWARF
    if (instr.src_loc.line > 0) {
        emitLine("    .loc 1 " + std::to_string(instr.src_loc.line) + " " +
                 std::to_string(instr.src_loc.column));
        source_map_.record(asm_line_, instr.src_loc);
    }

    switch (instr.op) {
        case TACOp::Label:
            if (instr.result[0] == '.') {
                emitLabel(instr.result);
            }
            break;

        case TACOp::LoadImm: {
            if (instr.arg1[0] == '.') {
                // String literal address
                std::string reg = loadToReg(instr.arg1, "%rax", alloc, info);
                storeFromReg(reg, instr.result, alloc, info);
            } else {
                emitLine("    movq    $" + instr.arg1 + ", %rax");
                storeFromReg("%rax", instr.result, alloc, info);
            }
            break;
        }

        case TACOp::Assign: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Add: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    addq    %rcx, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Sub: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    subq    %rcx, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Mul: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    imulq   %rcx, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Div: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    cqto");
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    idivq   %rcx");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Mod: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    cqto");
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    idivq   %rcx");
            storeFromReg("%rdx", instr.result, alloc, info); // remainder
            break;
        }

        case TACOp::BitAnd: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    andq    %rcx, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::BitOr: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    orq     %rcx, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::BitXor: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    xorq    %rcx, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::BitNot: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    notq    %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::ShiftL: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    salq    %cl, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::ShiftR: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    sarq    %cl, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::LogNot: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    cmpq    $0, %rax");
            emitLine("    sete    %al");
            emitLine("    movzbq  %al, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Eq: case TACOp::NEq: case TACOp::Lt:
        case TACOp::Gt: case TACOp::LEq: case TACOp::GEq: {
            loadToReg(instr.arg1, "%rax", alloc, info);
            loadToReg(instr.arg2, "%rcx", alloc, info);
            emitLine("    cmpq    %rcx, %rax");
            std::string set_instr;
            switch (instr.op) {
                case TACOp::Eq:  set_instr = "sete"; break;
                case TACOp::NEq: set_instr = "setne"; break;
                case TACOp::Lt:  set_instr = "setl"; break;
                case TACOp::Gt:  set_instr = "setg"; break;
                case TACOp::LEq: set_instr = "setle"; break;
                case TACOp::GEq: set_instr = "setge"; break;
                default: set_instr = "sete";
            }
            emitLine("    " + set_instr + "    %al");
            emitLine("    movzbq  %al, %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Load: {
            // x = *y: load from address in y
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    movq    (%rax), %rax");
            storeFromReg("%rax", instr.result, alloc, info);
            break;
        }

        case TACOp::Store: {
            // *x = y: store value of y at address x
            loadToReg(instr.arg1, "%rcx", alloc, info);  // value
            loadToReg(instr.result, "%rax", alloc, info); // address
            emitLine("    movq    %rcx, (%rax)");
            break;
        }

        case TACOp::AddressOf: {
            // x = &y
            std::string loc = getVarLoc(instr.arg1, alloc, info);
            if (loc[0] == '%') {
                // Can't take address of a register — this shouldn't normally happen
                // Push it to a temp location on stack
                emitLine("    pushq   " + loc);
                emitLine("    movq    %rsp, %rax");
                storeFromReg("%rax", instr.result, alloc, info);
            } else {
                emitLine("    leaq    " + loc + ", %rax");
                storeFromReg("%rax", instr.result, alloc, info);
            }
            break;
        }

        case TACOp::Jump:
            emitLine("    jmp     " + instr.result);
            break;

        case TACOp::JumpIfZero:
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    cmpq    $0, %rax");
            emitLine("    je      " + instr.result);
            break;

        case TACOp::JumpIfNotZero:
            loadToReg(instr.arg1, "%rax", alloc, info);
            emitLine("    cmpq    $0, %rax");
            emitLine("    jne     " + instr.result);
            break;

        case TACOp::Param: {
            param_count_for_call_++;
            break;
        }

        case TACOp::Call: {
            // Collect params that were emitted before this call
            int num_args = std::stoi(instr.arg2);

            // Load args into proper registers
            // We need to find the PARAM instructions that precede this CALL
            // The caller is responsible for setting up args — we handle it here
            // by looking at the param instructions that were emitted

            // Actually, we need to emit arg loading here. Search backwards for
            // PARAM instructions.
            // This is handled in emitFunction by pre-processing

            emitLine("    xorq    %rax, %rax"); // zero rax (no float args)
            emitLine("    call    " + instr.arg1 + (instr.arg1 == "printf" ? "@PLT" : ""));

            if (!instr.result.empty()) {
                storeFromReg("%rax", instr.result, alloc, info);
            }
            param_count_for_call_ = 0;
            break;
        }

        case TACOp::Return: {
            if (!instr.arg1.empty()) {
                loadToReg(instr.arg1, "%rax", alloc, info);
            } else {
                emitLine("    xorq    %rax, %rax");
            }
            emitLine("    jmp     .L" + info.name + "_epilogue");
            break;
        }

        case TACOp::Nop:
            break;

        default:
            emitComment("unhandled: " + instr.toString());
            break;
    }
}

void X86Emitter::emitFunction(const FunctionCFG& cfg, const FunctionInfo& info,
                               const RegAllocResult& alloc) {
    param_index_.clear();
    stack_slots_.clear();
    next_stack_slot_ = 0;
    for (int i = 0; i < info.num_params; ++i) {
        if (i < static_cast<int>(info.param_names.size())) {
            param_index_[info.param_names[i]] = i;
        }
    }

    emitPrologue(info, alloc);

    // Emit instructions, handling PARAM + CALL sequences
    for (auto& bb : cfg.blocks) {
        // Collect all params before calls
        std::vector<std::string> pending_params;

        for (size_t i = 0; i < bb->instructions.size(); ++i) {
            auto& instr = bb->instructions[i];

            if (instr.op == TACOp::Param) {
                pending_params.push_back(instr.arg1);
                continue;
            }

            if (instr.op == TACOp::Call) {
                // Emit .loc
                if (instr.src_loc.line > 0) {
                    emitLine("    .loc 1 " + std::to_string(instr.src_loc.line) + " " +
                             std::to_string(instr.src_loc.column));
                }

                // Load arguments into registers
                for (int j = 0; j < static_cast<int>(pending_params.size()) && j < 6; ++j) {
                    loadToReg(pending_params[j], kArgRegs[j], alloc, info);
                }
                // Stack args (reverse order) for > 6 args
                for (int j = static_cast<int>(pending_params.size()) - 1; j >= 6; --j) {
                    loadToReg(pending_params[j], "%rax", alloc, info);
                    emitLine("    pushq   %rax");
                }

                // Align stack if needed
                int stack_args = std::max(0, static_cast<int>(pending_params.size()) - 6);
                bool needs_align = (stack_args % 2 != 0);
                if (needs_align) {
                    emitLine("    subq    $8, %rsp");
                }

                emitLine("    xorq    %rax, %rax");
                emitLine("    call    " + instr.arg1 + (instr.arg1 == "printf" ? "@PLT" : ""));

                // Clean up stack args
                if (needs_align) {
                    emitLine("    addq    $8, %rsp");
                }
                if (stack_args > 0) {
                    emitLine("    addq    $" + std::to_string(stack_args * 8) + ", %rsp");
                }

                if (!instr.result.empty()) {
                    storeFromReg("%rax", instr.result, alloc, info);
                }
                pending_params.clear();
                continue;
            }

            emitInstruction(instr, alloc, info);
        }
    }

    emitEpilogue(info, alloc);
    emitLine("");
}

void X86Emitter::emitStringLiterals(const std::vector<std::pair<std::string, std::string>>& strings) {
    if (strings.empty()) return;
    emitLine("    .section .rodata");
    for (auto& str_entry : strings) {
        auto& label = str_entry.first;
        auto& value = str_entry.second;
        emitLabel(label);
        // Escape the string for assembly
        std::string escaped;
        for (char c : value) {
            if (c == '\n') escaped += "\\n";
            else if (c == '\t') escaped += "\\t";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '"') escaped += "\\\"";
            else if (c == '%') escaped += "%"; // keep printf format specifiers
            else escaped += c;
        }
        emitLine("    .string \"" + escaped + "\"");
    }
    emitLine("    .text");
}

void X86Emitter::emitGlobals(const std::vector<std::pair<std::string, int64_t>>& globals) {
    if (globals.empty()) return;
    emitLine("    .data");
    for (auto& glob_entry : globals) {
        auto& name = glob_entry.first;
        auto& val = glob_entry.second;
        emitLine("    .globl  " + name);
        emitLine("    .align  8");
        emitLabel(name);
        emitLine("    .quad   " + std::to_string(val));
    }
    emitLine("    .text");
}

} // namespace minic
