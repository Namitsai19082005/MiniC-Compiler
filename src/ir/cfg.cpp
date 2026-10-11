#include "cfg.h"
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <queue>

namespace minic {

bool BasicBlock::terminated() const {
    if (instructions.empty()) return false;
    auto& last = instructions.back();
    return last.op == TACOp::Jump || last.op == TACOp::Return;
}

void BasicBlock::print(std::ostream& os) const {
    os << "  BB" << id;
    if (!label.empty()) os << " (" << label << ")";
    os << ":\n";
    for (auto& i : instructions) os << "    " << i.toString() << "\n";
    os << "    preds: [";
    for (size_t i = 0; i < predecessors.size(); ++i) {
        if (i) os << ", ";
        os << "BB" << predecessors[i]->id;
    }
    os << "]  succs: [";
    for (size_t i = 0; i < successors.size(); ++i) {
        if (i) os << ", ";
        os << "BB" << successors[i]->id;
    }
    os << "]\n";
}

// ─── FunctionCFG ─────────────────────────────────────────────────────

void FunctionCFG::buildFromTAC(const std::vector<TACInstr>& instrs, size_t start, size_t end) {
    if (start >= end) return;

    // Step 1: Identify leaders
    std::set<size_t> leaders;
    leaders.insert(start);

    for (size_t i = start; i < end; ++i) {
        auto& instr = instrs[i];
        if (instr.op == TACOp::Jump || instr.op == TACOp::JumpIfZero ||
            instr.op == TACOp::JumpIfNotZero) {
            // Next instruction is a leader
            if (i + 1 < end) leaders.insert(i + 1);
        }
        if (instr.op == TACOp::Label && i > start) {
            leaders.insert(i);
        }
    }

    // Step 2: Create basic blocks
    std::unordered_map<std::string, BasicBlock*> label_to_block;
    int block_id = 0;

    std::vector<size_t> sorted_leaders(leaders.begin(), leaders.end());
    std::sort(sorted_leaders.begin(), sorted_leaders.end());

    for (size_t li = 0; li < sorted_leaders.size(); ++li) {
        size_t block_start = sorted_leaders[li];
        size_t block_end = (li + 1 < sorted_leaders.size()) ? sorted_leaders[li + 1] : end;

        auto bb = std::make_unique<BasicBlock>();
        bb->id = block_id++;

        for (size_t j = block_start; j < block_end; ++j) {
            bb->instructions.push_back(instrs[j]);
            if (instrs[j].op == TACOp::Label) {
                bb->label = instrs[j].result;
                label_to_block[instrs[j].result] = bb.get();
            }
        }

        blocks.push_back(std::move(bb));
    }

    if (!blocks.empty()) entry = blocks[0].get();

    // Step 3: Build edges
    for (size_t i = 0; i < blocks.size(); ++i) {
        auto& bb = blocks[i];
        if (bb->instructions.empty()) continue;
        auto& last = bb->instructions.back();

        if (last.op == TACOp::Jump) {
            auto it = label_to_block.find(last.result);
            if (it != label_to_block.end()) {
                bb->successors.push_back(it->second);
                it->second->predecessors.push_back(bb.get());
            }
        } else if (last.op == TACOp::JumpIfZero || last.op == TACOp::JumpIfNotZero) {
            // Target branch
            auto it = label_to_block.find(last.result);
            if (it != label_to_block.end()) {
                bb->successors.push_back(it->second);
                it->second->predecessors.push_back(bb.get());
            }
            // Fall-through
            if (i + 1 < blocks.size()) {
                bb->successors.push_back(blocks[i + 1].get());
                blocks[i + 1]->predecessors.push_back(bb.get());
            }
        } else if (last.op != TACOp::Return) {
            // Fall-through
            if (i + 1 < blocks.size()) {
                bb->successors.push_back(blocks[i + 1].get());
                blocks[i + 1]->predecessors.push_back(bb.get());
            }
        }
    }
}

// Helper: get variables used/defined by an instruction
static void getUseDef(const TACInstr& instr, std::set<std::string>& uses, std::set<std::string>& defs) {
    auto isVar = [](const std::string& s) {
        return !s.empty() && s[0] != '.' && s[0] != '#' &&
               !(s[0] >= '0' && s[0] <= '9') && s[0] != '-';
    };

    switch (instr.op) {
        case TACOp::LoadImm:
            defs.insert(instr.result);
            break;
        case TACOp::Assign:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            defs.insert(instr.result);
            break;
        case TACOp::Add: case TACOp::Sub: case TACOp::Mul: case TACOp::Div:
        case TACOp::Mod: case TACOp::BitAnd: case TACOp::BitOr: case TACOp::BitXor:
        case TACOp::ShiftL: case TACOp::ShiftR:
        case TACOp::Eq: case TACOp::NEq: case TACOp::Lt: case TACOp::Gt:
        case TACOp::LEq: case TACOp::GEq:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            if (isVar(instr.arg2)) uses.insert(instr.arg2);
            defs.insert(instr.result);
            break;
        case TACOp::LogNot: case TACOp::BitNot:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            defs.insert(instr.result);
            break;
        case TACOp::Load:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            defs.insert(instr.result);
            break;
        case TACOp::Store:
            if (isVar(instr.result)) uses.insert(instr.result); // address
            if (isVar(instr.arg1)) uses.insert(instr.arg1);     // value
            break;
        case TACOp::AddressOf:
            // arg1 is the variable whose address we take — it's used
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            defs.insert(instr.result);
            break;
        case TACOp::Param:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            break;
        case TACOp::Call:
            if (!instr.result.empty()) defs.insert(instr.result);
            break;
        case TACOp::Return:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            break;
        case TACOp::JumpIfZero: case TACOp::JumpIfNotZero:
            if (isVar(instr.arg1)) uses.insert(instr.arg1);
            break;
        default:
            break;
    }
}

void FunctionCFG::computeLiveness() {
    // Compute gen/kill for each block
    for (auto& bb : blocks) {
        bb->gen.clear();
        bb->kill.clear();
        for (auto& instr : bb->instructions) {
            std::set<std::string> uses, defs;
            getUseDef(instr, uses, defs);
            for (auto& u : uses) {
                if (bb->kill.find(u) == bb->kill.end()) bb->gen.insert(u);
            }
            for (auto& d : defs) {
                bb->kill.insert(d);
            }
        }
    }

    // Fixed-point iteration
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
            auto& bb = blocks[i];

            // liveOut = union of liveIn of successors
            std::set<std::string> newOut;
            for (auto* succ : bb->successors) {
                newOut.insert(succ->liveIn.begin(), succ->liveIn.end());
            }

            // liveIn = gen U (liveOut - kill)
            std::set<std::string> newIn = bb->gen;
            for (auto& v : newOut) {
                if (bb->kill.find(v) == bb->kill.end()) newIn.insert(v);
            }

            if (newIn != bb->liveIn || newOut != bb->liveOut) {
                bb->liveIn = newIn;
                bb->liveOut = newOut;
                changed = true;
            }
        }
    }
}

