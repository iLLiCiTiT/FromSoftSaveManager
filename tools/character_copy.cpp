// Copies a character between .sl2 files with fssm::parse::copy_character and verifies
// the result: MD5s, untouched entries byte for byte, the copied slot against the source,
// a full round-trip of every slot through the SaveData models and the game's parser.
//
// Usage: CharacterCopy list <file.sl2>
//        CharacterCopy copy <src.sl2> <srcSlot> <dst.sl2> <dstSlot> <out.sl2>

#include <cstdio>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

#include "../src/parse/CharacterCopy.h"
#include "../src/parse/SL2File.h"
#include "../src/parse/Utils.h"
#include "../src/parse/DSR/SaveData.h"
#include "../src/parse/DSR/SaveFile.h"
#include "../src/parse/DS3/SaveData.h"
#include "../src/parse/DS3/SaveFile.h"
#include "../src/parse/Sekiro/SaveData.h"
#include "../src/parse/Sekiro/SaveFile.h"
#include "../src/parse/EldenRing/SaveFile.h"

using namespace fssm::parse;
using fssm::Game;

static void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
    if (!f) throw std::runtime_error("failed to write " + path);
}

static int list(const std::string& path) {
    const CopyFileInfo info = read_copy_file_info(path);
    std::printf("%s: %s, steam id %llu\n", path.c_str(), info.game.toString(), (unsigned long long)info.steamId);
    for (const auto& slot : info.characters) {
        if (!slot.occupied) {
            std::printf("  %d empty\n", slot.index);
            continue;
        }
        std::printf("  %d %s level %u, %u:%02u\n", slot.index, slot.name.c_str(), slot.level,
                    slot.playTime / 3600, slot.playTime / 60 % 60);
    }
    return 0;
}

// Every slot (and the system entry) must still round-trip through the typed models.
static bool round_trip_all(const SL2File& sl2) {
    bool ok = true;
    for (size_t i = 0; i <= 10; ++i) {
        const auto& content = sl2.entries[i].content;
        std::vector<uint8_t> out;
        switch (sl2.game) {
            case Game::DSR:
                if (i == 10) out = dsr::serialize_system_slot(dsr::parse_system_slot(content));
                else out = dsr::is_empty_slot(content) ? content : dsr::serialize_character_slot(dsr::parse_character_slot(content));
                break;
            case Game::DS3:
                if (i == 10) out = ds3::serialize_system_slot(ds3::parse_system_slot(content));
                else if (ds3::is_empty_slot(content)) out = content;
                else {
                    const auto slot = ds3::parse_character_slot(content);
                    out = ds3::serialize_character_slot(slot);
                    if (ds3::compute_character_checksum(out, slot.header) != slot.header.checksum) {
                        std::printf("  %s checksum BAD\n", sl2.entries[i].name.c_str());
                        ok = false;
                    }
                }
                break;
            case Game::Sekiro:
                out = sekiro::serialize_entry(sekiro::parse_entry(content));
                break;
            default:
                out = content;     // no ER model, checked by parse_er_file below
        }
        if (out != content) {
            std::printf("  %s does not round-trip\n", sl2.entries[i].name.c_str());
            ok = false;
        }
    }
    return ok;
}

// Characters the app's own parser finds (what the UI lists), as "slot:name/level".
static std::string parsed_characters(const SL2File& sl2) {
    std::string out;
    const auto add = [&out](int index, const std::string& name, uint32_t level) {
        out += (out.empty() ? "" : ", ") + std::to_string(index) + ":" + name + "/" + std::to_string(level);
    };
    switch (sl2.game) {
        case Game::DSR:
            for (const auto& c : dsr::parse_dsr_file(sl2).characters) add(c.index, utf16_to_utf8(c.name), c.level);
            break;
        case Game::DS3:
            for (const auto& c : ds3::parse_ds3_file(sl2).characters) add(c.index, utf16_to_utf8(c.name), c.level);
            break;
        case Game::Sekiro:
            for (const auto& c : sekiro::parse_sekiro_file(sl2).characters) add(c.index, "Wolf", 0);
            break;
        case Game::ER:
            for (const auto& c : er::parse_er_file(sl2).characters) add(c.index, utf16_to_utf8(c.name), c.level);
            break;
        default:
            break;
    }
    return out;
}

