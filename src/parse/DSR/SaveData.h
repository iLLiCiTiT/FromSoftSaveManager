#pragma once
// Exact on-disk layout of Dark Souls Remastered save slots (DRAKS0005.sl2).
//
// Recovered from DarkSoulsRemastered.exe (Steam build, 2025-02-03). Class names
// come from the game's RTTI (NS_FRPG::*), field order from the game's own
// Serialize/Deserialize virtuals (IGameDataElem vtable slots 1 and 2).
// Fields whose meaning is not known are kept as `unk_<offset>` so that every
// byte is accounted for and `serialize(parse(x)) == x` holds bit for bit.
//
// Offsets in comments are relative to the start of the struct as written to
// the save (not the in-memory object).

#include <array>
#include <cstdint>
#include <vector>

namespace fssm::parse::dsr {
#pragma pack(push, 1)

    // ---------------------------------------------------------------- PlayerGameData

    // PlayerGameData+0x10, written raw (0x1a8 bytes).
    struct PlayerGameDataStats {
        uint32_t unk_000;
        uint32_t hp;
        uint32_t hpMax;
        uint32_t hpBase;
        std::array<uint32_t, 4> unk_010;
        uint32_t stamina;
        uint32_t staminaMax;
        uint32_t staminaBase;
        uint32_t unk_02c;
        uint32_t vitality;
        uint32_t unk_034;
        uint32_t attunement;
        uint32_t unk_03c;
        uint32_t endurance;
        uint32_t unk_044;
        uint32_t strength;
        uint32_t unk_04c;
        uint32_t dexterity;
        uint32_t unk_054;
        uint32_t intelligence;
        uint32_t unk_05c;
        uint32_t faith;
        std::array<uint32_t, 4> unk_064;
        uint32_t humanity;
        uint32_t unk_078;
        uint32_t resistance;
        uint32_t level;
        uint32_t souls;
        uint32_t unk_088;
        uint32_t earnedSouls;
        uint32_t unk_090;
        uint32_t hollowState;
        std::array<char16_t, 17> name;          // +0x098
        uint8_t gender;                         // +0x0ba
        std::array<uint8_t, 3> unk_0bb;
        uint8_t classId;                        // +0x0be
        uint8_t physiqueId;
        uint8_t giftId;
        std::array<uint8_t, 3> unk_0c1;
        uint32_t multiplayerEncounters;         // +0x0c4
        uint32_t coopVictories;
        std::array<uint8_t, 14> unk_0cc;
        std::array<uint8_t, 10> covenantLevels; // +0x0da
        std::array<uint8_t, 12> unk_0e4;
        uint32_t toxicResistance;               // +0x0f0
        uint32_t bleedResistance;
        uint32_t poisonResistance;
        uint32_t curseResistance;
        std::array<uint8_t, 3> unk_100;
        uint8_t covenantId;                     // +0x103
        uint8_t faceId;
        uint8_t hairStyleId;
        uint8_t hairColorId;
        uint8_t cursed;
        std::array<uint8_t, 0xa0> unk_108;
    };
    static_assert(sizeof(PlayerGameDataStats) == 0x1a8);

    // PlayerGameData+0x1f0, written raw: 9 entries of 16 bytes (ctor inits id = -1).
    struct PlayerGameDataUnkEntry {
        int32_t id;
        uint32_t unk_04;
        uint64_t unk_08;
    };
    static_assert(sizeof(PlayerGameDataUnkEntry) == 0x10);

    // Equip slot order shared by EquipGameData::equipInventoryIndices and ChrAsm.
    enum class EquipSlot : uint8_t {
        LeftHand1, LeftHand2, RightHand1, RightHand2,
        LeftArrows, LeftBolts, RightArrows, RightBolts,
        Head, Body, Arms, Legs, Unknown12,
        LeftRing, RightRing,
        Quick1, Quick2, Quick3, Quick4, Quick5,
        Count
    };