void FunctionCFG::print(std::ostream& os) const {
    os << "=== CFG for " << name << " ===\n";
    for (auto& bb : blocks) bb->print(os);
}

std::string FunctionCFG::toDot() const {
    std::ostringstream os;
    os << "digraph " << name << " {\n";
    for (auto& bb : blocks) {
        os << "  BB" << bb->id << " [label=\"BB" << bb->id;
        if (!bb->label.empty()) os << " (" << bb->label << ")";
        os << "\"];\n";
        for (auto* succ : bb->successors) {
            os << "  BB" << bb->id << " -> BB" << succ->id << ";\n";
        }
    }
    os << "}\n";
    return os.str();
}

int FunctionCFG::instructionCount() const {
    int count = 0;
    for (auto& bb : blocks) count += static_cast<int>(bb->instructions.size());
    return count;
}

// ─── CFGBuilder ──────────────────────────────────────────────────────

std::vector<FunctionCFG> CFGBuilder::build(const TACProgram& program) {
    std::vector<FunctionCFG> cfgs;
    auto& instrs = program.instructions;

    // Find function boundaries: each function starts with a Label instruction
    // whose result is a function name (not starting with '.')
    std::vector<std::pair<size_t, std::string>> func_starts;
    for (size_t i = 0; i < instrs.size(); ++i) {
        if (instrs[i].op == TACOp::Label && !instrs[i].result.empty() &&
            instrs[i].result[0] != '.') {
            func_starts.push_back({i, instrs[i].result});
        }
    }

    for (size_t fi = 0; fi < func_starts.size(); ++fi) {
        size_t start = func_starts[fi].first;
        size_t end = (fi + 1 < func_starts.size()) ? func_starts[fi + 1].first : instrs.size();

        FunctionCFG cfg;
        cfg.name = func_starts[fi].second;
        cfg.buildFromTAC(instrs, start, end);
        cfgs.push_back(std::move(cfg));
    }

    return cfgs;
}

} // namespace minic
