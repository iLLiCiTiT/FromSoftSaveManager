#include "SL2File.h"

#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <locale>

#include "aes.hpp"

#include "Game.h"
#include "Utils.h"


static const unsigned char DSR_KEY[16] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
static const unsigned char DS2_KEY[16] = {0x59,0x9f,0x9b,0x69,0x96,0x40,0xa5,0x52,0x36,0xee,0x2d,0x70,0x83,0x5e,0xc7,0x44};
static const unsigned char DS3_KEY[16] = {0xfd,0x46,0x4d,0x69,0x5e,0x69,0xa3,0x9a,0x10,0xe3,0x19,0xa7,0xac,0xe8,0xb7,0xfa};

namespace fssm::parse {
static Game detect_game(const BND4Header& header, const std::vector<uint8_t>& content) {
    Game game = Game::Unknown;
    if (header.files_count == 11) {
        game = Game::DSR;
    } else if (header.files_count == 23) {
        game = Game::DS2_SOTFS;
    } else if (header.files_count == 12) {
        if (content.size() < 64 + 32) return Game::Unknown;
        const uint8_t* hs = content.data() + 64;
        (void)read_u64_le(hs + 0); // padding
        uint64_t entry_size = read_u64_le(hs + 8);
        if (entry_size == 786480) {
            game = Game::DS3;
        } else if (entry_size == 1048592) {
            game = Game::Sekiro;
        } else {
            game = Game::ER;
        }
    }
    return game;
}


static std::vector<uint8_t> decrypt_entry(
    std::vector<uint8_t>& entry_content,
    const unsigned char* key
) {
    std::vector<uint8_t> iv;
    iv.assign(entry_content.begin(), entry_content.begin() + 16);

    uint8_t* buf = entry_content.data();
    AES_ctx ctx; AES_init_ctx_iv(&ctx, key, iv.data());
    AES_CBC_decrypt_buffer(&ctx, buf, entry_content.size());

    // Result buffer layout now: [0:16]=out_iv, [16:20]=len, [20:]=data (possibly padded)
    const uint8_t* dec = entry_content.data();
    int len = read_u32_le(dec + 16);
    entry_content.erase(entry_content.begin(), entry_content.begin() + 20);
    entry_content.resize(len);
    return entry_content;
}

std::vector<uint8_t> decrypt_entry_content(
    const std::vector<uint8_t>& content,
    const BND4EntryHeader& header,
    const Game game
) {
    uint32_t ds = header.entry_data_offset;
    uint32_t de = ds + header.entry_size;
    std::vector<uint8_t> checksum;
    std::vector<uint8_t> entry_content;
    checksum.reserve(16);
    checksum.assign(content.begin() + ds, content.begin() + ds + 16);
    entry_content.assign(content.begin() + ds + 16, content.begin() + de);
    switch (game) {
        case Game::DSR: return decrypt_entry(entry_content, DSR_KEY);
        case Game::DS2_SOTFS: return decrypt_entry(entry_content, DS2_KEY);
        case Game::DS3: return decrypt_entry(entry_content, DS3_KEY);
        default: return entry_content;
    }
}

SL2File parse_sl2_file(const std::string& input_sl2_file) {
    std::ifstream f(input_sl2_file, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file");
    std::vector<uint8_t> content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    if (content.size() < 64) {
        throw std::runtime_error("File too small to be a valid BND4 container");
    }

    const uint8_t* data = content.data();

    // Check magic "BND4"
    if (!(data[0] == 'B' && data[1] == 'N' && data[2] == 'D' && data[3] == '4')) {
        throw std::runtime_error("Expected header 'BND4'");
    }

    // Unpack <QIQQQQ? from bytes [4:49)
    BND4Header header{};
    std::memcpy(header.bnd_vers.data(), data + 0, 4);
    header.unknown_1 = read_u64_le(data + 4);
    header.files_count = read_u32_le(data + 12);
    header.unknown_2 = read_u64_le(data + 16);
    header.sig = read_u64_le(data + 24);
    header.entry_header_size = read_u64_le(data + 32);
    header.data_offset = read_u64_le(data + 40);
    header.is_utf16 = (*(data + 48)) != 0;
    if (content.size() >= 64) {
        std::memcpy(header.unknown_3.data(), data + 49, 15);
    }

    SL2File sl2;
    sl2.header = header;
    sl2.game = detect_game(header, content);
    sl2.filepath = input_sl2_file;

    // Collect entry headers and basic info
    sl2.entries.reserve(header.files_count);
    const bool utf16 = header.is_utf16;
    for (uint32_t idx = 0; idx < header.files_count; ++idx) {
        int hs = 64 + static_cast<int>(idx) * 32;
        if (static_cast<int>(content.size()) < hs + 32) break;
        const uint8_t* hp = data + hs;
        BND4EntryHeader eh{};
        eh.padding = read_u64_le(hp + 0);
        eh.entry_size = read_u64_le(hp + 8);
        eh.entry_data_offset = read_u32_le(hp + 16);
        eh.entry_name_offset = read_u32_le(hp + 20);
        eh.entry_footer_length = read_u64_le(hp + 24);

        int ns = static_cast<int>(eh.entry_name_offset);
        std::vector<uint8_t> name_b;
        // TODO use u16 string all the time
        std::string name;
        name_b.assign(content.begin() + ns, content.begin() + ns + 26);
        if (utf16) {
            std::u16string name_u16;
            for (size_t i = 0; i + 1 < name_b.size(); i += 2) {
                char16_t ch = static_cast<char16_t>(name_b[i] | (static_cast<uint16_t>(name_b[i + 1]) << 8));
                if (ch == 0) break;
                name_u16.push_back(ch);
            }
            name = utf16_to_utf8(name_u16);
        } else {
            for (char c: name_b) {
                if (c == 0) break;
                name.push_back(c);
            }
        }

        // Copy raw content of the entry (still encrypted for some games)
        std::vector<uint8_t> entry_content = decrypt_entry_content(content, eh, sl2.game);
        sl2.entries.push_back(BND4Entry{eh, std::move(name_b), std::move(name), std::move(entry_content)});
    }

    return sl2;
}
}
