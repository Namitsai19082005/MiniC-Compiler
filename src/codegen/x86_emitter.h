#pragma once
#include "reg_alloc.h"
#include "ir/cfg.h"
#include "ir/tac.h"
#include "debug/source_map.h"
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>

namespace minic {

struct FunctionInfo {
    std::string name;
    int num_params = 0;
    int local_area_size = 0;      // bytes for locals
    int spill_area_size = 0;      // bytes for spills
    int total_frame_size = 0;     // total stack frame
    std::vector<std::string> param_names;
};

class X86Emitter {
public:
    X86Emitter(const std::string& source_filename);

    void emitFunction(const FunctionCFG& cfg, const FunctionInfo& info,
                      const RegAllocResult& alloc);
    void emitStringLiterals(const std::vector<std::pair<std::string, std::string>>& strings);
    void emitGlobals(const std::vector<std::pair<std::string, int64_t>>& globals);

    std::string getOutput() const { return output_.str(); }
    const SourceMap& sourceMap() const { return source_map_; }

private:
    void emitLine(const std::string& line);
    void emitComment(const std::string& comment);
    void emitLabel(const std::string& label);

    // Get the location of a variable (register or stack offset)
    std::string getVarLoc(const std::string& var, const RegAllocResult& alloc,
                          const FunctionInfo& info);
    // Load a variable into a register, return the register name
    std::string loadToReg(const std::string& var, const std::string& preferred_reg,
                          const RegAllocResult& alloc, const FunctionInfo& info);
    // Store a register into a variable location
    void storeFromReg(const std::string& reg, const std::string& var,
                      const RegAllocResult& alloc, const FunctionInfo& info);

    void emitInstruction(const TACInstr& instr, const RegAllocResult& alloc,
                         const FunctionInfo& info);
    void emitPrologue(const FunctionInfo& info, const RegAllocResult& alloc);
    void emitEpilogue(const FunctionInfo& info, const RegAllocResult& alloc);

    bool isConstant(const std::string& s) const;

    std::ostringstream output_;
    SourceMap source_map_;
    std::string source_filename_;
    int asm_line_ = 0;
    int param_count_for_call_ = 0;

    // Track parameter -> argument register mapping
    std::unordered_map<std::string, int> param_index_;

    // Stack slot management: variable name -> stack offset from %rbp
    std::unordered_map<std::string, int> stack_slots_;
    int next_stack_slot_ = 0;

    static const std::vector<std::string> kArgRegs;
};

} // namespace minic
