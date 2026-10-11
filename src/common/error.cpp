#include "error.h"

namespace minic {

std::string Diagnostic::toString() const {
    std::string prefix;
    switch (level) {
        case DiagLevel::Error:   prefix = "error"; break;
        case DiagLevel::Warning: prefix = "warning"; break;
        case DiagLevel::Note:    prefix = "note"; break;
    }
    return loc.toString() + ": " + prefix + ": " + message;
}

void DiagnosticEngine::report(DiagLevel level, const SourceLoc& loc, const std::string& msg) {
    diags_.push_back({level, loc, msg});
    if (level == DiagLevel::Error) {
        ++error_count_;
    }
}

void DiagnosticEngine::error(const SourceLoc& loc, const std::string& msg) {
    report(DiagLevel::Error, loc, msg);
}

void DiagnosticEngine::warning(const SourceLoc& loc, const std::string& msg) {
    report(DiagLevel::Warning, loc, msg);
}

void DiagnosticEngine::note(const SourceLoc& loc, const std::string& msg) {
    report(DiagLevel::Note, loc, msg);
}

void DiagnosticEngine::printAll(std::ostream& os) const {
    for (auto& d : diags_) {
        os << d.toString() << "\n";
    }
}

} // namespace minic
