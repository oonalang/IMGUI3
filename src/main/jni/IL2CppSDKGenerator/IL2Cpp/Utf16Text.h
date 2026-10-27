#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace il2cpp_text {
constexpr std::size_t kMaxCodeUnits = 1024 * 1024;

inline std::string ToUtf8(const uint16_t* source, std::size_t length) {
    std::string out;
    if (!source || length > kMaxCodeUnits) return out;
    out.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        uint32_t cp = source[i];
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (i + 1 < length && source[i + 1] >= 0xDC00 && source[i + 1] <= 0xDFFF) {
                const uint32_t low = source[++i];
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
            } else {
                cp = 0xFFFD;
            }
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            cp = 0xFFFD;
        }
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

// Returned text remains valid until the next conversion on this thread.
inline const char* CString(const uint16_t* source, std::size_t length) {
    thread_local std::string text;
    text = ToUtf8(source, length);
    return text.c_str();
}
} // namespace il2cpp_text
