#pragma once
#include "tac.h"
#include <vector>
#include <memory>
#include <set>
#include <string>
#include <iostream>

namespace minic {

struct BasicBlock {
    int id = 0;
    std::string label;
    std::vector<TACInstr> instructions;
    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;

    // Dataflow analysis sets
    std::set<std::string> gen, kill;
    std::set<std::string> liveIn, liveOut;

    bool terminated() const;
    void print(std::ostream& os) const;
};

struct FunctionCFG {
    std::string name;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock* entry = nullptr;

    void buildFromTAC(const std::vector<TACInstr>& instrs, size_t start, size_t end);
    void computeLiveness();
    void print(std::ostream& os) const;
    std::string toDot() const;

    int instructionCount() const;
};

class CFGBuilder {
public:
    // Build CFGs for all functions in a TAC program
    std::vector<FunctionCFG> build(const TACProgram& program);
};

} // namespace minic
