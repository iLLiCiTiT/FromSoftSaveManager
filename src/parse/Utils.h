#pragma once
#include <cstdint>
#include <string>

namespace fssm::parse {
    uint32_t read_u32_le(const uint8_t* p);
    uint64_t read_u64_le(const uint8_t* p);
    std::string utf16_to_utf8(const std::u16string& s);
}
