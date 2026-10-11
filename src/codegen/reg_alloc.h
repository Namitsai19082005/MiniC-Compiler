#pragma once
#include "ir/cfg.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

namespace minic {

struct LiveInterval {
    std::string var;
    int start = 0;
    int end = 0;
};

struct RegAllocResult {
    std::unordered_map<std::string, std::string> reg_map;   // var → physical reg
    std::unordered_map<std::string, int> spill_map;          // var → stack offset
    std::set<std::string> used_callee_saved;                 // which callee-saved regs used
    int spill_area_size = 0;                                 // total bytes for spills
};

class RegisterAllocator {
public:
    RegAllocResult allocate(const FunctionCFG& cfg, int num_params);

private:
    std::vector<LiveInterval> computeLiveIntervals(const FunctionCFG& cfg);
    void linearScan(std::vector<LiveInterval>& intervals, RegAllocResult& result);

    static const std::vector<std::string> kCalleeSaved;
    static const std::vector<std::string> kCallerSaved;
    static const std::vector<std::string> kArgRegs;
};

} // namespace minic
