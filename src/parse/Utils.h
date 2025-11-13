#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <codecvt>
#include <cstring>
#include <iostream>
#include <locale>
#include <ostream>

namespace fssm::parse {
    uint8_t read_u8_le(const uint8_t* p);
    uint32_t read_u32_le(const uint8_t* p);
    uint64_t read_u64_le(const uint8_t* p);
    std::string utf16_to_utf8(const std::u16string& s);
    std::u16string parse_name(const std::vector<uint8_t>& name_b);

    enum class Endianness { Little, Big };
    constexpr uint16_t bswap16(uint16_t v) {
        return static_cast<uint16_t>(((v & 0x00FFu) << 8) | ((v & 0xFF00u) >> 8));
    }
    constexpr uint32_t bswap32(uint32_t v) {
        return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
               ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
    }
    constexpr uint64_t bswap64(uint64_t v) {
        return ((v & 0x00000000000000FFull) << 56) |
               ((v & 0x000000000000FF00ull) << 40) |
               ((v & 0x0000000000FF0000ull) << 24) |
               ((v & 0x00000000FF000000ull) << 8)  |
               ((v & 0x000000FF00000000ull) >> 8)  |
               ((v & 0x0000FF0000000000ull) >> 24) |
               ((v & 0x00FF000000000000ull) >> 40) |
               ((v & 0xFF00000000000000ull) >> 56);
    }
    template <class T>
    T byteswap_if_needed(T v, Endianness srcEndian) {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        const bool hostIsLittle = false;
#else
        const bool hostIsLittle = true;  // Windows/MinGW
#endif
        if (
            (srcEndian == Endianness::Little && !hostIsLittle)
            || (srcEndian == Endianness::Big && hostIsLittle)
        ) {
            if constexpr (sizeof(T) == 2)      return static_cast<T>(bswap16(static_cast<uint16_t>(v)));
            else if constexpr (sizeof(T) == 4) return static_cast<T>(bswap32(static_cast<uint32_t>(v)));
            else if constexpr (sizeof(T) == 8) return static_cast<T>(bswap64(static_cast<uint64_t>(v)));
        }
        return v;
    }
    class ContentReader {
    public:
        explicit ContentReader(const std::vector<uint8_t>& content)
        : m_data(content.data()), m_size(content.size()), m_pos(0) {}
        template <class T>
        T read(Endianness e) {
            T v;
            copyTo(&v, sizeof(T));
            return byteswap_if_needed<T>(v, e);
        }
        uint8_t  read_u8_le() { return read<uint8_t>(Endianness::Little); };
        uint16_t read_u16_le() { return read<uint16_t>(Endianness::Little); }
        uint32_t read_u32_le() { return read<uint32_t>(Endianness::Little); }
        uint64_t read_u64_le() { return read<uint64_t>(Endianness::Little); }
        int32_t  read_i32_le() { return static_cast<int32_t>(read<uint32_t>(Endianness::Little)); }

        uint16_t read_u16_be() { return read<uint16_t>(Endianness::Big); }
        uint32_t read_u32_be() { return read<uint32_t>(Endianness::Big); }
        uint64_t read_u64_be() { return read<uint64_t>(Endianness::Big); }
        int32_t  read_i32_be() { return static_cast<int32_t>(read<uint32_t>(Endianness::Big)); }

        std::vector<uint8_t> read_vec_u8(size_t size) {
            std::vector<uint8_t> v;
            v.reserve(size);
            v.assign(m_data + m_pos, m_data + m_pos + size);
            m_pos += size;
            return v;
        }

        std::u16string read_u16_string(size_t size) {
            std::u16string s;
            for (size_t i = 0; i < size; ++i) {
                char16_t ch = m_data[m_pos + (2 * i)] | static_cast<char16_t>(m_data[m_pos + (2 * i) + 1]) << 8;
                if (ch == 0) break;
                s.push_back(ch);
            }
            m_pos += (2 * size);
            return s;
        }
        void copyTo(void* dest, size_t n) {
            if (m_pos + n > m_size) throw std::out_of_range("read past end");
            std::memcpy(dest, m_data + m_pos, n);
            m_pos += n;
        }
        void skip(int n) { m_pos += n; }

    private:
        const uint8_t* m_data;
        size_t m_size;
        size_t m_pos;
    };
}
