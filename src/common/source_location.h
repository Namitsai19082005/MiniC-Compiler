#pragma once
#include <string>
#include <sstream>

namespace minic {

struct SourceLoc {
    std::string filename;
    int line   = 1;
    int column = 1;

    std::string toString() const {
        std::ostringstream os;
        os << filename << ":" << line << ":" << column;
        return os.str();
    }
};

} // namespace minic
