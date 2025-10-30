#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>

namespace fsm::parse {

class Game {
    public:
        enum Value: uint8_t {
            Unknown,
            DSR,
            DS2_SOTFS,
            DS3,
            ER,
            Sekiro,
        };

        Game() = default;
        constexpr Game(Value aGame) : value(aGame) {}

        // Allow switch and comparisons.
        constexpr operator Value() const {return value;}

        // Prevent usage: if(game)
        explicit operator bool() const = delete;

        constexpr bool operator==(Game a) const {return value == a.value;}
        constexpr bool operator==(Value a) const {return value == a;}
        constexpr bool operator!=(Game a) const {return value != a.value;}
        constexpr bool operator!=(Value a) const {return value != a;}

        constexpr const char* toString() const {
            switch (value) {
                case DSR: return "DSR";
                case DS2_SOTFS: return "DS2_SOTFS";
                case DS3: return "DS3";
                case ER: return "ER";
                case Sekiro: return "Sekiro";
                default: return "Unknown";
            }
        }

        static Game fromString(std::string_view gameName) {
            if (iequals(gameName, "dsr"))       return DSR;
            if (iequals(gameName, "ds3"))       return DS3;
            if (iequals(gameName, "er"))        return ER;
            if (iequals(gameName, "sekiro"))    return Sekiro;
            if (iequals(gameName, "ds2_sotfs")) return DS2_SOTFS;
            return Unknown;
        }

    private:
        Value value;
        static bool iequals(std::string_view a, std::string_view b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                unsigned char ca = static_cast<unsigned char>(a[i]);
                unsigned char cb = static_cast<unsigned char>(b[i]);
                if (std::tolower(ca) != cb) return false;
            }
            return true;
        }
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
    std::vector<DS3CharacterInfo> ds3_characters;
    std::vector<ERCharacter> er_characters;
};

// Parse the .sl2 container and detect the game. Does not decrypt inner files yet.
SL2File parse_sl2_file(const std::string& input_sl2_file);

// Per-game parse functions (currently minimal wrappers, to be expanded later)
ParsedFile parse_ds2_file(const SL2File& sl2);
ParsedFile parse_ds3_file(const SL2File& sl2);
ParsedFile parse_er_file(const SL2File& sl2);
ParsedFile parse_sekiro_file(const SL2File& sl2);

// Dispatch by detected game
ParsedFile parse_save_file(const std::string& filepath);

} // namespace fsm::parse
