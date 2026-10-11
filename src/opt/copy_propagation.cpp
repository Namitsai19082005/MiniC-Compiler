#include "copy_propagation.h"
#include <unordered_map>

namespace minic {

bool CopyPropagation::run(FunctionCFG& cfg) {
    bool changed = false;

    for (auto& bb : cfg.blocks) {
        // Map: target → source (where target = source was a copy)
        std::unordered_map<std::string, std::string> copies;

        for (auto& instr : bb->instructions) {
            // Record copies: x = y (simple assign where arg1 is a variable)
            if (instr.op == TACOp::Assign) {
                bool arg_is_var = !instr.arg1.empty() && instr.arg1[0] != '.' &&
                                  !(instr.arg1[0] >= '0' && instr.arg1[0] <= '9') &&
                                  instr.arg1[0] != '-';
                if (arg_is_var) {
                    // Resolve chains: if arg1 is itself a copy, use the source
                    std::string src = instr.arg1;
                    while (copies.count(src)) {
                        std::string next = copies[src];
                        if (next == src) break; // prevent infinite loop
                        src = next;
                    }
                    copies[instr.result] = src;
                }
            }

            // Propagate copies in uses
            auto propagate = [&](std::string& arg) {
                if (arg.empty()) return;
                auto it = copies.find(arg);
                if (it != copies.end() && it->second != arg) {
                    arg = it->second;
                    changed = true;
                }
            };

            // Don't propagate in the result of Assign (that's the target)
            if (instr.op != TACOp::Assign) {
                propagate(instr.arg1);
            }
            propagate(instr.arg2);

            // For Assign, propagate in arg1
            if (instr.op == TACOp::Assign) {
                std::string& arg = instr.arg1;
                auto it = copies.find(arg);
                if (it != copies.end() && it->second != arg) {
                    arg = it->second;
                    changed = true;
                    // Update the copy record
                    copies[instr.result] = arg;
                }
            }

            // Invalidate copies when source is redefined
            if (!instr.result.empty() && instr.op != TACOp::Label &&
                instr.op != TACOp::Jump && instr.op != TACOp::JumpIfZero &&
                instr.op != TACOp::JumpIfNotZero && instr.op != TACOp::Store &&
                instr.op != TACOp::Param && instr.op != TACOp::Return) {
                // Any copy that uses this result as source is now invalid
                auto it2 = copies.begin();
                while (it2 != copies.end()) {
                    if (it2->second == instr.result && it2->first != instr.result) {
                        it2 = copies.erase(it2);
                    } else {
                        ++it2;
                    }
                }
            }
        }
    }

    return changed;
}

} // namespace minic
