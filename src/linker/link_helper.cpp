#include "link_helper.h"
#include "common/error.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/sema.h"
#include "ir/ir_gen.h"
#include "ir/cfg.h"
#include "opt/constant_folding.h"
#include "opt/dead_code_elim.h"
#include "opt/copy_propagation.h"
#include "codegen/codegen.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace minic {

bool LinkHelper::linkTranslationUnits(const std::vector<std::string>& source_files,
                                       const std::string& output_name,
                                       bool verbose) {
    std::vector<std::string> asm_files;

    // Phase 1: Compile each TU independently
    for (auto& src : source_files) {
        std::ifstream file(src);
        if (!file.is_open()) {
            std::cerr << "error: cannot open '" << src << "'\n";
            return false;
        }
        std::string source((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        DiagnosticEngine diag;
        Lexer lexer(source, src, diag);
        Parser parser(lexer, diag);
        auto ast = parser.parse();

        if (diag.hasErrors()) {
            diag.printAll();
            return false;
        }

        SemanticAnalyzer sema(diag);
        sema.analyze(*ast);

        if (diag.hasErrors()) {
            diag.printAll();
            return false;
        }

        // Collect symbols for linker resolution
        resolver_.addTranslationUnit(src, sema.symbolTable());

        // Generate IR
        IRGenerator ir_gen;
        auto tac = ir_gen.generate(*ast);

        CFGBuilder cfg_builder;
        auto cfgs = cfg_builder.build(tac);

        // Optimizations
        ConstantFolding cf;
        DeadCodeElimination dce;
        CopyPropagation cp;
        for (auto& cfg : cfgs) {
            bool changed = true;
            while (changed) {
                changed = false;
                changed |= cf.run(cfg);
                changed |= cp.run(cfg);
                changed |= dce.run(cfg);
            }
        }

        // Code generation
        CodeGenerator codegen(src);
        std::string asm_code = codegen.generate(cfgs, tac, *ast);

        // Write .s file
        std::string asm_path = src;
        auto dot = asm_path.rfind('.');
        if (dot != std::string::npos) asm_path = asm_path.substr(0, dot);
        asm_path += ".s";

        std::ofstream asm_file(asm_path);
        asm_file << asm_code;
        asm_file.close();

        asm_files.push_back(asm_path);
        if (verbose) {
            std::cerr << "Compiled " << src << " → " << asm_path << "\n";
        }
    }

    // Phase 2: Resolve symbols
    bool resolved = resolver_.resolve();
    if (verbose) {
        resolver_.printLinkMap(std::cerr);
    }

    if (!resolved) {
        auto unresolved = resolver_.getUnresolved();
        for (auto& sym : unresolved) {
            std::cerr << "error: unresolved external symbol: " << sym << "\n";
        }
        auto dups = resolver_.getDuplicates();
        for (auto& msg : dups) {
            std::cerr << "error: " << msg << "\n";
        }
        return false;
    }

    // Phase 3: Invoke assembler and linker
    // Build the gcc command
    std::string cmd = "gcc -no-pie -g";
    for (auto& asm_f : asm_files) {
        cmd += " " + asm_f;
    }
    cmd += " -o " + output_name;

    if (verbose) {
        std::cerr << "Link: " << cmd << "\n";
    }

    int ret = system(cmd.c_str());
    return ret == 0;
}

} // namespace minic
