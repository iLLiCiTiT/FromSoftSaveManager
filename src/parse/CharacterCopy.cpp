#include "CharacterCopy.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

#include "aes.hpp"

#include "SL2File.h"
#include "Utils.h"
#include "DSR/SaveData.h"
#include "DS3/SaveData.h"
#include "Sekiro/SaveData.h"

namespace fssm::parse {
namespace {
    constexpr int CHARACTER_SLOTS = 10;
    constexpr size_t SYSTEM_ENTRY = 10;    // USER_DATA010 in every supported game

    void ensure(bool cond, const std::string& msg) {
        if (!cond) throw std::runtime_error(msg);
    }

    // ---------------------------------------------------------------- MD5 (RFC 1321)

    std::array<uint8_t, 16> md5(const uint8_t* data, size_t size) {
        static constexpr uint32_t K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
        };
        static constexpr uint8_t R[64] = {
            7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
            5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
            4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
            6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
        };
        uint32_t h[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
        const auto block = [&h](const uint8_t* p) {
            uint32_t m[16];
            for (int i = 0; i < 16; ++i) m[i] = read_u32_le(p + i * 4);
            uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
            for (int i = 0; i < 64; ++i) {
                uint32_t f;
                int g;
                if (i < 16) { f = (b & c) | (~b & d); g = i; }
                else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) % 16; }
                else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
                else { f = c ^ (b | ~d); g = (7 * i) % 16; }
                const uint32_t t = d;
                d = c;
                c = b;
                const uint32_t x = a + f + K[i] + m[g];
                b = b + ((x << R[i]) | (x >> (32 - R[i])));
                a = t;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        };
        size_t full = size / 64 * 64;
        for (size_t i = 0; i < full; i += 64) block(data + i);
        uint8_t tail[128] = {};
        const size_t rest = size - full;
        std::memcpy(tail, data + full, rest);
        tail[rest] = 0x80;
        const size_t tailSize = rest < 56 ? 64 : 128;
        const uint64_t bits = uint64_t(size) * 8;
        for (int i = 0; i < 8; ++i) tail[tailSize - 8 + i] = uint8_t(bits >> (8 * i));
        for (size_t i = 0; i < tailSize; i += 64) block(tail + i);
        std::array<uint8_t, 16> out{};
        for (int i = 0; i < 16; ++i) out[i] = uint8_t(h[i / 4] >> (8 * (i % 4)));
        return out;
    }

    // ---------------------------------------------------------------- Container

    // Parsed (decrypted) entries plus the raw file bytes the changed entries are written into.
    struct RawFile {
        SL2File sl2;
        std::vector<uint8_t> bytes;
    };

    void check_md5s(const RawFile& f) {
        for (const auto& entry : f.sl2.entries) {
            const auto& h = entry.header;
            ensure(uint64_t(h.entry_data_offset) + h.entry_size <= f.bytes.size(),
                   "Save file changed while it was read, try again");
            const uint8_t* raw = f.bytes.data() + h.entry_data_offset;
            ensure(h.entry_size >= 16 && md5(raw + 16, h.entry_size - 16) == *reinterpret_cast<const std::array<uint8_t, 16>*>(raw),
                   "Entry " + entry.name + " has a bad MD5, the file is damaged or still being written");
        }
    }

    RawFile load(const std::string& path) {
        RawFile f{parse_sl2_file(path), read_sl2_bytes(path)};
        ensure(supports_character_copy(f.sl2.game), "Copying characters is not supported for this game");
        ensure(f.sl2.entries.size() == f.sl2.header.files_count && f.sl2.entries.size() > SYSTEM_ENTRY,
               "Save file has too few entries");
        check_md5s(f);
        return f;
    }

    // Writes `data` as the content of entry `index`, updating its MD5. The entry keeps its size;
    // encrypted entries keep their IV and padding.
    void write_entry(RawFile& f, size_t index, const std::vector<uint8_t>& data) {
        auto& entry = f.sl2.entries.at(index);
        ensure(data.size() == entry.content.size(), "Entry " + entry.name + " size mismatch");
        const size_t size = entry.header.entry_size;
        uint8_t* raw = f.bytes.data() + entry.header.entry_data_offset;
        if (const unsigned char* key = entry_aes_key(f.sl2.game)) {
            // md5 | iv | AES-128-CBC(u32 length | data | padding)
            ensure(size >= 32 && (size - 32) % 16 == 0 && 4 + data.size() <= size - 32,
                   "Entry " + entry.name + " has an unexpected encrypted size");
            std::vector<uint8_t> plain(raw + 32, raw + size);
            AES_ctx ctx;
            AES_init_ctx_iv(&ctx, key, raw + 16);
            AES_CBC_decrypt_buffer(&ctx, plain.data(), plain.size());
            ensure(read_u32_le(plain.data()) == data.size(), "Entry " + entry.name + " length mismatch");
            std::memcpy(plain.data() + 4, data.data(), data.size());
            AES_init_ctx_iv(&ctx, key, raw + 16);
            AES_CBC_encrypt_buffer(&ctx, plain.data(), plain.size());
            std::memcpy(raw + 32, plain.data(), plain.size());
        } else {
            // md5 | data
            ensure(size == 16 + data.size(), "Entry " + entry.name + " has an unexpected size");
            std::memcpy(raw + 16, data.data(), data.size());
        }
        const auto hash = md5(raw + 16, size - 16);
        std::memcpy(raw, hash.data(), hash.size());
        entry.content = data;
    }

