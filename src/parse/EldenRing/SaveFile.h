#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../SL2File.h"
#include "Items.h"


namespace fssm::parse::er {
// From an NPC's status flags (Npc in NpcProgress.h).
enum class NpcState : uint8_t {
    Unknown,        // none set: the NPC's map hasn't loaded yet
    Friendly,
    Hostile,
    Dead,
};

struct GaItem {
    uint32_t handle;
    uint32_t itemId;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t gamhandle;
    uint8_t unknown3;
};
// One slot of an EquipInventoryData list.
struct InventoryEntry {
    uint32_t handle;            // 0xB goods / 0xA talisman (item id in the low bits),
                                // 0x8 weapon / 0x9 armor / 0xC ash of war (gaitem handles)
    uint32_t quantity;
    uint32_t acquisitionIndex;  // grows with every pickup; "Recent Items" sorts by it
};

// Held inventory (0xa80 normal + 0x180 key item slots) and the storage chest
// (0x780 + 0x80) share this layout. Read from the save (validated on all
// characters of a live save: the counts match the used slots).
struct EquipInventoryData {
    uint32_t commonCount = 0;
    std::vector<InventoryEntry> commonItems;
    uint32_t keyCount = 0;
    std::vector<InventoryEntry> keyItems;
    uint32_t nextEquipIndex = 0;
    uint32_t nextAcquisitionSortId = 0;
};

// Inventory entry resolved to its item.
struct InventoryItem {
    uint32_t handle = 0;
    uint32_t itemId = 0;        // type nibble << 28 | param row (+ upgrade level for weapons)
    uint32_t quantity = 0;
    uint32_t acquisitionIndex = 0;
    bool keyItem = false;
    bool equipped = false;      // armament, armor or talisman slot
    bool inPouch = false;       // quick items, pouch or the equipped great rune
    BaseItem baseItem;          // id 0 when the item is not in the database
};

struct ERCharacterInfo {
    int index;
    uint32_t version;
    uint32_t mapId;
    std::array<uint8_t, 8> unknown1;
    std::array<uint8_t, 16> unknown2;

    std::vector<GaItem> gaItems;

    std::u16string name;
    uint32_t runes;
    uint32_t earnedRunes;

    uint32_t hpCurrent;
    uint32_t hpMax;
    uint32_t hpBase;
    uint32_t fpCurrent;
    uint32_t fpMax;
    uint32_t fpBase;
    uint32_t staminaCurrent;
    uint32_t staminaMax;
    uint32_t staminaBase;

    uint32_t level;
    uint32_t vigor;
    uint32_t mind;
    uint32_t endurance;
    uint32_t strength;
    uint32_t dexterity;
    uint32_t intelligence;
    uint32_t faith;
    uint32_t arcane;

    std::array<uint32_t, 22> equippedHandles {};
    std::array<uint32_t, 10> quickItemHandles {};
    std::array<uint32_t, 6> pouchHandles {};
    uint32_t greatRuneHandle = 0;
    EquipInventoryData inventory;
    EquipInventoryData storageBox;
    std::vector<InventoryItem> inventoryItems;   // held, normal then key items
    std::vector<InventoryItem> storageItems;

    // From the event flags, one per GRACES / BOSSES entry (WorldProgress.h); empty
    // when the event flag region could not be located.
    std::vector<bool> gracesDiscovered;
    std::vector<bool> bossesDefeated;
    // One per NPCS entry (NpcProgress.h); empty like the lists above.
    std::vector<NpcState> npcStates;
};

struct MenuSystemSaveLoad {
    uint16_t unknown1;
    uint16_t unknown2;
    uint32_t length;
    std::vector<uint8_t> data;
};

struct SummaryEquipmentGaItem {
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t armStyle;
    uint32_t leftHandActiveSlot;
    uint32_t rightHandActiveSlot;
    uint32_t leftArrowActiveSlot;
    uint32_t rightArrowActiveSlot;
    uint32_t leftBoltActiveSlot;
    uint32_t rightBoltActiveSlot;
    std::array<uint32_t, 3> leftHandArmaments;
    std::array<uint32_t, 3> rightHandArmaments;
    std::array<uint32_t, 2> arrows;
    std::array<uint32_t, 2> bolts;
    uint32_t unknown3;
    uint32_t head;
    uint32_t chest;
    uint32_t arms;
    uint32_t legs;
    uint32_t unknown4;
    std::array<uint32_t, 4> talismans;
    uint32_t unknown5;
};

struct SummaryEquipmentItem {
    std::array<uint32_t, 3> leftHandArmaments;
    std::array<uint32_t, 3> rightHandArmaments;
    uint32_t unknown1;
    std::array<uint32_t, 2> arrows;
    std::array<uint32_t, 2> bolts;
    uint32_t unknown2;
    uint32_t head;
    uint32_t chest;
    uint32_t arms;
    uint32_t legs;
    uint32_t unknown3;
    std::array<uint32_t, 4> talismans;
    std::array<uint32_t, 6> unknown4;
};

struct SlotSummary {
    std::u16string name;
    uint32_t level;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
    uint32_t unknown5;
    std::array<uint8_t, 288> unknown6;
    SummaryEquipmentGaItem equipmentGaItem;
    SummaryEquipmentItem equipmentItem;
    uint8_t unknown7;
    uint8_t unknown8;
    uint8_t unknown9;
    uint8_t unknown10;
    uint8_t unknown11;
    uint8_t unknown12;
    int32_t unknown13;
};

struct SlotsSummary {
    std::array<uint8_t, 10> occupied;
    // std::array<SlotSummary, 10> slots;
};

struct UserData10 {
    uint32_t version;
    uint64_t steamId;
    std::array<uint8_t, 320> settings;
    MenuSystemSaveLoad menuSystemSaveLoad;
    SlotsSummary slotsSummary;
    // TODO add rest
};

struct ERSaveFile {
    UserData10 userData10;
    BND4Entry sideCarEnty;
    std::vector<ERCharacterInfo> characters;
};

ERSaveFile parse_er_file(const SL2File& sl2);
}
