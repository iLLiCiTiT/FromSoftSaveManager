// Parses every slot of a Dark Souls III .sl2 with the full slot parser and
// checks that re-serializing reproduces the decrypted slot byte for byte, and
// that the stored character checksum matches.
//
// Usage: DS3RoundTrip <file.sl2> [more.sl2 ...]

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "../src/parse/SL2File.h"
#include "../src/parse/DS3/SaveData.h"

using namespace fssm::parse;

static size_t first_diff(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return i;
    }
    return n;
}

static std::string utf16_to_ascii(const char16_t* s, size_t n) {
    std::string out;
    for (size_t i = 0; i < n && s[i]; ++i) out.push_back(s[i] < 0x80 ? char(s[i]) : '?');
    return out;
}

static bool check_file(const std::string& path) {
    std::printf("%s\n", path.c_str());
    SL2File sl2 = parse_sl2_file(path);
    if (sl2.game != fssm::Game::DS3) {
        std::printf("  not a DS3 save, skipped\n");
        return true;
    }
    bool ok = true;
    for (size_t i = 0; i < sl2.entries.size() && i < 11; ++i) {
        const auto& entry = sl2.entries[i];
        const auto& content = entry.content;
        try {
            std::vector<uint8_t> out;
            std::string info;
            bool checksumOk = true;
            if (i == 10) {
                const auto slot = ds3::parse_system_slot(content);
                out = ds3::serialize_system_slot(slot);
                int used = 0;
                for (auto o : slot.profiles.occupied) used += o != 0;
                info = "system, " + std::to_string(used) + " profiles, "
                    + std::to_string(slot.trailingGarbage.size()) + " trailing bytes";
            } else if (ds3::is_empty_slot(content)) {
                std::printf("  %s empty\n", entry.name.c_str());
                continue;
            } else {
                const auto slot = ds3::parse_character_slot(content);
                out = ds3::serialize_character_slot(slot);
                const auto& st = slot.player.stats;
                size_t items = 0;
                for (const auto& e : slot.player.equip.inventory.list.normal) items += e.quantity != 0;
                checksumOk = ds3::compute_character_checksum(out, slot.header) == slot.header.checksum;
                info = utf16_to_ascii(st.name.data(), st.name.size()) + ", level " + std::to_string(st.level)
                    + ", " + std::to_string(items) + " inventory entries"
                    + ", checksum " + (checksumOk ? "OK" : "BAD")
                    + ", " + std::to_string(slot.trailingGarbage.size()) + " trailing bytes";
            }
            if (out == content && checksumOk) {
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