    // EquipGameData+0x80 (non-virtual serializer FUN_14031be50).
    struct ChrAsm {
        uint32_t unk_00;
        std::array<uint32_t, 6> unk_04;
        // Item ids of equipped items (-1 when empty), EquipSlot order.
        uint32_t leftHand1;
        uint32_t leftHand2;
        uint32_t rightHand1;
        uint32_t rightHand2;
        uint32_t leftArrows;
        uint32_t leftBolts;
        uint32_t rightArrows;
        uint32_t rightBolts;
        uint32_t head;
        uint32_t body;
        uint32_t arms;
        uint32_t legs;
        uint32_t unk_4c;
        uint32_t leftRing;
        uint32_t rightRing;
        std::array<uint32_t, 5> quickItems;
    };
    static_assert(sizeof(ChrAsm) == 0x6c);

    struct EquipInventoryItem {
        uint32_t itemType;
        uint32_t itemId;
        uint32_t amount;
        uint32_t order;
        uint32_t unk_10;
        uint32_t durability;
        uint32_t unk_18;
    };
    static_assert(sizeof(EquipInventoryItem) == 0x1c);

    // NS_FRPG::EquipInventoryData (0x800 item slots for the local player).
    struct EquipInventoryData {
        uint32_t unk_40;                                // object +0x40
        uint32_t unk_44;                                // object +0x44
        uint32_t itemCount;                             // object +0x10
        std::array<EquipInventoryItem, 0x800> items;
        uint32_t unk_60;                                // object +0x60
    };
    static_assert(sizeof(EquipInventoryData) == 0xe010);

    struct AttunementSlot {
        uint32_t spellId;
        uint32_t remainingUses;
    };

    // NS_FRPG::EquipMagicData
    struct EquipMagicData {
        std::array<AttunementSlot, 12> attunement;
        uint32_t selectedSlot;
    };
    static_assert(sizeof(EquipMagicData) == 0x64);

    // NS_FRPG::EquipItemData
    struct EquipItemData {
        std::array<uint32_t, 5> quickSlots;
        uint32_t selected;
    };
    static_assert(sizeof(EquipItemData) == 0x18);

    // Gesture entries are packed: bit 0 = flag, bits 1..15 = id, high 16 bits = index.
    struct GestureEquipData {
        std::array<uint32_t, 8> gestures;
    };
    static_assert(sizeof(GestureEquipData) == 0x20);

    // NS_FRPG::EquipGameData
    struct EquipGameData {
        // Inventory indices of equipped items (-1 when empty), EquipSlot order.
        std::array<uint32_t, 20> equipInventoryIndices; // object +0x24
        ChrAsm chrAsm;                                  // object +0x80
        EquipInventoryData inventory;                   // object +0x120
        EquipMagicData magic;                           // *(object +0x198)
        EquipItemData items;                            // object +0x1a0
        GestureEquipData gestureEquip;                  // *(object +0x1d0)
    };
    static_assert(sizeof(EquipGameData) == 0xe168);

    // NS_FRPG::FaceData
    struct FaceData {
        uint32_t unk_00;
        std::array<uint8_t, 0x90> params;
    };
    static_assert(sizeof(FaceData) == 0x94);

    struct RepositoryItem {
        uint32_t itemId;        // includes item type in high bits, 0xFFFFFFFF when empty
        uint32_t order;
        uint16_t amount;
        uint16_t unk_0a;
        uint32_t durability;
        std::array<uint32_t, 4> unk_10;
    };
    static_assert(sizeof(RepositoryItem) == 0x20);

    // NS_FRPG::RepositoryData (bottomless box)
    struct RepositoryData {
        std::array<RepositoryItem, 0x800> items;
        uint32_t itemCount;
    };
    static_assert(sizeof(RepositoryData) == 0x10004);

    // NS_FRPG::GestureGameData
    struct GestureGameData {
        std::array<uint32_t, 16> gestures;
    };
    static_assert(sizeof(GestureGameData) == 0x40);

#pragma pack(pop)

