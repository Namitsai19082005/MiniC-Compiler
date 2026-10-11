#pragma once
#include "tac.h"
#include "cfg.h"
#include <iostream>

namespace minic {

class IRPrinter {
public:
    static void printTAC(const TACProgram& prog, std::ostream& os = std::cout);
    static void printCFG(const std::vector<FunctionCFG>& cfgs, std::ostream& os = std::cout);
    static void printCFGDot(const std::vector<FunctionCFG>& cfgs, std::ostream& os = std::cout);
};

} // namespace minic
