#pragma once
#include "source_location.h"
#include <string>
#include <vector>
#include <iostream>

namespace minic {

enum class DiagLevel { Error, Warning, Note };

struct Diagnostic {
    DiagLevel level;
    SourceLoc loc;
    std::string message;

    std::string toString() const;
};

class DiagnosticEngine {
public:
    void report(DiagLevel level, const SourceLoc& loc, const std::string& msg);
    void error(const SourceLoc& loc, const std::string& msg);
    void warning(const SourceLoc& loc, const std::string& msg);
    void note(const SourceLoc& loc, const std::string& msg);

    bool hasErrors() const { return error_count_ > 0; }
    int errorCount() const { return error_count_; }
    const std::vector<Diagnostic>& diagnostics() const { return diags_; }

    void printAll(std::ostream& os = std::cerr) const;

private:
    std::vector<Diagnostic> diags_;
    int error_count_ = 0;
};

} // namespace minic
