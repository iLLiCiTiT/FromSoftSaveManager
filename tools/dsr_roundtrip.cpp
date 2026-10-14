// Parses every slot of a Dark Souls Remastered .sl2 with the full slot parser and
// checks that re-serializing reproduces the decrypted slot byte for byte.
//
// Usage: DSRRoundTrip <file.sl2> [more.sl2 ...]

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "../src/parse/SL2File.h"
#include "../src/parse/DSR/SaveData.h"

using namespace fssm::parse;

static size_t first_diff(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return i;
    }
    return n;
}

static bool check_file(const std::string& path) {
    std::printf("%s\n", path.c_str());
    SL2File sl2 = parse_sl2_file(path);
    if (sl2.game != fssm::Game::DSR) {
        std::printf("  not a DSR save, skipped\n");
        return true;
    }
    bool ok = true;
    for (size_t i = 0; i < sl2.entries.size(); ++i) {
        const auto& entry = sl2.entries[i];
        const auto& content = entry.content;
        try {
            std::vector<uint8_t> out;
            std::string info;
            if (i == 10) {
                const auto slot = dsr::parse_system_slot(content);
                out = dsr::serialize_system_slot(slot);
                info = "system";
            } else if (dsr::is_empty_slot(content)) {
                std::printf("  %s empty\n", entry.name.c_str());
                continue;
            } else {
                const auto slot = dsr::parse_character_slot(content);
                out = dsr::serialize_character_slot(slot);
                std::u16string name;
                for (char16_t c : slot.player.stats.name) {
                    if (c == 0) break;
                    name.push_back(c);
                }
                info = "level " + std::to_string(slot.player.stats.level)
                    + ", " + std::to_string(slot.player.unk_618.size()) + " vector entries"
                    + ", " + std::to_string(slot.netChunks.chunks.size()) + " net chunks"
                    + ", " + std::to_string(slot.trailingGarbage.size()) + " trailing bytes";
            }
            if (out == content) {
                std::printf("  %s OK (%s)\n", entry.name.c_str(), info.c_str());
            } else {
                ok = false;
                std::printf("  %s MISMATCH at 0x%zx (%s)\n",
                            entry.name.c_str(), first_diff(out, content), info.c_str());
            }
        } catch (const std::exception& e) {
            ok = false;
            std::printf("  %s ERROR %s\n", entry.name.c_str(), e.what());
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
    std::printf(ok ? "all slots round-trip exactly\n" : "FAILED\n");
    return ok ? 0 : 1;
}
