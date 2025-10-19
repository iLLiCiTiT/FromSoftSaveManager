#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>

namespace fsm::parse {

enum class Game {
    DSR,
    DS2_SOTFS,
    DS3,
    ER,
    Sekiro,
    Unknown
};

struct BND4Header {
    // Only fields we currently need
    std::array<uint8_t, 4> bnd_vers;           // 'BND4' as little-endian integer
    uint64_t unknown_1{};
    uint32_t files_count{};
    uint64_t unknown_2{};
    uint64_t sig{};
    uint64_t entry_header_size{};
    uint64_t data_offset{};
    bool is_utf16{false};
    std::array<uint8_t, 15> unknown_3{};    // trailing bytes (49..63)
};

struct BND4EntryHeader {
    uint64_t padding{};
    uint64_t entry_size{};
    uint32_t entry_data_offset{}; // lower 32 bits of offset
    uint32_t entry_name_offset{};
    uint64_t entry_footer_length{};
};

struct BND4Entry {
    BND4EntryHeader header;
    std::vector<uint8_t> name_b;
    std::string name;
    std::vector<uint8_t> content;  // raw content (still encrypted for some games)
};

struct SL2File {
    Game game{Game::Unknown};
    std::string filepath;
    BND4Header header;
    std::vector<BND4Entry> entries;
};

// Game-specific character/result structures

struct DS3CharacterInfo {
    int index{-1};
    std::u16string name;
    int level{-1};
};

struct ERCharacter {
    int index{-1};
    int ver{0};
    int level{-1};
    std::u16string name;
};

struct ParsedFile {
    Game game{Game::Unknown};
    SL2File sl2; // retain container and entries
    // Separated per-game structures
    std::vector<DSRCharacterInfo> dsr_characters;
    std::vector<DS3CharacterInfo> ds3_characters;
    std::vector<ERCharacter> er_characters;
};

// Parse the .sl2 container and detect the game. Does not decrypt inner files yet.
SL2File parse_sl2_file(const std::string& input_sl2_file);

// Per-game parse functions (currently minimal wrappers, to be expanded later)
ParsedFile parse_dsr_file(const SL2File& sl2);
ParsedFile parse_ds2_file(const SL2File& sl2);
ParsedFile parse_ds3_file(const SL2File& sl2);
ParsedFile parse_er_file(const SL2File& sl2);
ParsedFile parse_sekiro_file(const SL2File& sl2);

// Dispatch by detected game
ParsedFile parse_save_file(const std::string& filepath);

} // namespace fsm::parse
