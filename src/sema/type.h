#pragma once
#include <string>

namespace minic {

struct TypeInfo {
    enum BaseType { Int, Char, Void };
    BaseType base = Int;
    int pointer_depth = 0; // 0 = value, 1 = T*, 2 = T**

    bool isPointer() const { return pointer_depth > 0; }
    bool isVoid() const { return base == Void && pointer_depth == 0; }
    bool isIntegral() const { return (base == Int || base == Char) && pointer_depth == 0; }

    TypeInfo deref() const { return {base, pointer_depth - 1}; }
    TypeInfo addr() const { return {base, pointer_depth + 1}; }

    // Size in bytes (our int is 64-bit for simplicity)
    int size() const {
        if (pointer_depth > 0) return 8; // pointers are always 8 bytes
        switch (base) {
            case Int:  return 8;
            case Char: return 1;
            case Void: return 0;
        }
        return 0;
    }

    // Size of the pointed-to element (for pointer arithmetic scaling)
    int pointeeSize() const {
        if (pointer_depth == 0) return 0;
        TypeInfo pointee = {base, pointer_depth - 1};
        return pointee.size();
    }

    bool operator==(const TypeInfo& o) const {
        return base == o.base && pointer_depth == o.pointer_depth;
    }
    bool operator!=(const TypeInfo& o) const { return !(*this == o); }

    std::string toString() const {
        std::string s;
        switch (base) {
            case Int:  s = "int"; break;
            case Char: s = "char"; break;
            case Void: s = "void"; break;
        }
        for (int i = 0; i < pointer_depth; ++i) s += "*";
        return s;
    }

    // Check if two types are assignment-compatible
    bool isCompatibleWith(const TypeInfo& o) const {
        if (*this == o) return true;
        // int and char are interchangeable
        if (isIntegral() && o.isIntegral()) return true;
        // pointer types must match exactly
        return false;
    }
};

} // namespace minic
