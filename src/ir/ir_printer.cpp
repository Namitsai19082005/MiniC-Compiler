#include "ir_printer.h"

namespace minic {

void IRPrinter::printTAC(const TACProgram& prog, std::ostream& os) {
    os << "=== Three-Address Code ===\n";
    prog.print(os);
    os << "=== End TAC (" << prog.instructionCount() << " instructions) ===\n";
}

void IRPrinter::printCFG(const std::vector<FunctionCFG>& cfgs, std::ostream& os) {
    for (auto& cfg : cfgs) cfg.print(os);
}

void IRPrinter::printCFGDot(const std::vector<FunctionCFG>& cfgs, std::ostream& os) {
    for (auto& cfg : cfgs) os << cfg.toDot();
}

} // namespace minic
