#include "dead_code_elim.h"
#include <set>
#include <queue>
#include <algorithm>

namespace minic {

bool DeadCodeElimination::run(FunctionCFG& cfg) {
    bool changed = false;
    changed |= eliminateUnreachableBlocks(cfg);
    cfg.computeLiveness();
    changed |= eliminateDeadAssignments(cfg);
    return changed;
}

bool DeadCodeElimination::eliminateUnreachableBlocks(FunctionCFG& cfg) {
    if (!cfg.entry || cfg.blocks.empty()) return false;

    // BFS from entry
    std::set<int> reachable;
    std::queue<BasicBlock*> worklist;
    worklist.push(cfg.entry);
    reachable.insert(cfg.entry->id);

    while (!worklist.empty()) {
        auto* bb = worklist.front(); worklist.pop();
        for (auto* succ : bb->successors) {
            if (reachable.insert(succ->id).second) {
                worklist.push(succ);
            }
        }
    }

    // Remove unreachable blocks
    bool changed = false;
    auto it = cfg.blocks.begin();
    while (it != cfg.blocks.end()) {
        if (reachable.find((*it)->id) == reachable.end()) {
            // Remove this block from predecessor lists of its successors
            for (auto* succ : (*it)->successors) {
                auto& preds = succ->predecessors;
                preds.erase(std::remove(preds.begin(), preds.end(), it->get()), preds.end());
            }
            it = cfg.blocks.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    return changed;
}

bool DeadCodeElimination::eliminateDeadAssignments(FunctionCFG& cfg) {
    bool changed = false;

    for (auto& bb : cfg.blocks) {
        // Compute what's live at each point going backwards
        std::set<std::string> live = bb->liveOut;

        // Process instructions in reverse
        for (int i = static_cast<int>(bb->instructions.size()) - 1; i >= 0; --i) {
            auto& instr = bb->instructions[i];

            // Skip instructions with side effects
            if (instr.op == TACOp::Call || instr.op == TACOp::Store ||
                instr.op == TACOp::Param || instr.op == TACOp::Return ||
                instr.op == TACOp::Label || instr.op == TACOp::Jump ||
                instr.op == TACOp::JumpIfZero || instr.op == TACOp::JumpIfNotZero) {
                // Update liveness
                if (!instr.result.empty() && instr.op != TACOp::Jump &&
                    instr.op != TACOp::JumpIfZero && instr.op != TACOp::JumpIfNotZero &&
                    instr.op != TACOp::Store && instr.op != TACOp::Label) {
                    live.erase(instr.result);
                }
                // Mark uses as live
                auto isVar = [](const std::string& s) {
                    return !s.empty() && s[0] != '.' && !(s[0] >= '0' && s[0] <= '9') && s[0] != '-';
                };
                if (isVar(instr.arg1)) live.insert(instr.arg1);
                if (isVar(instr.arg2)) live.insert(instr.arg2);
                continue;
            }

            // Check if the result is dead
            if (!instr.result.empty() && live.find(instr.result) == live.end()) {
                // Dead assignment: result is not live
                // Don't remove if it's a named variable (not a temp)
                if (instr.result[0] == '_') {
                    instr.op = TACOp::Nop;
                    instr.result.clear();
                    instr.arg1.clear();
                    instr.arg2.clear();
                    changed = true;
                    continue;
                }
            }

            // Update liveness
            live.erase(instr.result);
            auto isVar = [](const std::string& s) {
                return !s.empty() && s[0] != '.' && !(s[0] >= '0' && s[0] <= '9') && s[0] != '-';
            };
            if (isVar(instr.arg1)) live.insert(instr.arg1);
            if (isVar(instr.arg2)) live.insert(instr.arg2);
        }
    }

    // Remove NOP instructions
    if (changed) {
        for (auto& bb : cfg.blocks) {
            bb->instructions.erase(
                std::remove_if(bb->instructions.begin(), bb->instructions.end(),
                    [](const TACInstr& i) { return i.op == TACOp::Nop; }),
                bb->instructions.end()
            );
        }
    }

    return changed;
}

} // namespace minic
