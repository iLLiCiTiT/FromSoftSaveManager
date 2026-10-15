#pragma once
// Exact on-disk layout of Dark Souls III save slots (DS30000.sl2).
//
// Recovered from DarkSoulsIII.exe (Steam build). Class names come from the
// game's RTTI (NS_SPRJ::*), field order from the game's own Serialize virtuals
// (IGameDataElem vtable slot 1) and the slot writers:
//   character slot  FUN_14062af10 (header, then GameDataMan FUN_140598b20, ...)
//   system slot     FUN_140599190
// Fields whose meaning is not known are kept as `unk_<offset>` so that every
// byte is accounted for and `serialize(parse(x)) == x` holds bit for bit.
//
// Offsets in comments are relative to the start of the struct as written to
// the save (not the in-memory object) unless noted otherwise.

#include <array>
#include <cstdint>
#include <vector>

namespace fssm::parse::ds3 {
#pragma pack(push, 1)

    // ---------------------------------------------------------------- Gaitem table

    // One socket of a weapon/armor gaitem (SprjGaitemIns+0x18, 5 entries).
    struct GaitemGemSlot {
        uint32_t gemId;             // ctor: 0x80000000
        uint32_t gemHandle;         // gaitem handle, mapped through the gaitem table
    };
    static_assert(sizeof(GaitemGemSlot) == 8);

    // Payload of SprjWepGaitemIns / SprjProGaitemIns.
    struct GaitemEquipPayload {
        uint32_t unk_00;            // durability
        uint32_t unk_04;
        uint32_t unk_08;            // ctor: 1
        std::array<GaitemGemSlot, 5> gems;
    };
    static_assert(sizeof(GaitemEquipPayload) == 0x34);

    // Payload of SprjGemGaitemIns (item id type 0x8).
    struct GaitemGemPayload {
        uint32_t unk_00;
        uint32_t unk_04;
        std::array<uint8_t, 0x18> unk_08;
    };
    static_assert(sizeof(GaitemGemPayload) == 0x20);

    // Which payload follows handle + itemId. Empty slots are {0, 0xffffffff}.
    enum class GaitemKind : uint8_t { None, Equip, Gem };
    GaitemKind gaitem_kind(uint32_t itemId);

    struct GaitemEntry {
        uint32_t handle = 0;
        uint32_t itemId = 0xffffffff;
        GaitemEquipPayload equip{};     // GaitemKind::Equip only
        GaitemGemPayload gem{};         // GaitemKind::Gem only
    };

    constexpr size_t GAITEM_COUNT = 0x1800;

    // ---------------------------------------------------------------- PlayerGameData

    // PlayerGameData+0x10, written raw (0x140 bytes).
    struct PlayerGameDataStats {
        std::array<uint32_t, 2> unk_00;
        uint32_t hp;
        uint32_t hpMax;
        uint32_t hpBase;
        uint32_t fp;
        uint32_t fpMax;
        uint32_t fpBase;
        uint32_t unk_20;
        uint32_t stamina;
        uint32_t staminaMax;
        uint32_t staminaBase;
        uint32_t unk_30;
        uint32_t vigor;
        uint32_t attunement;
        uint32_t endurance;
        uint32_t strength;
        uint32_t dexterity;
        uint32_t intelligence;
        uint32_t faith;
        uint32_t luck;
        uint32_t unk_54;
        uint32_t unk_58;
        uint32_t vitality;
        uint32_t level;
        uint32_t souls;
        uint32_t earnedSouls;
        std::array<uint8_t, 0xc> unk_6c;
        std::array<char16_t, 17> name;          // +0x078
        std::array<uint8_t, 0x36> unk_09a;
        uint32_t toxicResistance;               // +0x0d0
        uint32_t bleedResistance;
        uint32_t poisonResistance;
        uint32_t curseResistance;
        uint32_t frostResistance;
        std::array<uint8_t, 10> unk_0e4;
        uint8_t hollowing;                      // +0x0ee
        std::array<uint8_t, 3> unk_0ef;
        uint8_t estusMax;                       // +0x0f2
        uint8_t ashenEstusMax;
        std::array<uint8_t, 0x4c> unk_0f4;
    };
    static_assert(sizeof(PlayerGameDataStats) == 0x140);

    // PlayerGameData+0x150, written raw: 11 entries of 16 bytes (ctor inits id = -1).
    struct PlayerGameDataUnkEntry {
        int32_t id;
        uint32_t unk_04;
        uint64_t unk_08;
    };
    static_assert(sizeof(PlayerGameDataUnkEntry) == 0x10);

