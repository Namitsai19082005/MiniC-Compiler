#include "reg_alloc.h"
#include <algorithm>

namespace minic {

const std::vector<std::string> RegisterAllocator::kCalleeSaved = {
    "%rbx", "%r12", "%r13", "%r14", "%r15"
};
const std::vector<std::string> RegisterAllocator::kCallerSaved = {
    "%r10", "%r11"
};
const std::vector<std::string> RegisterAllocator::kArgRegs = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

std::vector<LiveInterval> RegisterAllocator::computeLiveIntervals(const FunctionCFG& cfg) {
    std::unordered_map<std::string, LiveInterval> intervals;
    int pos = 0;

    auto isVar = [](const std::string& s) {
        return !s.empty() && s[0] != '.' && !(s[0] >= '0' && s[0] <= '9') && s[0] != '-';
    };

    for (auto& bb : cfg.blocks) {
        for (auto& instr : bb->instructions) {
            auto touch = [&](const std::string& var) {
                if (!isVar(var)) return;
                if (intervals.find(var) == intervals.end()) {
                    intervals[var] = {var, pos, pos};
                } else {
                    intervals[var].end = pos;
                }
            };

            if (!instr.result.empty() && instr.op != TACOp::Label &&
                instr.op != TACOp::Jump && instr.op != TACOp::JumpIfZero &&
                instr.op != TACOp::JumpIfNotZero && instr.op != TACOp::Store) {
                touch(instr.result);
            }
            if (isVar(instr.arg1)) touch(instr.arg1);
            if (isVar(instr.arg2)) touch(instr.arg2);

            ++pos;
        }
    }

    std::vector<LiveInterval> result;
    for (auto& entry : intervals) {
        result.push_back(entry.second);
    }
    return result;
}

void RegisterAllocator::linearScan(std::vector<LiveInterval>& intervals, RegAllocResult& result) {
    // Sort by start point
    std::sort(intervals.begin(), intervals.end(),
              [](const LiveInterval& a, const LiveInterval& b) { return a.start < b.start; });

    // All available registers (callee-saved first for longer-lived vars)
    std::vector<std::string> all_regs;
    for (auto& r : kCalleeSaved) all_regs.push_back(r);
    for (auto& r : kCallerSaved) all_regs.push_back(r);

    std::set<std::string> free_regs(all_regs.begin(), all_regs.end());
    std::vector<std::pair<LiveInterval, std::string>> active; // (interval, reg)
    int next_spill_offset = 0;

    for (auto& interval : intervals) {
        // Expire old intervals
        active.erase(
            std::remove_if(active.begin(), active.end(),
                [&](const std::pair<LiveInterval, std::string>& a) {
                    if (a.first.end < interval.start) {
                        free_regs.insert(a.second);
                        return true;
                    }
                    return false;
                }),
            active.end()
        );

        if (!free_regs.empty()) {
            // Assign a register
            std::string reg = *free_regs.begin();
            free_regs.erase(free_regs.begin());
            result.reg_map[interval.var] = reg;
            active.push_back({interval, reg});

            // Track callee-saved usage
            for (auto& cs : kCalleeSaved) {
                if (reg == cs) result.used_callee_saved.insert(reg);
            }
        } else {
            // Spill: pick the active interval with the furthest endpoint
            auto spill_it = std::max_element(active.begin(), active.end(),
                [](const auto& a, const auto& b) { return a.first.end < b.first.end; });

            if (spill_it != active.end() && spill_it->first.end > interval.end) {
                // Spill the existing one, give its reg to the new interval
                std::string stolen_reg = spill_it->second;
                next_spill_offset += 8;
                result.spill_map[spill_it->first.var] = next_spill_offset;
                result.reg_map.erase(spill_it->first.var);

                active.erase(spill_it);

                result.reg_map[interval.var] = stolen_reg;
                active.push_back({interval, stolen_reg});
            } else {
                // Spill the new interval
                next_spill_offset += 8;
                result.spill_map[interval.var] = next_spill_offset;
            }
        }
    }

    result.spill_area_size = next_spill_offset;
}

RegAllocResult RegisterAllocator::allocate(const FunctionCFG& cfg, int num_params) {
    RegAllocResult result;
    auto intervals = computeLiveIntervals(cfg);
    linearScan(intervals, result);
    return result;
}

} // namespace minic
