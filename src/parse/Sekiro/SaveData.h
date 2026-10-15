#pragma once
// Exact on-disk layout of Sekiro saves (S0000.sl2).
//
// Recovered from sekiro.exe (Steam build, SteamStub removed with Steamless for
// analysis). Every USER_DATA entry is a list of records written by the game's
// NS_SPRJ::SaveDataBase subclasses (registered in FUN_140a94570): a 0x20 byte
// header followed by `size` bytes of payload, terminated by an _SaveDataEnd
// record. Bytes after the end record are whatever the game's buffer held, so
// they are kept as an opaque tail to make `serialize(parse(x)) == x` hold.
// Entries are not encrypted (only MD5 protected, handled by SL2File).

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace fssm::parse::sekiro {
    // Value of RecordHeader::id.
    enum class RecordId : int32_t {
        End = -1,
        // System entry (USER_DATA010)
        Option = 0,
        MenuSystem = 1,
        ProfileSummary = 2,
        PcOption = 3,
        PcKeyConfig = 4,
        // Character entries (USER_DATA000..009)
        Event = 1000,               // event flags, variable size
        GameDataMan = 1001,
        PlayerGameData = 1002,
        Gaitem = 1003,              // item instances
        EquipThrowSkillData = 1004,
        EquipMagicData = 1005,
        EquipHistory = 1006,
        AutoReplenishData = 1007,
        GestureEquipData = 1008,
        FaceData = 1009,
        GestureGameData = 1010,
        TrophyEquipData = 1011,
        StartPointInfo = 1012,
        ChrInitState = 1013,
        ObjInitState = 1014,
        DecalInitState = 1015,
        Network = 1016,
        RematchEvent = 1018,
        // USER_DATA011
        Ghost = 10000,
    };

#pragma pack(push, 1)
    struct RecordHeader {
        uint32_t flags;         // 0xFFFFFFFF on some records, 0 on others
        int32_t id;
        uint32_t version;
        uint32_t size;
        std::array<uint32_t, 4> reserved;
    };
    static_assert(sizeof(RecordHeader) == 0x20);
#pragma pack(pop)

    struct Record {
        RecordHeader header;
        std::vector<uint8_t> data;      // header.size bytes

        RecordId id() const { return static_cast<RecordId>(header.id); }
    };

    // One USER_DATA entry.
    struct SaveEntry {
        bool empty = false;             // all zero (unused character slot)
        std::vector<Record> records;    // last one is RecordId::End
        std::vector<uint8_t> trailing;  // stale buffer bytes after the End record

        const Record* find(RecordId id) const;
    };

    // Throws std::runtime_error when the data is not a record list.
    SaveEntry parse_entry(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_entry(const SaveEntry& entry);

    constexpr size_t CHARACTER_SLOTS = 10;
    constexpr size_t SYSTEM_ENTRY_INDEX = 10;
    constexpr size_t GHOST_ENTRY_INDEX = 11;
}