    // EquipGameData+0x88 (non-virtual serializer FUN_1408695e0).
    struct ChrAsm {
        uint32_t unk_00;
        std::array<uint8_t, 0x18> unk_04;
        std::array<uint32_t, 0x16> equipItemIds;    // ChrAsm+0x7c
        std::array<uint32_t, 0x16> equipHandles;    // ChrAsm+0x24, mapped through the gaitem table
        uint32_t unk_e8;
    };
    static_assert(sizeof(ChrAsm) == 0xd0);

    // Inventory record (FUN_140a32940).
    struct InventoryEntry {
        uint32_t handle;            // gaitem handle (weapons/armor) or item id
        uint32_t itemId;
        uint32_t quantity;
        uint32_t unk_0c;            // sort / acquisition index
    };
    static_assert(sizeof(InventoryEntry) == 0x10);

    // EquipInventoryData+0x10 (FUN_140a337d0): three fixed-capacity lists.
    struct InventoryList {
        uint32_t unk_00;
        std::vector<InventoryEntry> normal;     // INVENTORY_NORMAL_CAPACITY
        uint32_t unk_20;
        std::vector<InventoryEntry> key;        // INVENTORY_KEY_CAPACITY
        uint32_t unk_30;
        std::vector<InventoryEntry> unk;        // INVENTORY_KEY_CAPACITY
    };

    constexpr size_t INVENTORY_NORMAL_CAPACITY = 0x780;
    constexpr size_t INVENTORY_KEY_CAPACITY = 0x80;

    // NS_SPRJ::EquipInventoryData (inventory and storage box).
    struct EquipInventoryData {
        InventoryList list;
        uint32_t unk_88;
    };

    // NS_SPRJ::EquipMagicData
    struct EquipMagicData {
        std::array<uint8_t, 0x70> unk_18;
        uint32_t unk_88;
    };
    static_assert(sizeof(EquipMagicData) == 0x74);

    // NS_SPRJ::EquipItemData (quick slots, belt).
    struct EquipItemData {
        std::array<uint8_t, 0x50> unk_18;
        uint32_t unk_68;
        std::array<uint8_t, 0x28> unk_6c;
        std::array<uint8_t, 8> unk_94;
    };
    static_assert(sizeof(EquipItemData) == 0x84);

    // NS_SPRJ::GestureEquipData
    struct GestureEquipData {
        std::array<uint8_t, 0x20> unk_10;
    };
    static_assert(sizeof(GestureEquipData) == 0x20);

    // Element of the list at EquipGameData+0x308 (FUN_14057a780). The game copies
    // {u32, u8} into an 8 byte stack slot, so the last 3 bytes are stack garbage.
    struct EquipGameDataUnkEntry {
        uint32_t unk_00;
        uint8_t unk_04;
        std::array<uint8_t, 3> garbage;
    };
    static_assert(sizeof(EquipGameDataUnkEntry) == 8);

    // NS_SPRJ::EquipGameData (PlayerGameData+0x228)
    struct EquipGameData {
        std::array<uint32_t, 0x16> equipInventoryIndices;   // +0x24
        ChrAsm chrAsm;
        EquipInventoryData inventory;                       // +0x1a8
        EquipMagicData magic;                               // *(+0x248)
        EquipItemData items;                                // +0x250
        GestureEquipData gestures;                          // *(+0x300)
        std::vector<EquipGameDataUnkEntry> unk_308;         // u32 count + entries
        std::array<uint32_t, 0x25> unk_32c;
    };

    // NS_SPRJ::FaceData
    struct FaceData {
        uint32_t unk_10;
        std::array<uint8_t, 0xf4> data;     // starts with "FACE"
    };
    static_assert(sizeof(FaceData) == 0xf8);

    // NS_SPRJ::GestureGameData
    struct GestureGameData {
        std::array<uint8_t, 0xa4> unk_10;
    };
    static_assert(sizeof(GestureGameData) == 0xa4);

    // NS_SPRJ::PlayerGameData (FUN_1405a7b70). The gaitem table (SprjGaitemImp,
    // FUN_140a2c310) is written from inside PlayerGameData's serializer.
    struct PlayerGameData {
        std::vector<GaitemEntry> gaitems;           // GAITEM_COUNT
        PlayerGameDataStats stats;
        std::array<PlayerGameDataUnkEntry, 11> unk_150;
        EquipGameData equip;
        FaceData face;
        EquipInventoryData storage;                 // *(+0x7b0), storage box
        GestureGameData gestures;                   // *(+0x7b8)
        std::vector<uint32_t> unk_810;              // u32 count + entries
    };

