#pragma once
#include <string>
#include <vector>
#include <sstream>

namespace minic {

struct DWARFFunctionInfo {
    std::string name;
    bool is_external = true;
};

class DWARFEmitter {
public:
    DWARFEmitter(const std::string& source_filename, const std::string& producer = "minic 1.0");

    void addFunction(const DWARFFunctionInfo& func);
    std::string emit() const;

private:
    std::string source_filename_;
    std::string producer_;
    std::vector<DWARFFunctionInfo> functions_;
};

} // namespace minic
