#pragma once
#include "pass.h"

namespace minic {

class ConstantFolding : public OptimizationPass {
public:
    std::string name() const override { return "ConstantFolding"; }
    bool run(FunctionCFG& cfg) override;
};

} // namespace minic