    // NS_SPRJ::TrophyEquipData
    struct TrophyEquipData {
        uint32_t unk_14;
        std::array<uint8_t, 0x800> unk_20;
        uint32_t unk_18;
        uint32_t unk_1c;
        std::array<uint8_t, 0x3c0> unk_820;
        std::array<uint8_t, 0x10> unk_be0;
        std::array<uint8_t, 0x10> unk_bf0;
        std::array<uint8_t, 0x10> unk_c00;
    };
    static_assert(sizeof(TrophyEquipData) == 0xbfc);

    // GameDataMan+0x40 / +0x50
    struct GameDataManBlob44 {
        std::array<uint8_t, 0x44> data;
    };

    // Scalars written by GameDataMan after TrophyEquipData.
    struct GameDataManScalars {
        uint32_t unk_84;
        uint32_t unk_88;
        uint32_t unk_8c;
        uint8_t unk_90;
        uint8_t unk_80;
        uint8_t unk_81;
        uint8_t unk_82;
        uint32_t unk_94;
        uint32_t unk_98;
        uint32_t unk_9c;
        uint32_t unk_computed;      // FUN_140627430
        uint8_t unk_flag;           // !func_140627af0
        uint32_t unk_computed2;     // FUN_1409f6200
        uint32_t unk_computed3;     // func_140626eb0
    };
    static_assert(sizeof(GameDataManScalars) == 0x29);

    // Section 4 (FrpgSaveLoadMan +0xb40/+0xaa0/+0xab0).
    struct SaveLoadManBlock {
        std::array<uint8_t, 0x30> unk_b40;
        std::array<uint8_t, 0x10> unk_aa0;
        std::array<uint8_t, 0x10> unk_ab0;
    };
    static_assert(sizeof(SaveLoadManBlock) == 0x50);

    // Section 6 (versions > 0x36).
    struct CharacterSlotTrailer {
        uint8_t unk_a1;                 // GameDataMan+0xa1
        uint32_t unk_a8;                // GameDataMan+0xa8
        uint8_t unk_ac9;                // FrpgSaveLoadMan+0xac9, version > 0x34
        uint32_t unk_acc;
        uint32_t unk_ad4;
        uint8_t unk_bc2;                // version > 0x36
        std::array<uint8_t, 0xc> unk_dc;        // GameDataMan+0xdc, version > 0x35
        std::array<uint16_t, 0x20> unk_c6e;     // FrpgSaveLoadMan+0xc6e (FUN_14062b6d0)
    };
    static_assert(sizeof(CharacterSlotTrailer) == 0x5b);

    // Written after the last section; not covered by the header's offsets but
    // part of the checksum.
    struct CharacterSlotEnd {
        std::array<uint8_t, 0x10> unk_e8;   // GameDataMan+0xe8
        uint64_t steamId;                   // FUN_140ef4050
        std::array<uint8_t, 0x32> unk_10;   // global +0x10
    };
    static_assert(sizeof(CharacterSlotEnd) == 0x4a);

    // 0x5c byte header of a character slot. Offset/size pairs are filled by the
    // game while writing.
    struct CharacterSlotHeader {
        uint32_t version;
        uint32_t mapId;                 // 0x28000000 when unknown
        uint32_t unk_08;                // GameDataMan+0xa4
        uint32_t gameDataOffset;
        uint32_t gameDataSize;
        uint32_t playerGameDataOffset;
        uint32_t playerGameDataSize;
        uint32_t checksumOffset;        // always 0x5c
        uint32_t checksum;              // adler32 * 2 of [0x5c, end of CharacterSlotEnd)
        uint32_t blobs44Offset;
        uint32_t blobs44Size;
        uint32_t unk2u32Offset;
        uint32_t unk2u32Size;
        uint32_t eventFlagsOffset;
        uint32_t eventFlagsSize;
        uint32_t mapStateOffset;
        uint32_t mapStateSize;
        uint32_t saveLoadOffset;
        uint32_t saveLoadSize;
        uint32_t netChunksOffset;
        uint32_t netChunksSize;
        uint32_t trailerOffset;
        uint32_t trailerSize;
    };
    static_assert(sizeof(CharacterSlotHeader) == 0x5c);

#pragma pack(pop)

