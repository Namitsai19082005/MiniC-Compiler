#pragma once
#include "type.h"
#include "common/source_location.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace minic {

struct Symbol {
    std::string name;
    TypeInfo type;
    bool is_function = false;
    bool is_global = false;
    bool is_defined = false;   // for functions: has body vs just declared
    bool is_array = false;
    int array_size = 0;
    std::vector<TypeInfo> param_types;
    int scope_depth = 0;
    SourceLoc decl_loc;
    int stack_offset = 0; // filled in during codegen
};

class SymbolTable {
public:
    void pushScope();
    void popScope();
    bool declare(const Symbol& sym);
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name);
    int currentDepth() const { return static_cast<int>(scopes_.size()) - 1; }

    // For linker helper: export symbol info
    std::vector<Symbol> getGlobalSymbols() const;
    std::vector<std::string> getExternalRefs() const;

    // Track referenced but undefined symbols
    void addExternalRef(const std::string& name);

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    std::vector<std::string> external_refs_;
};

} // namespace minic
