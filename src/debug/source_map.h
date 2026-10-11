#pragma once
#include "common/source_location.h"
#include <vector>

namespace minic {

struct SourceMap {
    struct Entry {
        int asm_line;
        SourceLoc src_loc;
    };
    std::vector<Entry> entries;

    void record(int asm_line, SourceLoc loc) {
        entries.push_back({asm_line, loc});
    }
};

} // namespace minic