    // One USER_DATA000..009 entry (0xc0000 bytes, decrypted).
    struct CharacterSlot {
        CharacterSlotHeader header;
        std::array<uint32_t, 4> random;     // 4 random u32 written after the header
        // Section 1: GameDataMan
        PlayerGameData player;
        uint8_t hasBlob44a = 1;             // always written as 1
        GameDataManBlob44 blob44a;
        uint32_t unk_48;
        uint8_t hasBlob44b = 1;
        GameDataManBlob44 blob44b;
        uint32_t unk_78;
        uint32_t unk_7c;
        TrophyEquipData trophy;
        GameDataManScalars scalars;
        // Section 2: event flags (raw bitfield) + 1 byte
        std::vector<uint8_t> eventFlags;
        uint8_t eventFlagsUnk;
        // Section 3: map state, three u32 length-prefixed blobs
        // (" RHC" chr bits, " JBO" obj bits, world state).
        std::array<std::vector<uint8_t>, 3> mapState;
        // Section 4
        SaveLoadManBlock saveLoad;
        // Section 5: u32 (always 2) + 0x20000 byte FrpgNetDataChunk buffer
        uint32_t netChunksVersion;
        std::vector<uint8_t> netChunks;
        // Section 6
        CharacterSlotTrailer trailer;
        CharacterSlotEnd end;
        // Stale heap memory the game leaves after the data.
        std::vector<uint8_t> trailingGarbage;
    };

#pragma pack(push, 1)
    // ProfileSummary entry (FUN_1405acf50), shown on the load game screen.
    struct ProfileSummaryEntry {
        std::array<char16_t, 17> name;
        uint32_t level;
        uint32_t unk_28;
        uint32_t souls;
        uint32_t mapId;
        uint32_t unk_34;
        FaceData face;
        std::array<uint8_t, 0xf0> chrAsm;   // FUN_1408696e0
        std::array<uint8_t, 4> unk_240;
        uint16_t unk_244;
        uint8_t unk_246;
        uint8_t unk_247;
        uint32_t unk_248;
    };
    static_assert(sizeof(ProfileSummaryEntry) == 0x22a);

    // NS_SPRJ::ProfileSummary
    struct ProfileSummary {
        std::array<uint8_t, 10> occupied;
        std::array<ProfileSummaryEntry, 10> profiles;
    };
    static_assert(sizeof(ProfileSummary) == 0x15ae);

    // NS_SPRJ::PcOptionData
    struct PcOptionData {
        std::array<uint8_t, 0x1a> unk_10;
        std::array<uint8_t, 0xa0> unk_2a;
    };
    static_assert(sizeof(PcOptionData) == 0xba);
#pragma pack(pop)

    // USER_DATA010 (0x60000 bytes, decrypted).
    struct SystemSlot {
        uint32_t version;
        uint64_t steamId;
        std::array<uint8_t, 0x80> menu;         // GameDataMan+0x58 (FUN_1405a0900)
        // GameDataMan+0x60 (CSMenuSystemSaveLoad, FUN_140755910): 8 byte header + chunk buffer
        uint16_t menuChunksUnk0;
        uint16_t menuChunksUnk1;
        std::vector<uint8_t> menuChunks;
        ProfileSummary profiles;
        uint32_t unk_cc;                        // GameDataMan+0xcc
        uint8_t unk_flag;
        PcOptionData options;
        std::array<uint8_t, 900> unk_384;       // global +0x384 (FUN_14057d730)
        std::array<uint8_t, 8> unk_710;
        std::vector<uint8_t> trailingGarbage;
    };

    constexpr size_t CHARACTER_SLOT_SIZE = 0xc0000;
    constexpr size_t SYSTEM_SLOT_SIZE = 0x60000;
    constexpr uint32_t CHARACTER_SLOT_VERSION = 0x62;

    // Throws std::runtime_error on data that doesn't match the known layout.
    CharacterSlot parse_character_slot(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_character_slot(const CharacterSlot& slot);

    SystemSlot parse_system_slot(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_system_slot(const SystemSlot& slot);

    // The checksum the game stores in CharacterSlotHeader::checksum, computed
    // over a serialized slot.
    uint32_t compute_character_checksum(const std::vector<uint8_t>& data, const CharacterSlotHeader& header);

    // True when the slot is unused (all zero).
    bool is_empty_slot(const std::vector<uint8_t>& data);
}
