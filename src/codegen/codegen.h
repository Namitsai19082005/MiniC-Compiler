#pragma once
#include "ir/cfg.h"
#include "ir/tac.h"
#include "parser/ast.h"
#include "debug/dwarf_emitter.h"
#include <string>
#include <vector>

namespace minic {

class CodeGenerator {
public:
    CodeGenerator(const std::string& source_filename);

    std::string generate(const std::vector<FunctionCFG>& cfgs,
                         const TACProgram& tac_program,
                         Program& ast_program);

private:
    std::string source_filename_;
};

} // namespace minic
