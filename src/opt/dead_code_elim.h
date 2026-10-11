#pragma once
#include "pass.h"

namespace minic {

class DeadCodeElimination : public OptimizationPass {
public:
    std::string name() const override { return "DeadCodeElimination"; }
    bool run(FunctionCFG& cfg) override;

private:
    bool eliminateUnreachableBlocks(FunctionCFG& cfg);
    bool eliminateDeadAssignments(FunctionCFG& cfg);
};

} // namespace minic
