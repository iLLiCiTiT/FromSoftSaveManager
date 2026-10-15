#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../SL2File.h"
#include "Items.h"


namespace fssm::parse::dsr {
    // Layout of the decrypted USER_DATA entries of DRAKS0005.sl2.
    //
    // USER_DATA000-009 are character slots (0x60000 bytes each). A slot starts
    // with 'SlotHeader' which holds offsets and sizes of the sections that
    // follow it. Order of sections inside a slot:
    //
    //   PlayerGameData   - variable size (ends with a counted list)
    //   unkBlock0        - 0x40
    //   positionBlock    - 0x8E
    //   clearCountBlock  - 0x8
    //   unkBlock1        - 0xC0B
    //   eventFlags       - 0x16D01
    //   mapEntityState   - variable size ('CHR '/'OBJ ' chunks)
    //   warpBlock        - 0x50
    //   networkBlock     - 0x20004
    //   unkBlock2        - 0x2C
    //   trailing bytes   - unused rest of the 0x60000 buffer (stale data)
    //
    // Sections are packed without alignment, so most of them do not start on
    //   4 byte boundary.
    //
    // 'PlayerGameData' is serialized in the same order (and mostly the same
    //   layout) as the game keeps it in memory, minus pointers and vtables.
    //   Memory offsets (DSR 64-bit) relate to save offsets as follows:
    //   - AttributeInfo:  save offset = PlayerGameData + 0x4C
    //   - EquipGameData:  save offset = PlayerGameData - 0x24
    //
    // USER_DATA010 is the "system" entry (options, key config and load screen
    //   preview of every character), see 'SystemData'.
    //
    // Fields prefixed with 'unk' are not understood yet, they are kept to
    //   have every byte of the file represented.

    inline constexpr uint32_t SLOT_VERSION = 0x47;
    inline constexpr uint32_t SLOT_COUNT = 10;
    inline constexpr uint32_t INVENTORY_ITEMS_COUNT = 2048;
    inline constexpr uint32_t STORAGE_BOX_ITEMS_COUNT = 2048;
    inline constexpr uint32_t ATTUNEMENT_SLOTS_COUNT = 12;
    inline constexpr uint32_t EQUIP_SLOTS_COUNT = 20;
    inline constexpr uint32_t QUICK_SLOTS_COUNT = 5;
    inline constexpr uint32_t GESTURES_COUNT = 15;
    inline constexpr uint32_t GESTURE_SLOTS_COUNT = 8;
    inline constexpr uint32_t FACE_DATA_SIZE = 100;
    inline constexpr uint32_t EVENT_FLAGS_SIZE = 0x16D00;
    inline constexpr uint32_t EMPTY_ITEM_ID = 0xFFFFFFFFu;

