#include "symbol_table.h"

namespace minic {

void SymbolTable::pushScope() {
    scopes_.emplace_back();
}

void SymbolTable::popScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

bool SymbolTable::declare(const Symbol& sym) {
    if (scopes_.empty()) pushScope();
    auto& scope = scopes_.back();
    if (scope.count(sym.name)) return false; // redeclaration
    scope[sym.name] = sym;
    scope[sym.name].scope_depth = currentDepth();
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) return &it->second;
    }
    return nullptr;
}

Symbol* SymbolTable::lookupCurrentScope(const std::string& name) {
    if (scopes_.empty()) return nullptr;
    auto it = scopes_.back().find(name);
    if (it != scopes_.back().end()) return &it->second;
    return nullptr;
}

std::vector<Symbol> SymbolTable::getGlobalSymbols() const {
    std::vector<Symbol> result;
    if (!scopes_.empty()) {
        for (auto& entry : scopes_[0]) {
            result.push_back(entry.second);
        }
    }
    return result;
}

std::vector<std::string> SymbolTable::getExternalRefs() const {
    return external_refs_;
}

void SymbolTable::addExternalRef(const std::string& name) {
    external_refs_.push_back(name);
}

} // namespace minic