    template <class T>
    T read_at(const std::vector<uint8_t>& data, size_t offset) {
        ensure(offset + sizeof(T) <= data.size(), "Read past the end of an entry");
        T v;
        std::memcpy(&v, data.data() + offset, sizeof(T));
        return v;
    }

    template <class T>
    void write_at(std::vector<uint8_t>& data, size_t offset, const T& v) {
        ensure(offset + sizeof(T) <= data.size(), "Write past the end of an entry");
        std::memcpy(data.data() + offset, &v, sizeof(T));
    }

    std::string name_utf8(const char16_t* name, size_t maxLen) {
        std::u16string s;
        for (size_t i = 0; i < maxLen && name[i] != 0; ++i) s.push_back(name[i]);
        return utf16_to_utf8(s);
    }

    // ---------------------------------------------------------------- Per game: system entry
    //
    // Each game keeps a Load Game summary per slot in its system entry: an occupied
    // byte array and fixed size summary records. Copying a slot copies both.

    struct SummaryLayout {
        size_t occupiedOffset;
        size_t entriesOffset;
        size_t entrySize;
    };

    // DSR / DS3: the typed models; returns the layout inside the serialized system slot.
    template <class SystemSlot, class Parse, class Serialize>
    SummaryLayout typed_summary_layout(const std::vector<uint8_t>& data, Parse parse, Serialize serialize) {
        const SystemSlot slot = parse(data);
        ensure(serialize(slot) == data, "System entry does not round-trip, refusing to rewrite it");
        // Locate ProfileSummary by its serialized bytes (it is written raw).
        const auto* p = reinterpret_cast<const uint8_t*>(&slot.profiles);
        const auto it = std::search(data.begin(), data.end(), p, p + sizeof(slot.profiles));
        ensure(it != data.end(), "ProfileSummary not found in the system entry");
        const size_t base = size_t(it - data.begin());
        const size_t entriesOffset = base + (reinterpret_cast<const uint8_t*>(&slot.profiles.profiles) - p);
        return {base, entriesOffset, sizeof(slot.profiles.profiles[0])};
    }

    // Sekiro: ProfileSummary record (FUN_1407c94a0): occupied[10], u8, u8, 10 x 0x218.
    constexpr size_t SEKIRO_SUMMARY_SIZE = 0x218;
    constexpr size_t SEKIRO_SUMMARY_ACTIVE = 0x28;      // u32, 1 when the slot is used
    constexpr size_t SEKIRO_SUMMARY_PLAY_TIME = 0x2c;

    size_t sekiro_record_offset(const sekiro::SaveEntry& entry, sekiro::RecordId id, size_t size) {
        size_t pos = 0;
        for (const auto& record : entry.records) {
            pos += sizeof(sekiro::RecordHeader);
            if (record.id() == id) {
                ensure(record.data.size() == size || size == 0, "Unexpected record size");
                return pos;
            }
            pos += record.data.size();
        }
        throw std::runtime_error("Record " + std::to_string(int(id)) + " not found");
    }

    sekiro::SaveEntry sekiro_parse_checked(const std::vector<uint8_t>& data) {
        sekiro::SaveEntry entry = sekiro::parse_entry(data);
        ensure(sekiro::serialize_entry(entry) == data, "Entry does not round-trip, refusing to rewrite it");
        return entry;
    }

    // Elden Ring USER_DATA010: version, steam id, 0x140 settings, MenuSystemSaveLoad
    // (u16, u16, u32 length, data), occupied[10], 10 x 0x24c summaries.
    constexpr size_t ER_SUMMARY_SIZE = 0x24c;

