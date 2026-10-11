#include "dwarf_emitter.h"

namespace minic {

DWARFEmitter::DWARFEmitter(const std::string& source_filename, const std::string& producer)
    : source_filename_(source_filename), producer_(producer) {}

void DWARFEmitter::addFunction(const DWARFFunctionInfo& func) {
    functions_.push_back(func);
}

std::string DWARFEmitter::emit() const {
    std::ostringstream os;

    // ── .debug_info section ──────────────────────────────────
    os << "\n";
    os << "    # ── DWARF Debug Info ──\n";
    os << "    .section .debug_info,\"\",@progbits\n";
    os << ".Ldebug_info_start:\n";
    os << "    .long   .Ldebug_info_end - .Ldebug_info_start - 4  # Length\n";
    os << "    .short  4                   # DWARF Version 4\n";
    os << "    .long   .Ldebug_abbrev      # Abbrev Offset\n";
    os << "    .byte   8                   # Pointer Size (64-bit)\n";
    os << "\n";
    os << "    # DW_TAG_compile_unit (abbrev 1)\n";
    os << "    .byte   1                   # Abbrev code 1\n";
    os << "    .string \"" << source_filename_ << "\"  # DW_AT_name\n";
    os << "    .string \"" << producer_ << "\"         # DW_AT_producer\n";
    os << "    .short  0x000c              # DW_AT_language (C)\n";
    os << "    .quad   .Ltext_start        # DW_AT_low_pc\n";
    os << "    .quad   .Ltext_end - .Ltext_start  # DW_AT_high_pc\n";
    os << "    .long   .Ldebug_line        # DW_AT_stmt_list\n";
    os << "\n";

    // Emit DW_TAG_subprogram for each function (abbrev 2)
    for (auto& func : functions_) {
        os << "    # DW_TAG_subprogram (abbrev 2) — " << func.name << "\n";
        os << "    .byte   2                   # Abbrev code 2\n";
        os << "    .string \"" << func.name << "\"     # DW_AT_name\n";
        os << "    .quad   " << func.name << "          # DW_AT_low_pc\n";
        os << "    .quad   .L" << func.name << "_epilogue - " << func.name << "  # DW_AT_high_pc\n";
        os << "    .byte   " << (func.is_external ? "1" : "0") << "  # DW_AT_external\n";
        os << "\n";
    }

    os << "    .byte   0                   # End of children\n";
    os << ".Ldebug_info_end:\n";
    os << "\n";

    // ── .debug_abbrev section ────────────────────────────────
    os << "    .section .debug_abbrev,\"\",@progbits\n";
    os << ".Ldebug_abbrev:\n";
    os << "    # Abbrev 1: DW_TAG_compile_unit\n";
    os << "    .byte   1                   # Abbrev code\n";
    os << "    .byte   0x11                # DW_TAG_compile_unit\n";
    os << "    .byte   1                   # DW_CHILDREN_yes\n";
    os << "    .byte   0x03, 0x08          # DW_AT_name, DW_FORM_string\n";
    os << "    .byte   0x25, 0x08          # DW_AT_producer, DW_FORM_string\n";
    os << "    .byte   0x13, 0x05          # DW_AT_language, DW_FORM_data2\n";
    os << "    .byte   0x11, 0x01          # DW_AT_low_pc, DW_FORM_addr\n";
    os << "    .byte   0x12, 0x07          # DW_AT_high_pc, DW_FORM_data8\n";
    os << "    .byte   0x10, 0x17          # DW_AT_stmt_list, DW_FORM_sec_offset\n";
    os << "    .byte   0, 0                # End of attributes\n";
    os << "\n";
    os << "    # Abbrev 2: DW_TAG_subprogram\n";
    os << "    .byte   2                   # Abbrev code\n";
    os << "    .byte   0x2e                # DW_TAG_subprogram\n";
    os << "    .byte   0                   # DW_CHILDREN_no\n";
    os << "    .byte   0x03, 0x08          # DW_AT_name, DW_FORM_string\n";
    os << "    .byte   0x11, 0x01          # DW_AT_low_pc, DW_FORM_addr\n";
    os << "    .byte   0x12, 0x07          # DW_AT_high_pc, DW_FORM_data8\n";
    os << "    .byte   0x3f, 0x0c          # DW_AT_external, DW_FORM_flag\n";
    os << "    .byte   0, 0                # End of attributes\n";
    os << "\n";
    os << "    .byte   0                   # End of abbrevs\n";
    os << "\n";

    // ── .debug_line section (GAS appends line table from .loc directives) ──
    os << "    .section .debug_line,\"\",@progbits\n";
    os << ".Ldebug_line:\n";
    os << "\n";

    // ── Stack protection marker ──────────────────────────────
    os << "    .section .note.GNU-stack,\"\",@progbits\n";

    return os.str();
}

} // namespace minic