    struct Vector4 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
    };

    // Position and euler rotation (radians), rotation.y is the facing angle.
    struct Transform {
        std::array<float, 3> position{};
        std::array<float, 3> rotation{};
    };

    // Map id as packed by the game, e.g. m10_02 -> area 10, block 2.
    struct MapId {
        uint8_t unk0 = 0;
        uint8_t unk1 = 0;
        uint8_t block = 0;
        uint8_t area = 0;

        bool isValid() const { return area != 0xFF; }
    };

    // Order of equipment in 'ChrAsm::equipItems' and
    //   'EquipGameData::equipItemToInventoryIndex'.
    enum class EquipSlot : uint8_t {
        LeftHand1 = 0,
        RightHand1,
        LeftHand2,
        RightHand2,
        Arrows1,
        Bolts1,
        Arrows2,
        Bolts2,
        Head,
        Body,
        Arms,
        Legs,
        Hair,
        Ring1,
        Ring2,
        Quick1,
        Quick2,
        Quick3,
        Quick4,
        Quick5,
    };

    // --- USER_DATA000-009 ------------------------------------------------
    // 0x5C bytes at the start of a character slot.
    struct SlotHeader {
        uint32_t version = 0;                  // 0x47
        MapId mapId;                           // map where the game was saved
        uint32_t playTimeMs = 0;
        // Block from PlayerGameData up to (excluding) event flags.
        uint32_t gameDataOffset = 0;
        uint32_t gameDataSize = 0;
        uint32_t playerGameDataOffset = 0;
        uint32_t playerGameDataSize = 0;
        uint32_t unkBlock1Offset = 0;
        uint32_t unkBlock0Size = 0;            // block right after PlayerGameData
        uint32_t positionBlockOffset = 0;
        uint32_t positionBlockSize = 0;
        uint32_t clearCountBlockOffset = 0;
        uint32_t clearCountBlockSize = 0;
        uint32_t eventFlagsOffset = 0;
        uint32_t eventFlagsSize = 0;
        uint32_t mapEntityStateOffset = 0;
        uint32_t mapEntityStateSize = 0;
        uint32_t warpBlockOffset = 0;
        // Size of 'mapEntityState' + 'warpBlock'.
        uint32_t mapEntityStateAndWarpSize = 0;
        uint32_t networkBlockOffset = 0;
        uint32_t networkBlockSize = 0;
        uint32_t unkBlock2Offset = 0;
        uint32_t unkBlock2Size = 0;
    };

    // PlayerGameData::AttributeInfo (0x1A4 bytes, memory PGD+0x10).
    struct AttributeInfo {
        uint32_t unk00 = 0;
        uint32_t hp = 0;
        uint32_t hpMax = 0;
        uint32_t hpBase = 0;
        // MP is a leftover from Demon's Souls, not used by the game.
        uint32_t mp = 0;
        uint32_t mpMax = 0;
        uint32_t mpBase = 0;
        uint32_t unk1C = 0;
        uint32_t stamina = 0;
        uint32_t staminaMax = 0;
        uint32_t staminaBase = 0;
        uint32_t unk2C = 0;
        // Stats are stored as 8 byte values, upper 4 bytes are always 0.
        uint32_t vitality = 0;
        uint32_t unk34 = 0;
        uint32_t attunement = 0;
        uint32_t unk3C = 0;
        uint32_t endurance = 0;
        uint32_t unk44 = 0;
        uint32_t strength = 0;
        uint32_t unk4C = 0;
        uint32_t dexterity = 0;
        uint32_t unk54 = 0;
        uint32_t intelligence = 0;
        uint32_t unk5C = 0;
        uint32_t faith = 0;
        uint32_t unk64 = 0;
        uint32_t unk68 = 0;                    // always 10, unused stat?
        uint32_t unk6C = 0;
        uint32_t unk70 = 0;                    // close to humanity value
        uint32_t humanity = 0;
        uint32_t resistance = 0;
        uint32_t unk7C = 0;
        uint32_t level = 0;
        uint32_t souls = 0;
        uint64_t soulMemory = 0;               // souls earned in total
        uint32_t unk90 = 0;
        uint32_t hollowState = 0;              // 0 human, 8 hollow
        std::u16string name;                   // wchar_t[17]
        uint8_t gender = 0;                    // 0 female, 1 male
        std::array<uint8_t, 3> unkBB{};
        uint8_t classId = 0;
        uint8_t physiqueId = 0;
        uint8_t giftId = 0;
        std::array<uint8_t, 3> unkC1{};
        uint32_t multiplayerCount = 0;
        uint32_t coopSuccessCount = 0;
        std::array<uint8_t, 0xE> unkCC{};
        // Index matches 'DSR_COVENANT_LABELS'.
        std::array<uint8_t, 10> covenantLevels{};
        std::array<uint8_t, 0xC> unkE4{};
        // Toxic and poison resistance always have the same value.
        uint32_t toxicRes = 0;
        uint32_t bleedRes = 0;
        uint32_t poisonRes = 0;
        uint32_t curseRes = 0;
        std::array<uint8_t, 3> unk102{};
        uint8_t covenantId = 0;
        // Character creation presets, the real look is in 'FaceData'.
        uint8_t faceId = 0;
        uint8_t hairStyleId = 0;
        uint8_t hairColorId = 0;
        uint8_t unk107 = 0;
        std::array<uint8_t, 4> unk108{};
        uint32_t unk10C = 0;                   // mirrored in 'MenuCharacterData'
        uint32_t unk110 = 0;
        uint32_t unk114 = 0;
        uint32_t unk118 = 0;
        std::u16string name2;                  // wchar_t[17], same as 'name'
        std::array<uint8_t, 0xA> unk13E{};
        std::array<char, 8> gameVersion{};     // e.g. "1.03.1"
        std::array<uint8_t, 0x54> unk150{};
    };

    struct UnkIdRecord {
        int32_t id = -1;
        uint32_t value = 0;
        uint32_t unk08 = 0;
        uint32_t unk0C = 0;
    };

    // ChrAsm without vtable.
    struct ChrAsm {
        uint32_t equippedWeaponStyle = 0;      // 1 one handed, 3 two handed
        uint32_t lHandEquippedIndex = 0;       // 0 = slot 1, 1 = slot 2
        uint32_t rHandEquippedIndex = 0;
        std::array<uint32_t, 4> unk0C{};
        // Item ids, order defined by 'EquipSlot'.
        std::array<uint32_t, EQUIP_SLOTS_COUNT> equipItems{};
    };

    // EquipInventoryData item (0x1C bytes).
    struct EquipInventoryItem {
        uint32_t category = 0;                 // TYPE_* constants from Items.h
        uint32_t itemId = EMPTY_ITEM_ID;       // contains upgrade/infusion
        uint32_t quantity = 0;
        uint32_t sortId = 0;
        uint32_t isValid = 0;
        uint32_t durability = 0;
        uint32_t durabilityHits = 0;
    };

    struct EquipInventoryData {
        uint32_t normalItemsCount = 0;
        uint32_t keyItemsCount = 0;
        uint32_t itemsCapacity = 0;            // 2048
        std::vector<EquipInventoryItem> items;
        // Upper bound of used indexes in 'items'.
        uint32_t itemsEndIndex = 0;
    };

    struct MagicSlot {
        int32_t magicId = -1;
        uint32_t count = 0;
    };

    struct EquipMagicData {
        std::array<MagicSlot, ATTUNEMENT_SLOTS_COUNT> magicSlots{};
        int32_t selectedSlot = -1;
    };

    struct EquipItemData {
        // Inventory indexes of quick slot items.
        std::array<int32_t, QUICK_SLOTS_COUNT> quickSlots{};
        int32_t selectedQuickSlotItem = -1;
    };

    struct GestureSlot {
        uint16_t gestureId = 0;                // see 'DSR_GESTURES' in python
        uint16_t index = 0;
    };

    // Character look as rendered by the game.
    struct FaceData {
        Vector4 hairColor;                     // RGBA
        Vector4 eyeColor;                      // RGBA
        // FaceGen parameters (geometry followed by texture).
        std::array<uint8_t, FACE_DATA_SIZE> data{};
        uint32_t unk70 = 0;
    };

    // Bottomless box item (0x20 bytes), category is part of 'itemId'.
    struct StorageItem {
        uint32_t itemId = EMPTY_ITEM_ID;
        uint32_t sortId = 0;
        uint16_t quantity = 0;
        uint16_t isValid = 0;
        uint32_t durability = 0;
        uint32_t durabilityHits = 0;
        std::array<uint32_t, 3> unk14{};
    };

    struct StorageBoxData {
        std::array<uint32_t, 2> unk00{};
        std::vector<StorageItem> items;
        uint32_t unkEnd = 0;
    };

    // Gesture unlock flags. 'value' = (index + 1) * 2 + unlocked.
    struct GestureEntry {
        uint16_t value = 0;
        uint16_t index = 0;

        bool isUnlocked() const { return (value & 1) != 0; }
    };

    struct GestureGameData {
        std::array<GestureEntry, GESTURES_COUNT> gestures{};
        int32_t terminator = -2;
    };

    struct PlayerGameData {
        AttributeInfo attributes;
        uint32_t unk1A4 = 0;
        std::array<UnkIdRecord, 9> unkRecords{};
        // EquipGameData
        // Inventory index of every equipped item (-1 if empty).
        std::array<int32_t, EQUIP_SLOTS_COUNT> equipItemToInventoryIndex{};
        ChrAsm chrAsm;
        EquipInventoryData inventory;
        EquipMagicData magic;
        EquipItemData quickItems;
        std::array<GestureSlot, GESTURE_SLOTS_COUNT> gestureSlots{};
        uint32_t unkGestureSlots = 0;
        FaceData face;
        StorageBoxData storageBox;
        GestureGameData gestures;
        // Most likely unlocked play region ids (AABCCC, AA area, B block).
        //   The only variable sized part of PlayerGameData.
        uint32_t unkRegionsHeader = 0;
        std::vector<uint32_t> regionIds;
    };

    struct PositionBlock {
        uint8_t unk00 = 0;
        std::array<Transform, 2> transforms{};
        std::array<int32_t, 3> unk31{};
        MapId mapId;
        uint32_t unk41 = 0;
        uint32_t unk45 = 0;
        uint32_t unk49 = 0;
        std::array<uint8_t, 0x2D> unk4D{};
        std::array<int32_t, 4> unk7A{};
        uint32_t unk8A = 0;
    };

    struct ClearCountBlock {
        // Observed (0, 0) during first playthrough and (1, N) later.
        uint32_t unk00 = 0;
        uint32_t clearCount = 0;
    };

    struct UnkRecord16 {
        int32_t unk00 = -1;
        int32_t unk04 = -1;
        int32_t unk08 = -1;
        uint32_t unk0C = 0;
    };

    struct UnkRecord8 {
        int32_t unk00 = -1;
        uint32_t unk04 = 0;
    };

    struct UnkBlock1 {
        std::array<UnkRecord16, 128> records16{};
        std::array<UnkRecord8, 121> records8{};
        std::array<uint8_t, 0x43> unkC08{};
    };

    // Event flag bits. Use 'isEventFlagSet' to read them by flag id.
    struct EventFlags {
        std::vector<uint8_t> flags;            // 0x16D00 bytes
        uint8_t unkEnd = 0;
    };

    // Persistent state of entities of the current map. Chunk payloads start
    //   with fourcc 'CHR ' (characters) or 'OBJ ' (objects).
    struct MapEntityStateChunk {
        std::string tag;
        std::vector<uint8_t> data;             // payload after the fourcc
    };

    struct WarpBlock {
        Vector4 position;                      // w is 1.0
        Vector4 rotation;                      // y is the facing angle
        uint32_t unk20 = 0;
        std::array<uint32_t, 3> unk24{};
        // Most likely last 'stable' position the player is moved to.
        Vector4 stablePosition;
        Vector4 stableRotation;
    };

    struct CharacterSlot {
        SlotHeader header;
        PlayerGameData playerGameData;
        std::array<uint8_t, 0x40> unkBlock0{};
        PositionBlock positionBlock;
        ClearCountBlock clearCountBlock;
        UnkBlock1 unkBlock1;
        EventFlags eventFlags;
        std::vector<MapEntityStateChunk> mapEntityState;
        WarpBlock warpBlock;
        // Multiplayer related data (contains steam ids of other players).
        std::vector<uint8_t> networkBlock;
        std::vector<uint8_t> unkBlock2;
        // Rest of the slot buffer, contains leftovers of older saves.
        std::vector<uint8_t> trailingData;
    };

    // --- USER_DATA010 ----------------------------------------------------
    // Character preview shown in load game menu (0x190 bytes).
    struct MenuCharacterData {
        std::u16string name;                   // wchar_t[17]
        uint16_t unk22 = 0;
        uint32_t level = 0;
        uint32_t playTimeSeconds = 0;
        uint32_t soulMemory = 0;
        MapId mapId;
        uint32_t placeNameId = 0;              // e.g. 1020 for m10_02
        std::array<uint8_t, 0x28> unk38{};
        Vector4 hairColor;
        Vector4 eyeColor;
        std::array<uint8_t, FACE_DATA_SIZE> faceData{};
        std::array<uint8_t, 0x1C> unkE4{};
        std::array<uint8_t, 0x24> unk100{};
        std::array<uint32_t, EQUIP_SLOTS_COUNT> equipItems{};
        std::array<uint8_t, 0xC> unk174{};
        uint8_t gender = 0;
        uint8_t classId = 0;
        uint8_t giftId = 0;
        uint8_t physiqueId = 0;
        uint8_t faceId = 0;
        uint8_t hairStyleId = 0;
        uint8_t hairColorId = 0;
        uint8_t unk187 = 0;
        uint32_t unk188 = 0;                   // 'AttributeInfo::unk10C'
        uint32_t unk18C = 0;
    };

    struct SystemData {
        // Game options (and other unknown data).
        std::array<uint8_t, 0xB0> unk00{};
        std::array<uint8_t, SLOT_COUNT> occupiedSlots{};
        std::array<uint8_t, 6> unkBA{};
        std::array<MenuCharacterData, SLOT_COUNT> menuCharacters{};
        // Key bindings and other unknown data.
        std::vector<uint8_t> unk1060;
    };

    // --- Parsed results --------------------------------------------------
    struct InventoryItem {
        uint32_t itemId = 0;
        uint32_t itemType = 0;
        uint8_t upgradeLevel = 0;
        uint16_t infusion = 0;
        uint32_t amount = 0;
        uint32_t durability = 0;
        uint32_t order = 0;
        uint32_t idx = 0;
        // NOTE how to pass in reference only reference?
        bool knownItem = true;
        BaseItem baseItem;
    };

    struct AttunementSlot {
        uint32_t itemId;
        uint32_t remainingUses;
    };

    struct DSRCharacterInfo {
        int index;
        std::u16string name;
        uint32_t humanity;
        uint32_t souls;
        uint64_t earnedSouls;
        uint32_t hollowState;
        uint8_t covenantId;
        std::array<uint8_t, 10> covenantLevels;

        uint32_t hpCurrent;
        uint32_t hpMax;
        uint32_t hpBase;
        uint32_t staminaCurrent;
        uint32_t staminaMax;
        uint32_t staminaBase;

        uint32_t level;
        uint32_t vitality;
        uint32_t attunement;
        uint32_t endurance;
        uint32_t strength;
        uint32_t dexterity;
        uint32_t resistance;
        uint32_t intelligence;
        uint32_t faith;

        uint8_t gender;
        uint8_t classId;
        uint8_t physiqueId;
        uint8_t giftId;

        uint32_t toxicRes;
        uint32_t bleedRes;
        uint32_t poisonRes;
        uint32_t curseRes;

        uint8_t faceId;
        uint8_t hairStyleId;
        uint8_t hairColorId;

        uint32_t lHandSlot1;
        uint32_t lHandSlot2;
        uint32_t rHandSlot1;
        uint32_t rHandSlot2;
        uint32_t lArrowsSlot;
        uint32_t lBoltsSlot;
        uint32_t rArrowsSlot;
        uint32_t rBoltsSlot;
        uint32_t head_slot;
        uint32_t body_slot;
        uint32_t arms_slot;
        uint32_t legs_slot;
        uint32_t hairSlot;
        uint32_t lRingSlot;
        uint32_t rRingSlot;
        uint32_t q1Slot;
        uint32_t q2Slot;
        uint32_t q3Slot;
        uint32_t q4Slot;
        uint32_t q5Slot;
        uint32_t playTimeMs;
        uint32_t clearCount;
        std::vector<InventoryItem> inventoryItems;
        std::vector<AttunementSlot> attunementSlots;
        std::array<bool, GESTURES_COUNT> unlockedGestures;
        std::vector<InventoryItem> bottomlessBoxItems;
        // Whole slot content as stored in the file.
        CharacterSlot slot;

        // Returns 'std::nullopt' for unknown flag ids.
        std::optional<bool> isEventFlagSet(uint32_t flagId) const;
    };

    struct DSRSaveFile {
        std::vector<DSRCharacterInfo> characters;
        SystemData systemData;
        BND4Entry sideCarEnty;
    };

    // Byte offset and bit mask of event flag in 'EventFlags::flags'.
    std::optional<std::pair<uint32_t, uint32_t>> eventFlagOffset(uint32_t flagId);
    CharacterSlot parse_character_slot(const std::vector<uint8_t>& content);
    SystemData parse_system_data(const std::vector<uint8_t>& content);
    DSRSaveFile parse_dsr_file(const SL2File& sl2);
}
