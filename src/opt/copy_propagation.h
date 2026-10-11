#pragma once
#include "pass.h"

namespace minic {

class CopyPropagation : public OptimizationPass {
public:
    std::string name() const override { return "CopyPropagation"; }
    bool run(FunctionCFG& cfg) override;
};

} // namespace minic
