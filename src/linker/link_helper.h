#pragma once
#include "symbol_resolver.h"
#include <string>
#include <vector>

namespace minic {

class LinkHelper {
public:
    // Compile multiple .mc files and resolve cross-references
    bool linkTranslationUnits(const std::vector<std::string>& source_files,
                              const std::string& output_name,
                              bool verbose = false);

    const SymbolResolver& resolver() const { return resolver_; }

private:
    SymbolResolver resolver_;
};

} // namespace minic
