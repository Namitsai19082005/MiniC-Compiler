#include "common/error.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"
#include "sema/sema.h"
#include "ir/ir_gen.h"
#include "ir/cfg.h"
#include "ir/ir_printer.h"
#include "opt/constant_folding.h"
#include "opt/dead_code_elim.h"
#include "opt/copy_propagation.h"
#include "codegen/codegen.h"
#include "linker/link_helper.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

using namespace minic;

static void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options] <source.mc> [-o output.s]\n"
              << "Options:\n"
              << "  -o <file>          Output assembly file (default: stdout)\n"
              << "  --dump-ast         Print AST after parsing\n"
              << "  --dump-ir          Print TAC after IR generation\n"
              << "  --dump-ir-before   Print TAC before optimizations\n"
              << "  --dump-ir-after    Print TAC after optimizations\n"
              << "  --dump-cfg         Print CFG\n"
              << "  --no-opt           Disable optimizations\n"
              << "  --link <file>...   Link with additional translation units\n"
              << "  --link-map         Print linker symbol map\n"
              << "  -v, --verbose      Verbose output\n"
              << "  -h, --help         Show this help\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string input_file;
    std::string output_file;
    bool dump_ast = false;
    bool dump_ir = false;
    bool dump_ir_before = false;
    bool dump_ir_after = false;
    bool dump_cfg = false;
    bool no_opt = false;
    bool verbose = false;
    bool link_map = false;
    std::vector<std::string> link_files;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--dump-ast") {
            dump_ast = true;
        } else if (arg == "--dump-ir") {
            dump_ir = true;
        } else if (arg == "--dump-ir-before") {
            dump_ir_before = true;
        } else if (arg == "--dump-ir-after") {
            dump_ir_after = true;
        } else if (arg == "--dump-cfg") {
            dump_cfg = true;
        } else if (arg == "--no-opt") {
            no_opt = true;
        } else if (arg == "--link") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                link_files.push_back(argv[++i]);
            }
        } else if (arg == "--link-map") {
            link_map = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg[0] != '-') {
            input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (input_file.empty()) {
        std::cerr << "error: no input file specified\n";
        return 1;
    }

    // Multi-TU linking mode
    if (!link_files.empty()) {
        std::vector<std::string> all_files = {input_file};
        all_files.insert(all_files.end(), link_files.begin(), link_files.end());

        std::string exe_name = output_file.empty() ? "a.out" : output_file;
        LinkHelper linker;
        bool ok = linker.linkTranslationUnits(all_files, exe_name, verbose || link_map);
        if (link_map) {
            linker.resolver().printLinkMap(std::cerr);
        }
        return ok ? 0 : 1;
    }

    // ── Single-file compilation pipeline ──────────────────────

    // Read source
    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "error: cannot open '" << input_file << "'\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    DiagnosticEngine diag;

    // Stage 1: Lex & Parse
    Lexer lexer(source, input_file, diag);
    Parser parser(lexer, diag);
    auto ast = parser.parse();

    if (diag.hasErrors()) {
        diag.printAll();
        return 1;
    }

    if (dump_ast) {
        ASTPrinter printer(std::cerr);
        printer.print(*ast);
    }

    // Stage 2: Semantic Analysis
    SemanticAnalyzer sema(diag);
    sema.analyze(*ast);

    if (diag.hasErrors()) {
        diag.printAll();
        return 1;
    }

    // Stage 3: IR Generation
    IRGenerator ir_gen;
    auto tac = ir_gen.generate(*ast);

    if (dump_ir || dump_ir_before) {
        std::cerr << "=== IR Before Optimization ===\n";
        IRPrinter::printTAC(tac, std::cerr);
    }

    // Stage 3b: CFG Construction
    CFGBuilder cfg_builder;
    auto cfgs = cfg_builder.build(tac);

    if (dump_cfg) {
        IRPrinter::printCFG(cfgs, std::cerr);
    }

    // Stage 4: Optimizations
    int instr_before = 0, instr_after = 0;
    for (auto& cfg : cfgs) instr_before += cfg.instructionCount();

    if (!no_opt) {
        ConstantFolding cf;
        DeadCodeElimination dce;
        CopyPropagation cp;

        for (auto& cfg : cfgs) {
            bool changed = true;
            int iterations = 0;
            while (changed && iterations < 10) {
                changed = false;
                changed |= cf.run(cfg);
                changed |= cp.run(cfg);
                changed |= dce.run(cfg);
                ++iterations;
            }
        }
    }

    for (auto& cfg : cfgs) instr_after += cfg.instructionCount();

    if (verbose) {
        double reduction = instr_before > 0 ?
            (1.0 - static_cast<double>(instr_after) / instr_before) * 100.0 : 0.0;
        std::cerr << "Optimization: " << instr_before << " → " << instr_after
                  << " instructions (" << reduction << "% reduction)\n";
    }

    if (dump_ir || dump_ir_after) {
        std::cerr << "=== IR After Optimization ===\n";
        // Rebuild TAC from CFG blocks for printing
        for (auto& cfg : cfgs) {
            cfg.print(std::cerr);
        }
    }

    // Stage 5: x86-64 Code Generation
    CodeGenerator codegen(input_file);
    std::string asm_code = codegen.generate(cfgs, tac, *ast);

    // Output
    if (output_file.empty()) {
        std::cout << asm_code;
    } else {
        std::ofstream out(output_file);
        if (!out.is_open()) {
            std::cerr << "error: cannot open output file '" << output_file << "'\n";
            return 1;
        }
        out << asm_code;
        out.close();
        if (verbose) {
            std::cerr << "Wrote " << output_file << "\n";
        }
    }

    return 0;
}