    SummaryLayout summary_layout(Game game, const std::vector<uint8_t>& system) {
        switch (game) {
            case Game::DSR:
                return typed_summary_layout<dsr::SystemSlot>(system, dsr::parse_system_slot, dsr::serialize_system_slot);
            case Game::DS3:
                return typed_summary_layout<ds3::SystemSlot>(system, ds3::parse_system_slot, ds3::serialize_system_slot);
            case Game::Sekiro: {
                const auto entry = sekiro_parse_checked(system);
                const size_t base = sekiro_record_offset(entry, sekiro::RecordId::ProfileSummary, 12 + 10 * SEKIRO_SUMMARY_SIZE);
                return {base, base + 12, SEKIRO_SUMMARY_SIZE};
            }
            case Game::ER: {
                const size_t menu = 4 + 8 + 0x140;
                const uint32_t length = read_at<uint32_t>(system, menu + 4);
                const size_t base = menu + 8 + length;
                ensure(base + 10 + 10 * ER_SUMMARY_SIZE <= system.size(), "Unexpected USER_DATA010 layout");
                return {base, base + 10, ER_SUMMARY_SIZE};
            }
            default:
                throw std::runtime_error("Copying characters is not supported for this game");
        }
    }

    // Steam ID the file belongs to, 0 when the game doesn't store one.
    uint64_t system_steam_id(Game game, const std::vector<uint8_t>& system) {
        switch (game) {
            case Game::DS3:
            case Game::ER:
                return read_at<uint64_t>(system, 4);    // version, steam id
            case Game::Sekiro: {
                // Option record: u32, steam id, ...
                const auto entry = sekiro_parse_checked(system);
                return read_at<uint64_t>(system, sekiro_record_offset(entry, sekiro::RecordId::Option, 0) + 4);
            }
            default:
                return 0;
        }
    }

    bool slot_occupied(Game game, const std::vector<uint8_t>& system, const SummaryLayout& layout, int slot) {
        if (game == Game::Sekiro) {
            return read_at<uint32_t>(system, layout.entriesOffset + slot * layout.entrySize + SEKIRO_SUMMARY_ACTIVE) == 1;
        }
        return system.at(layout.occupiedOffset + slot) != 0;
    }

    // ---------------------------------------------------------------- Per game: character entry

    // Offset of the Steam ID in an Elden Ring character slot. Field order after the event
    // flags (ER-Save-Editor, Apache-2.0, src/save/common/save_slot.rs): ..., u32 (2 for used
    // slots), 0x20000 CSNetDataChunks, 0xc weather, 0xc time, 0x10 unknown, steam id. The
    // fields before it vary in size, so match the id and check the fixed fields around it.
    size_t er_slot_steam_id_offset(const std::vector<uint8_t>& slot, uint64_t steamId) {
        constexpr size_t BEFORE = 0x10 + 0xc + 0xc + 0x20000 + 4;
        const auto* p = reinterpret_cast<const uint8_t*>(&steamId);
        std::vector<size_t> found;
        for (auto it = slot.begin(); (it = std::search(it, slot.end(), p, p + 8)) != slot.end(); ++it) {
            const size_t off = size_t(it - slot.begin());
            if (off >= BEFORE && read_at<uint32_t>(slot, off - BEFORE) == 2) found.push_back(off);
        }
        ensure(found.size() == 1, "Steam ID not found in the Elden Ring character slot");
        return found[0];
    }

    // Character entry content to write, with the Steam ID changed from `srcSteamId` to `dstSteamId`.
    std::vector<uint8_t> character_for_destination(
        Game game, const std::vector<uint8_t>& src, uint64_t srcSteamId, uint64_t dstSteamId
    ) {
        switch (game) {
            case Game::DSR: {
                ensure(!dsr::is_empty_slot(src), "The source slot is empty");
                const auto slot = dsr::parse_character_slot(src);
                ensure(dsr::serialize_character_slot(slot) == src, "Source slot does not round-trip");
                return src;     // no Steam ID or checksum inside
            }
            case Game::DS3: {
                ensure(!ds3::is_empty_slot(src), "The source slot is empty");
                auto slot = ds3::parse_character_slot(src);
                ensure(ds3::serialize_character_slot(slot) == src, "Source slot does not round-trip");
                ensure(ds3::compute_character_checksum(src, slot.header) == slot.header.checksum,
                       "Source slot has a bad checksum");
                slot.end.steamId = dstSteamId;
                auto out = ds3::serialize_character_slot(slot);
                slot.header.checksum = ds3::compute_character_checksum(out, slot.header);
                out = ds3::serialize_character_slot(slot);
                ensure(ds3::compute_character_checksum(out, slot.header) == slot.header.checksum, "Checksum update failed");
                return out;
            }
            case Game::Sekiro: {
                const auto entry = sekiro_parse_checked(src);
                ensure(!entry.empty, "The source slot is empty");
                // GameDataMan record: u32, steam id, ...
                const size_t offset = sekiro_record_offset(entry, sekiro::RecordId::GameDataMan, 0) + 4;
                ensure(read_at<uint64_t>(src, offset) == srcSteamId,
                       "The source character's Steam ID doesn't match its save file");
                auto out = src;
                write_at(out, offset, dstSteamId);
                return out;
            }
            case Game::ER: {
                ensure(std::any_of(src.begin(), src.end(), [](uint8_t b) { return b != 0; }), "The source slot is empty");
                const size_t offset = er_slot_steam_id_offset(src, srcSteamId);
                auto out = src;
                write_at(out, offset, dstSteamId);
                return out;
            }
            default:
                throw std::runtime_error("Copying characters is not supported for this game");
        }
    }
}