    // NS_FRPG::PlayerGameData (variable size because of the trailing vector).
    struct PlayerGameData {
        PlayerGameDataStats stats;
        std::array<PlayerGameDataUnkEntry, 9> unk_1f0;
        EquipGameData equip;
        FaceData face;
        RepositoryData repository;
        GestureGameData gestures;
        std::vector<uint32_t> unk_618;  // u64 count + u32[count]
    };

#pragma pack(push, 1)

    // NS_FRPG::QwcData (world tendency)
    struct QwcData {
        std::array<uint32_t, 2> unk_00;
        std::array<uint32_t, 14> unk_08;
    };
    static_assert(sizeof(QwcData) == 0x40);

    struct TrophyEntry {
        int64_t unk_00;
        uint8_t unk_08;
        std::array<uint8_t, 3> unk_09;
        int32_t unk_0c;
    };
    static_assert(sizeof(TrophyEntry) == 0x10);

    struct TrophyEntrySmall {
        int32_t unk_00;
        std::array<uint8_t, 4> unk_04;
    };
    static_assert(sizeof(TrophyEntrySmall) == 8);

    // NS_FRPG::TrophyEquipData
    struct TrophyEquipData {
        uint32_t unk_14;
        std::array<TrophyEntry, 0x80> entries;
        uint32_t unk_18;
        std::array<TrophyEntrySmall, 0x78> smallEntries;
        std::array<uint8_t, 0x10> unk_bdc;
        std::array<uint8_t, 0x10> unk_bec;
    };
    static_assert(sizeof(TrophyEquipData) == 0xbe8);

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
        uint32_t unk_94;
        uint32_t unk_98;
        uint32_t unk_9c;
        uint32_t unk_computed;   // FUN_1402c9bd0
        uint8_t unk_flag;        // !FUN_1402cb180
        uint32_t unk_computed2;  // FUN_1402df980
    };
    static_assert(sizeof(GameDataManScalars) == 0x23);

    // Section 4 (FrpgSaveLoadMan +0xba0/+0xb10/+0xb20).
    struct SaveLoadManBlock {
        std::array<uint8_t, 0x30> unk_ba0;
        std::array<uint8_t, 0x10> unk_b10;
        std::array<uint8_t, 0x10> unk_b20;
    };
    static_assert(sizeof(SaveLoadManBlock) == 0x50);

    // 0x5c byte header of a character slot. Offset/size pairs are filled by the
    // game while writing; note sec4Size is (end of sec4 - start of sec3), a game bug.
    struct CharacterSlotHeader {
        uint32_t version;
        uint32_t gameVersion;
        uint32_t unk_08;            // GameDataMan+0xa4
        uint32_t gameDataOffset;
        uint32_t gameDataSize;
        uint32_t playerGameDataOffset;
        uint32_t playerGameDataSize;
        uint32_t trophyOffset;      // first QwcData offset, overwritten with TrophyEquipData offset
        uint32_t qwcSize;
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

    // Per-map bitfield entry inside the " RHC" (chr) / " JBO" (obj) blobs.
    struct MapBitsEntry {
        uint32_t mapId;
        uint32_t bitCount;
        std::vector<uint8_t> bits;      // ceil(bitCount / 8) bytes
    };

    // Length-prefixed blob: u32 size, then (if size != 0) 8 byte magic,
    // entries, and terminator {0xffffffff, 0, 0}.
    struct MapBitsBlob {
        bool present = false;
        uint64_t magic = 0;
        std::vector<MapBitsEntry> entries;
    };

    // FrpgNetDataChunk entry inside section 5.
    struct NetDataChunk {
        uint16_t id;
        uint16_t version;
        std::vector<uint8_t> data;
    };

    // Section 5: u32 (always 1) + a 0x20000 byte buffer of 4-aligned chunks
    // terminated by id 0xffff. The rest of the buffer is zero filled.
    struct NetDataChunks {
        uint32_t unk_00 = 1;
        std::vector<NetDataChunk> chunks;
        uint16_t endVersion = 0;
        uint32_t endSize = 0;
    };

    struct CharacterSlotTail {
        uint8_t unk_a1;                 // GameDataMan+0xa1
        uint32_t unk_a8;                // GameDataMan+0xa8
        // version > 0x34
        uint8_t unk_b32;
        uint32_t unk_b34;
        uint32_t unk_b38;
        // version > 0x36
        uint8_t unk_bf3;
        // version > 0x35
        std::array<uint8_t, 0xc> unk_d0;
        // version > 0x38
        uint8_t unk_flag;
    };

    // One USER_DATA000..009 entry (0x60000 bytes, decrypted).
    struct CharacterSlot {
        CharacterSlotHeader header;
        // Section 1: GameDataMan
        PlayerGameData player;
        QwcData qwc;
        bool hasBlob44a;
        GameDataManBlob44 blob44a;
        uint32_t unk_48;
        bool hasBlob44b;
        GameDataManBlob44 blob44b;
        uint32_t unk_78;
        uint32_t unk_7c;
        TrophyEquipData trophy;
        GameDataManScalars scalars;
        // Section 2: event flags (raw bitfield) + 1 byte
        std::vector<uint8_t> eventFlags;
        uint8_t eventFlagsUnk;
        // Section 3: map state
        MapBitsBlob chrBits;
        MapBitsBlob objBits;
        // Section 4
        SaveLoadManBlock saveLoad;
        // Section 5
        NetDataChunks netChunks;
        // Section 6
        std::array<uint8_t, 0x2c> unk_c5c;
        CharacterSlotTail tail;
        // Uninitialized heap memory the game leaves after the data.
        std::vector<uint8_t> trailingGarbage;
    };

#pragma pack(push, 1)
    // GameDataMan+0x58 (menu system / save-load settings), version >= 0x1e.
    struct MenuSystemData {
        std::array<uint8_t, 0xac> data;
    };

    // Shown on the "Data List" (load game) screen.
    struct ProfileSummaryEntry {
        std::array<char16_t, 17> name;
        uint16_t unk_22;
        uint32_t level;
        uint32_t playTime;                  // seconds
        uint32_t unk_2c;
        uint32_t mapId;                     // 0xAABB0000 for map mAA_BB
        uint32_t areaId;                    // PlaceName id (e.g. 1020 Firelink Shrine)
        std::array<uint8_t, 400 - 0x38> unk_38;
    };
    static_assert(sizeof(ProfileSummaryEntry) == 400);

    // NS_FRPG::ProfileSummary
    struct ProfileSummary {
        std::array<uint8_t, 10> occupied;
        std::array<uint8_t, 6> unk_0a;
        std::array<ProfileSummaryEntry, 10> profiles;
    };
    static_assert(sizeof(ProfileSummary) == 0xfb0);

    // NS_FRPG::PcOptionData
    struct PcOptionData {
        std::array<uint8_t, 0x12> unk_00;
        std::array<uint8_t, 0xa0> unk_12;
    };
    static_assert(sizeof(PcOptionData) == 0xb2);
#pragma pack(pop)

    // USER_DATA010 (0x60000 bytes, decrypted).
    struct SystemSlot {
        uint32_t version;
        MenuSystemData menu;
        ProfileSummary profiles;
        uint32_t unk_b0;                // GameDataMan+0xb0
        uint8_t unk_flag;
        PcOptionData options;
        std::vector<uint8_t> trailingGarbage;
    };

    constexpr size_t SLOT_SIZE = 0x60000;
    constexpr uint32_t CHARACTER_SLOT_VERSION = 0x47;
    constexpr uint32_t SYSTEM_SLOT_VERSION = 0x1e;

    // Throws std::runtime_error on data that doesn't match the known layout.
    CharacterSlot parse_character_slot(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_character_slot(const CharacterSlot& slot);

    SystemSlot parse_system_slot(const std::vector<uint8_t>& data);
    std::vector<uint8_t> serialize_system_slot(const SystemSlot& slot);

    // True when the slot is unused (all zero).
    bool is_empty_slot(const std::vector<uint8_t>& data);
}
