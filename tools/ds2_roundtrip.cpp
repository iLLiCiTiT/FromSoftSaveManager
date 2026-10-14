// Parses every record list entry of a Dark Souls II: SotFS .sl2 and checks that
// re-serializing reproduces the decrypted entry byte for byte.
//
// Usage: DS2RoundTrip <file.sl2> [more.sl2 ...]

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "../src/parse/SL2File.h"
#include "../src/parse/Utils.h"
#include "../src/parse/DS2/SaveData.h"

using namespace fssm::parse;

static std::string name_of(const char16_t* name, size_t n) {
    std::u16string s;
    for (size_t i = 0; i < n && name[i] != 0; ++i) s.push_back(name[i]);
    return utf16_to_utf8(s);
}

static bool check_file(const std::string& path) {
    std::printf("%s\n", path.c_str());
    SL2File sl2 = parse_sl2_file(path);
    if (sl2.game != fssm::Game::DS2_SOTFS) {
        std::printf("  not a DS2 save, skipped\n");
        return true;
    }
    bool ok = true;
    for (size_t i = 0; i < sl2.entries.size(); ++i) {
        const auto& entry = sl2.entries[i];
        if (i == ds2::REGULATION_ENTRY_INDEX) {
            std::printf("  %s regulation, skipped\n", entry.name.c_str());
            continue;
        }
        try {
            const auto parsed = ds2::parse_entry(entry.content);
            const auto out = ds2::serialize_entry(parsed);
            std::string types;
            for (const auto& r : parsed.records) types += std::to_string(r.header.type) + " ";
            std::printf("  %s %s (records: %s)\n", entry.name.c_str(),
                        out == entry.content ? "OK" : "MISMATCH", types.c_str());
            if (out != entry.content) ok = false;
        } catch (const std::exception& e) {
            ok = false;
            std::printf("  %s ERROR %s\n", entry.name.c_str(), e.what());
        }
    }

    // Show what the typed views decode.
    const auto system = ds2::parse_entry(sl2.entries[ds2::SYSTEM_ENTRY_INDEX].content);
    if (auto info = ds2::read_load_info(system)) {
        for (size_t slot = 0; slot < ds2::CHARACTER_SLOTS; ++slot) {
            const auto& s = info->slots[slot];
            if ((s.flags & 1) == 0) continue;
            const auto character = ds2::parse_entry(sl2.entries[ds2::CHARACTER_ENTRY_OFFSET + slot].content);
            const auto player = ds2::read_player(character);
            const auto game = ds2::read_game(character);
            std::printf("  slot %zu: %s level %u, souls %u, map %08x\n", slot,
                        name_of(s.name.data(), s.name.size()).c_str(), s.level,
                        player ? player->souls : 0, game ? static_cast<uint32_t>(game->mapId) : 0);
        }
    }
    return ok;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: %s <file.sl2> [more.sl2 ...]\n", argv[0]);
        return 2;
    }
    bool ok = true;
    for (int i = 1; i < argc; ++i) {
        try {
            ok = check_file(argv[i]) && ok;
        } catch (const std::exception& e) {
            std::printf("  ERROR %s\n", e.what());
            ok = false;
        }
    }
    std::printf(ok ? "all entries round-trip exactly\n" : "FAILED\n");
    return ok ? 0 : 1;
}