void check_entry_md5s(const std::string& path) {
    const RawFile f{parse_sl2_file(path), read_sl2_bytes(path)};
    ensure(f.sl2.entries.size() == f.sl2.header.files_count, "Save file has too few entries");
    check_md5s(f);
}

bool supports_character_copy(Game game) {
    return game == Game::DSR || game == Game::DS3 || game == Game::Sekiro || game == Game::ER;
}

CopyFileInfo read_copy_file_info(const std::string& path) {
    const SL2File sl2 = parse_sl2_file(path);
    ensure(supports_character_copy(sl2.game), "Copying characters is not supported for this game");
    ensure(sl2.entries.size() > SYSTEM_ENTRY, "Save file has too few entries");
    const auto& system = sl2.entries[SYSTEM_ENTRY].content;
    const SummaryLayout layout = summary_layout(sl2.game, system);

    CopyFileInfo info;
    info.game = sl2.game;
    info.steamId = system_steam_id(sl2.game, system);
    for (int i = 0; i < CHARACTER_SLOTS; ++i) {
        CopySlotInfo slot;
        slot.index = i;
        slot.occupied = slot_occupied(sl2.game, system, layout, i);
        const size_t base = layout.entriesOffset + i * layout.entrySize;
        if (slot.occupied) {
            if (sl2.game == Game::Sekiro) {
                slot.playTime = read_at<uint32_t>(system, base + SEKIRO_SUMMARY_PLAY_TIME);
            } else {
                // DSR / DS3 / ER summaries start with name[17] (u16), DSR has a u16 after it.
                const size_t levelOffset = sl2.game == Game::DSR ? 0x24 : 0x22;
                std::array<char16_t, 17> name{};
                std::memcpy(name.data(), system.data() + base, sizeof(name));
                slot.name = name_utf8(name.data(), name.size());
                slot.level = read_at<uint32_t>(system, base + levelOffset);
                slot.playTime = read_at<uint32_t>(system, base + levelOffset + 4);
            }
        }
        info.characters.push_back(slot);
    }
    return info;
}

std::vector<uint8_t> copy_character(
    const std::string& srcPath, int srcSlot,
    const std::string& dstPath, int dstSlot
) {
    ensure(srcSlot >= 0 && srcSlot < CHARACTER_SLOTS && dstSlot >= 0 && dstSlot < CHARACTER_SLOTS, "Invalid slot");
    const RawFile src = load(srcPath);
    RawFile dst = load(dstPath);
    const Game game = dst.sl2.game;
    ensure(src.sl2.game == game, "Both files must be saves of the same game");
    ensure(srcPath != dstPath || srcSlot != dstSlot, "Source and destination are the same slot");

    const auto& srcSystem = src.sl2.entries[SYSTEM_ENTRY].content;
    const SummaryLayout srcLayout = summary_layout(game, srcSystem);
    ensure(slot_occupied(game, srcSystem, srcLayout, srcSlot), "The source slot is empty");

    std::vector<uint8_t> dstSystem = dst.sl2.entries[SYSTEM_ENTRY].content;
    const SummaryLayout dstLayout = summary_layout(game, dstSystem);
    ensure(srcLayout.entrySize == dstLayout.entrySize, "System entries have different layouts");

    const uint64_t srcSteamId = system_steam_id(game, srcSystem);
    const uint64_t dstSteamId = system_steam_id(game, dstSystem);
    const auto character = character_for_destination(
        game, src.sl2.entries.at(srcSlot).content, srcSteamId, dstSteamId
    );

    dstSystem[dstLayout.occupiedOffset + dstSlot] = srcSystem[srcLayout.occupiedOffset + srcSlot];
    std::memcpy(dstSystem.data() + dstLayout.entriesOffset + dstSlot * dstLayout.entrySize,
                srcSystem.data() + srcLayout.entriesOffset + srcSlot * srcLayout.entrySize,
                dstLayout.entrySize);

    write_entry(dst, size_t(dstSlot), character);
    write_entry(dst, SYSTEM_ENTRY, dstSystem);
    return dst.bytes;
}
}
