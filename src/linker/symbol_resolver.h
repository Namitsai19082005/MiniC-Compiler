#pragma once
#include "sema/symbol_table.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

namespace minic {

enum class SymbolKind { Function, GlobalVar };
enum class Linkage { External, Internal };

struct LinkerSymbol {
    std::string name;
    SymbolKind kind;
    Linkage linkage = Linkage::External;
    std::string translation_unit;
    bool is_defined = false;
    int section_offset = 0;
};

class SymbolResolver {
public:
    // Phase 1: Collect symbols from each TU's symbol table
    void addTranslationUnit(const std::string& tu_name,
                            const SymbolTable& symtab);

    // Phase 2: Resolve all references
    bool resolve();

    // Phase 3: Report
    std::vector<std::string> getUnresolved() const;
    std::vector<std::string> getDuplicates() const;
    void printLinkMap(std::ostream& os = std::cout) const;

    const std::unordered_map<std::string, LinkerSymbol>& definitions() const { return definitions_; }

private:
    std::unordered_map<std::string, LinkerSymbol> definitions_;
    std::vector<LinkerSymbol> references_;
    std::vector<std::string> unresolved_;
    std::vector<std::string> duplicates_;
};

} // namespace minic
