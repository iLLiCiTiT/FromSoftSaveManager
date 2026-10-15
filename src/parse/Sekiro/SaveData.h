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

#pragma pack(push, 1)
    // One slot of the item instance table (GaitemMan, FUN_140c364d0).
    struct GaitemEntry {
        uint32_t handle;            // 0 when free
        uint32_t itemId;            // 0xFFFFFFFF when free
        std::array<uint32_t, 13> data;
    };
    static_assert(sizeof(GaitemEntry) == 60);

    struct InventoryEntry {
        uint32_t sortKey;
        uint32_t handle;            // 0 / 0xFFFFFFFF when empty
        uint32_t itemId;            // type nibble << 28 | param id (0x4... goods, 0x0... weapons)
        uint32_t quantity;

        bool empty() const { return handle == 0 || handle == 0xFFFFFFFFu || itemId == 0xFFFFFFFFu; }
    };
    static_assert(sizeof(InventoryEntry) == 16);

    // Item container (FUN_1407a8120 + FUN_140c3d920): normal items, key items and
    // a third list sharing the key item count.
    struct ItemList {
        uint32_t unk_00;
        uint32_t capacity;          // 500 for the player inventory
        uint32_t normalCount;
        uint32_t keyCount;
        std::array<InventoryEntry, 0x700> normalItems;
        std::array<InventoryEntry, 0x100> keyItems;
        std::array<InventoryEntry, 0x100> unk_items;
    };
    static_assert(sizeof(ItemList) == 0x9010);

    // EquipInventoryData of the player (PlayerGameData+0x278, FUN_14079e940).
    struct PlayerInventory {
        std::array<uint32_t, 25> unk_00;
        std::array<uint32_t, 33> unk_64;    // FUN_1409cc6b0
        ItemList items;
        std::array<uint32_t, 33> unk_90f8;  // FUN_1407abbf0
    };
    static_assert(sizeof(PlayerInventory) == 0x917c);

    // Payload of RecordId::Gaitem.
    struct GaitemData {
        std::array<GaitemEntry, 0x1800> gaitems;
        PlayerInventory inventory;
        ItemList storage;           // PlayerGameData+0x858
    };
    static_assert(sizeof(GaitemData) == 0x6c18c);
#pragma pack(pop)

    // Typed view of a character entry's Gaitem record, if present.
    std::optional<GaitemData> read_gaitem(const SaveEntry& entry);

    // Throws std::runtime_error when the data is not a record list.
    SaveEntry parse_entry(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_entry(const SaveEntry& entry);

    constexpr size_t CHARACTER_SLOTS = 10;
    constexpr size_t SYSTEM_ENTRY_INDEX = 10;
    constexpr size_t GHOST_ENTRY_INDEX = 11;
}
