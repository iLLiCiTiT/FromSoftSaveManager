#pragma once
// Exact on-disk layout of Dark Souls II: Scholar of the First Sin saves (DS2SOFS0000.sl2).
//
// Recovered from DarkSoulsII.exe (Steam build, 2025-11-02). Every USER_DATA entry
// (except the regulation blob in USER_DATA021) is a list of records written by the
// game's SaveDataBase subclasses (FUN_1402e5240): a 0x20 byte header followed by
// `size` bytes of payload, terminated by a _SaveDataEnd record whose payload is
// skipped by the writer (it holds whatever was in the buffer before).
//
// Record payloads are kept as raw bytes so `serialize(parse(x)) == x` always holds;
// typed views below decode the payloads whose layout is known.

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace fssm::parse::ds2 {
    // Value of RecordHeader::type. List index -> type id comes from the table at
    // DAT_1410da1f0, which swaps the last two (GetObjItem = 14, End = 13).
    enum class RecordType : uint32_t {
        Player = 0,         // SaveDataPlayer, 0x35c
        Game = 1,           // SaveDataGame, 0x470
        Generator = 2,      // SaveDataGenerator, 0x1d0a0
        Option = 3,         // SaveDataOption, 0x32c
        LoadInfo = 4,       // SaveDataLoadInfo / SaveDataLoadInfo2, 0x1370
        ShopLineup = 5,     // SaveDataShopLineup, 0xfa4
        Network = 6,        // SaveDataNetwork, 0x61c
        Enemy = 7,          // SaveDataEnemy, 0x9c30
        Event = 8,          // SaveDataEvent, 0x21c8
        Obj = 9,            // SaveDataObj, 0x31970
        ItemInventory = 10, // SaveDataItemInventory2, 0x100bc
        ItemPack = 12,      // SaveDataItemPack, 0x1394
        End = 13,           // _SaveDataEnd, padding size varies per entry kind
        GetObjItem = 14,    // SaveDataGetObjItem, 0x604
    };

#pragma pack(push, 1)
    struct RecordHeader {
        uint32_t type;
        uint32_t version;
        uint32_t size;
        std::array<uint32_t, 5> reserved;
    };
    static_assert(sizeof(RecordHeader) == 0x20);
#pragma pack(pop)

    struct Record {
        RecordHeader header;
        std::vector<uint8_t> data;      // header.size bytes

        RecordType type() const { return static_cast<RecordType>(header.type); }
    };

    // One decrypted USER_DATA entry.
    struct SaveEntry {
        std::vector<Record> records;    // last one is RecordType::End
        std::vector<uint8_t> trailing;  // bytes after the End record (normally none)

        const Record* find(RecordType type) const;
    };

    // Throws std::runtime_error when the data is not a record list.
    SaveEntry parse_entry(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_entry(const SaveEntry& entry);

    // USER_DATA021 is the encrypted regulation, not a record list.
    constexpr size_t REGULATION_ENTRY_INDEX = 21;
    constexpr size_t SYSTEM_ENTRY_INDEX = 0;            // Option + LoadInfo
    constexpr size_t CHARACTER_ENTRY_OFFSET = 1;        // USER_DATA001..010
    constexpr size_t CHARACTER_WORLD_ENTRY_OFFSET = 11; // USER_DATA011..020
    constexpr size_t LOAD_INFO2_ENTRY_INDEX = 22;
    constexpr size_t CHARACTER_SLOTS = 10;

    // ---------------------------------------------------------------- Typed views

#pragma pack(push, 1)
    // Character select entry inside SaveDataLoadInfo (FUN_14019c190).
    struct LoadInfoSlot {
        std::array<uint8_t, 0xa4> unk_000;
        std::array<uint8_t, 0xd0> appearance;       // +0x0a4
        // vigor, endurance, vitality, attunement, strength, dexterity,
        // intelligence, faith, adaptability, unk, unk
        std::array<uint16_t, 11> stats;             // +0x174
        std::array<char16_t, 33> name;              // +0x18a
        uint32_t playTime;                          // +0x1cc, seconds (capped at 999:59:59)
        int32_t mapId;                              // +0x1d0
        uint16_t level;                             // +0x1d4
        uint16_t unk_1d6;
        uint8_t unk_1d8;
        uint8_t flags;                              // +0x1d9, bit 0 = slot in use
        uint8_t unk_1da;
        uint8_t unk_1db;
        uint32_t unk_1dc;
        uint32_t unk_1e0;
        std::array<uint8_t, 0xc> unk_1e4;
    };
    static_assert(sizeof(LoadInfoSlot) == 0x1f0);

    struct LoadInfo {
        uint32_t currentSlot;
        uint32_t slotCount;                         // always 10
        std::array<uint8_t, 8> flags;
        std::array<LoadInfoSlot, CHARACTER_SLOTS> slotInfos;
    };
    static_assert(sizeof(LoadInfo) == 0x1370);

    // SaveDataPlayer payload. The writer builds it through an Arxan protected
    // function, so field names are inferred from save contents.
    struct PlayerData {
        // vigor, endurance, vitality, attunement, strength, dexterity,
        // intelligence, faith, adaptability, unk
        std::array<uint16_t, 10> stats;
        uint32_t unk_14;
        uint32_t level;
        uint32_t souls;
        uint32_t soulMemory;
        uint32_t unk_24;
        uint32_t hp;                                // matches HP on the Player Status screen
        std::array<uint8_t, 0x35c - 0x2c> unk_2c;
    };
    static_assert(sizeof(PlayerData) == 0x35c);

    // SaveDataGame payload (FUN_1402e3fc0).
    struct GameData {
        std::array<float, 4> position;              // x, y, z, w
        uint64_t unk_10;
        float unk_18;
        uint8_t unk_1c;
        uint8_t flags;                              // +0x1d
        std::array<uint8_t, 2> unk_1e;
        int32_t mapId;                              // +0x20
        std::array<char16_t, 32> name;              // +0x24
        uint32_t unk_64;
        uint32_t unk_68;
        std::array<uint8_t, 0x404> unk_6c;
    };
    static_assert(sizeof(GameData) == 0x470);

    struct EnemyEntry {
        std::array<uint32_t, 13> data;
    };

    // One of the three map blocks in SaveDataEnemy (FUN_1402e3530).
    struct EnemyMapBlock {
        int32_t mapId;
        uint16_t count;
        uint8_t unk_06;
        std::array<uint8_t, 9> unk_07;
        std::array<EnemyEntry, 256> entries;        // first `count` are used
    };
    static_assert(sizeof(EnemyMapBlock) == 0x3410);

    struct EnemyData {
        std::array<EnemyMapBlock, 3> maps;
    };
    static_assert(sizeof(EnemyData) == 0x9c30);

    // One slot of SaveDataItemInventory2 (ItemInventory2 +0x30).
    struct InventoryEntry {
        uint32_t itemId;            // 0 or 0xFFFFFFFF when empty
        uint32_t flags;
        // Goods, ammo and spells: count. Equipment: current durability (float bits).
        // Estus Flask: bytes {1, 0, charges, max charges}.
        uint32_t value;
        uint32_t reinforcement;     // low byte upgrade level, next byte Infusion

        bool empty() const { return itemId == 0 || itemId == 0xFFFFFFFFu; }
    };
    static_assert(sizeof(InventoryEntry) == 0x10);

    struct InventoryData {
        std::array<InventoryEntry, 0x1000> entries;
        std::array<uint8_t, 0xbc> unk_10000;
    };
    static_assert(sizeof(InventoryData) == 0x100bc);
#pragma pack(pop)

    // SaveDataObj payload (FUN_1402e5a10 + FUN_1401f2ea0): variable length.
    struct ObjData {
        std::vector<std::array<uint8_t, 0x6008>> maps;      // u32 count, max 3
        std::vector<std::array<uint8_t, 0xc08>> generators; // u32 count (42)
        std::vector<uint8_t> unwritten;                     // stale bytes up to 0x31970
    };

    std::optional<LoadInfo> read_load_info(const SaveEntry& entry);
    std::optional<PlayerData> read_player(const SaveEntry& entry);
    std::optional<GameData> read_game(const SaveEntry& entry);
    std::optional<EnemyData> read_enemy(const SaveEntry& entry);
    std::optional<InventoryData> read_inventory(const SaveEntry& entry);
    std::optional<ObjData> read_obj(const SaveEntry& entry);
}
