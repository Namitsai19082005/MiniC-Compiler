#include "symbol_resolver.h"
#include <algorithm>

namespace minic {

void SymbolResolver::addTranslationUnit(const std::string& tu_name,
                                         const SymbolTable& symtab) {
    auto globals = symtab.getGlobalSymbols();
    for (auto& sym : globals) {
        LinkerSymbol lsym;
        lsym.name = sym.name;
        lsym.kind = sym.is_function ? SymbolKind::Function : SymbolKind::GlobalVar;
        lsym.translation_unit = tu_name;
        lsym.is_defined = sym.is_defined || !sym.is_function; // vars are always "defined"

        if (lsym.is_defined) {
            // Check for duplicate definitions
            auto it = definitions_.find(lsym.name);
            if (it != definitions_.end()) {
                duplicates_.push_back("multiple definition of '" + lsym.name +
                                      "' in " + it->second.translation_unit +
                                      " and " + tu_name);
            } else {
                definitions_[lsym.name] = lsym;
            }
        } else {
            // It's a reference (declared but not defined)
            references_.push_back(lsym);
        }
    }

    // Also collect external references
    auto ext_refs = symtab.getExternalRefs();
    for (auto& ref_name : ext_refs) {
        LinkerSymbol lsym;
        lsym.name = ref_name;
        lsym.kind = SymbolKind::Function;
        lsym.translation_unit = tu_name;
        lsym.is_defined = false;
        references_.push_back(lsym);
    }
}

bool SymbolResolver::resolve() {
    unresolved_.clear();

    for (auto& ref : references_) {
        // Skip built-in functions
        if (ref.name == "printf" || ref.name == "malloc" || ref.name == "free") {
            continue;
        }

        auto it = definitions_.find(ref.name);
        if (it == definitions_.end()) {
            // Check for duplicates in unresolved
            if (std::find(unresolved_.begin(), unresolved_.end(), ref.name) == unresolved_.end()) {
                unresolved_.push_back(ref.name);
            }
        }
    }

    return unresolved_.empty() && duplicates_.empty();
}

std::vector<std::string> SymbolResolver::getUnresolved() const {
    return unresolved_;
}

std::vector<std::string> SymbolResolver::getDuplicates() const {
    return duplicates_;
}

void SymbolResolver::printLinkMap(std::ostream& os) const {
    os << "\n=== Link Map ===\n";
    os << "SYMBOL" << std::string(20, ' ') << "KIND" << std::string(12, ' ')
       << "DEFINED_IN" << std::string(10, ' ') << "REFERENCED_BY\n";
    os << std::string(80, '-') << "\n";

    for (auto& entry : definitions_) {
        auto& name = entry.first;
        auto& sym = entry.second;
        os << name;
        int pad = 26 - static_cast<int>(name.size());
        if (pad > 0) os << std::string(pad, ' ');

        os << (sym.kind == SymbolKind::Function ? "Function" : "GlobalVar");
        os << std::string(8, ' ');
        os << sym.translation_unit;

        // Find who references this symbol
        std::string refs;
        for (auto& ref : references_) {
            if (ref.name == name && ref.translation_unit != sym.translation_unit) {
                if (!refs.empty()) refs += ", ";
                refs += ref.translation_unit;
            }
        }
        if (!refs.empty()) {
            int pad2 = 20 - static_cast<int>(sym.translation_unit.size());
            if (pad2 > 0) os << std::string(pad2, ' ');
            os << refs;
        }
        os << "\n";
    }

    if (!unresolved_.empty()) {
        os << "\nERROR: Unresolved symbols:\n";
        for (auto& sym : unresolved_) {
            os << "  - " << sym << "\n";
        }
    }
    if (!duplicates_.empty()) {
        os << "\nERROR: Duplicate definitions:\n";
        for (auto& msg : duplicates_) {
            os << "  - " << msg << "\n";
        }
    }

    if (unresolved_.empty() && duplicates_.empty()) {
        os << "\nAll symbols resolved successfully.\n";
    }
    os << "=== End Link Map ===\n";
}

} // namespace minic
