#pragma once
#include "ir/cfg.h"
#include <string>

namespace minic {

class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    virtual std::string name() const = 0;
    virtual bool run(FunctionCFG& cfg) = 0; // returns true if changes were made
};

} // namespace minic
