#include "codegen.h"
#include "x86_emitter.h"
#include "reg_alloc.h"

namespace minic {

CodeGenerator::CodeGenerator(const std::string& source_filename)
    : source_filename_(source_filename) {}

std::string CodeGenerator::generate(const std::vector<FunctionCFG>& cfgs,
                                     const TACProgram& tac_program,
                                     Program& ast_program) {
    X86Emitter emitter(source_filename_);
    DWARFEmitter dwarf(source_filename_);
    RegisterAllocator reg_alloc;

    // Emit text section markers
    emitter.emitStringLiterals(tac_program.string_literals);

    // Collect global declarations
    std::vector<std::pair<std::string, int64_t>> globals;
    for (auto& decl : ast_program.decls) {
        if (auto* gd = dynamic_cast<GlobalDecl*>(decl.get())) {
            int64_t val = 0;
            if (gd->init) {
                if (auto* lit = dynamic_cast<IntLiteralExpr*>(gd->init.get())) {
                    val = lit->value;
                }
            }
            globals.push_back({gd->name, val});
        }
    }
    emitter.emitGlobals(globals);

    // Build function info from AST
    std::unordered_map<std::string, FunctionInfo> func_infos;
    for (auto& decl : ast_program.decls) {
        if (auto* fd = dynamic_cast<FunctionDef*>(decl.get())) {
            if (fd->is_declaration) continue;
            FunctionInfo info;
            info.name = fd->name;
            info.num_params = static_cast<int>(fd->params.size());
            for (auto& p : fd->params) {
                info.param_names.push_back(p.name);
            }
            // Override with IR-renamed param names if available
            auto renamed_it = tac_program.func_param_names.find(fd->name);
            if (renamed_it != tac_program.func_param_names.end()) {
                info.param_names = renamed_it->second;
            }
            func_infos[fd->name] = info;

            dwarf.addFunction({fd->name, true});
        }
    }

    // Emit .Ltext_start marker before functions
    std::string output = emitter.getOutput();
    output += ".Ltext_start:\n";

    // Process each function CFG
    for (auto& cfg : cfgs) {
        auto it = func_infos.find(cfg.name);
        FunctionInfo info;
        if (it != func_infos.end()) {
            info = it->second;
        } else {
            info.name = cfg.name;
            info.local_area_size = 128;
        }

        // Register allocation
        RegAllocResult alloc = reg_alloc.allocate(cfg, info.num_params);
        info.spill_area_size = alloc.spill_area_size;
        info.total_frame_size = info.local_area_size + info.spill_area_size;

        // Emit the function
        X86Emitter func_emitter(source_filename_);
        func_emitter.emitFunction(cfg, info, alloc);
        output += func_emitter.getOutput();
    }

    output += ".Ltext_end:\n\n";

    // Append DWARF debug info
    output += dwarf.emit();

    return output;
}

} // namespace minic