static int copy(const std::string& srcPath, int srcSlot, const std::string& dstPath, int dstSlot, const std::string& outPath) {
    const auto bytes = copy_character(srcPath, srcSlot, dstPath, dstSlot);
    write_file(outPath, bytes);

    bool ok = true;
    const auto check = [&ok](bool cond, const char* what) {
        std::printf("  %-48s %s\n", what, cond ? "OK" : "FAILED");
        ok = ok && cond;
    };
    const SL2File src = parse_sl2_file(srcPath);
    const SL2File dst = parse_sl2_file(dstPath);
    const SL2File out = parse_sl2_file(outPath);
    const auto dstBytes = read_sl2_bytes(dstPath);
    std::printf("%s slot %d -> %s slot %d => %s\n", srcPath.c_str(), srcSlot, dstPath.c_str(), dstSlot, outPath.c_str());

    try { check_entry_md5s(outPath); check(true, "all entry MD5s"); }
    catch (const std::exception& e) { std::printf("  %s\n", e.what()); check(false, "all entry MD5s"); }
    check(out.game == dst.game && out.entries.size() == dst.entries.size() && bytes.size() == dstBytes.size(),
          "same game, entry count and file size");

    // Everything outside the two rewritten entries must be byte-identical.
    std::vector<bool> changed(bytes.size(), false);
    for (size_t i : {size_t(dstSlot), size_t(10)}) {
        const auto& h = dst.entries[i].header;
        std::fill(changed.begin() + h.entry_data_offset, changed.begin() + h.entry_data_offset + h.entry_size, true);
    }
    size_t outside = 0;
    for (size_t i = 0; i < bytes.size(); ++i) outside += !changed[i] && bytes[i] != dstBytes[i];
    check(outside == 0, "bytes outside the two entries unchanged");

    // The copied slot differs from the source only where the Steam ID / checksum live.
    const auto& a = src.entries[srcSlot].content;
    const auto& b = out.entries[dstSlot].content;
    size_t diff = 0;
    std::string where;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
        if (a[i] != b[i]) {
            if (diff < 12) where += " 0x" + [&] { char s[16]; std::snprintf(s, sizeof s, "%zx", i); return std::string(s); }();
            ++diff;
        }
    }
    std::printf("  copied slot differs from source in %zu bytes:%s\n", diff, where.c_str());
    check(a.size() == b.size() && diff <= 12, "copied slot equals source (except Steam ID/checksum)");

    check(round_trip_all(out), "every slot round-trips");
    const CopyFileInfo srcInfo = read_copy_file_info(srcPath);
    const CopyFileInfo outInfo = read_copy_file_info(outPath);
    const CopyFileInfo dstInfo = read_copy_file_info(dstPath);
    check(outInfo.steamId == dstInfo.steamId, "destination keeps its Steam ID");
    const auto& s = srcInfo.characters[srcSlot];
    const auto& o = outInfo.characters[dstSlot];
    check(o.occupied && o.name == s.name && o.level == s.level && o.playTime == s.playTime,
          "Load Game summary of the slot copied");
    for (int i = 0; i < 10; ++i) {
        if (i == dstSlot) continue;
        const auto& x = dstInfo.characters[i];
        const auto& y = outInfo.characters[i];
        if (x.occupied != y.occupied || x.name != y.name || x.level != y.level) {
            check(false, "other slots unchanged");
            break;
        }
    }
    std::printf("  game parser before: %s\n", parsed_characters(dst).c_str());
    std::printf("  game parser after:  %s\n", parsed_characters(out).c_str());
    std::printf(ok ? "copy verified\n" : "FAILED\n");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    try {
        const std::string cmd = argc > 1 ? argv[1] : "";
        if (cmd == "list" && argc == 3) return list(argv[2]);
        if (cmd == "copy" && argc == 7) return copy(argv[2], std::stoi(argv[3]), argv[4], std::stoi(argv[5]), argv[6]);
    } catch (const std::exception& e) {
        std::printf("ERROR %s\n", e.what());
        return 1;
    }
    std::printf("usage: %s list <file.sl2>\n"
                "       %s copy <src.sl2> <srcSlot> <dst.sl2> <dstSlot> <out.sl2>\n", argv[0], argv[0]);
    return 2;
}
