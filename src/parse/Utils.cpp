#include <codecvt>
#include <cstdint>
#include <cstring>
#include <locale>
#include <string>

#include "Utils.h"

namespace fssm::parse {
    uint8_t read_u8_le(const uint8_t* p) {
        uint8_t v;
        std::memcpy(&v, p, sizeof(v));
        return v;
    }

    uint32_t read_u32_le(const uint8_t* p) {
        uint32_t v;
        std::memcpy(&v, p, sizeof(v));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        v = ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
#endif
        return v;
    }

    uint64_t read_u64_le(const uint8_t* p) {
        uint64_t v;
        std::memcpy(&v, p, sizeof(v));
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        v = ((v & 0x00000000000000FFull) << 56) |
            ((v & 0x000000000000FF00ull) << 40) |
            ((v & 0x0000000000FF0000ull) << 24) |
            ((v & 0x00000000FF000000ull) << 8)  |
            ((v & 0x000000FF00000000ull) >> 8)  |
            ((v & 0x0000FF0000000000ull) >> 24) |
            ((v & 0x00FF000000000000ull) >> 40) |
            ((v & 0xFF00000000000000ull) >> 56);
#endif
        return v;
    }

    std::string utf16_to_utf8(const std::u16string& s)
    {
        // Note: deprecated in C++17, removed in some lib std builds; still widely available.
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
        return conv.to_bytes(s);
    }

    std::u16string parse_name(const std::vector<uint8_t>& name_b) {
        std::u16string name;
        for (size_t i = 0; i + 1 < name_b.size(); i += 2) {
            char16_t ch = static_cast<char16_t>(name_b[i] | (static_cast<char16_t>(name_b[i + 1]) << 8));
            if (ch == 0) break;
            name.push_back(ch);
        }
        return name;
    }
}
