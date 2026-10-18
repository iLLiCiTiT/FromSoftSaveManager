#pragma once

#include <vector>
#include <array>
#include <string>
#include <string_view>
#include "Items.h"
#include "../SL2File.h"

namespace fssm::parse::ds3 {
    struct InventoryItem {
        uint32_t itemId = 0;
        uint32_t amount = 0;
        uint8_t upgradeLevel = 0;
        Infusion infusion = Infusion::None;
        BaseItem baseItem;
        // Index the game uses to reference the entry from equip slots (InventoryEntry::index & 0xfff).
        uint16_t inventoryIndex = 0xffff;
        // Equipped, in a quick item slot or an attuned spell (red seal in the game's inventory).
        bool equipped = false;
        // In the menu's quick items (grey seal in the game's inventory, unless also equipped).
        bool inQuickMenu = false;

        static std::optional<InventoryItem> fromId(uint32_t itemId, const uint32_t& amount);
    };

    // From an NPC's status flags (Npc in WorldProgress.h).
    enum class NpcState : uint8_t {
        Unknown,        // none set: the NPC's map hasn't loaded yet
        Friendly,
        Hostile,
        Dead,           // or gone from the world for good
    };

    struct DS3CharacterInfo {
        uint8_t index;
        std::u16string name;

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
        uint32_t souls;
        uint32_t collectedSouls;
        uint32_t vigor;
        uint32_t attunement;
        uint32_t endurance;
        uint32_t strength;
        uint32_t dexterity;
        uint32_t intelligence;
        uint32_t faith;
        uint32_t luck;
        uint32_t vitality;

        uint32_t bleedRes;
        uint32_t poisonRes;
        uint32_t curseRes;
        uint32_t frostRes;

        uint8_t hollowing;
        uint8_t estusMax;
        uint8_t ashenEstusMax;

        std::vector<InventoryItem> inventoryItems;
        std::vector<InventoryItem> keyItems;
        std::vector<InventoryItem> storageBoxItems;

        // From the load game summary (ProfileSummaryEntry).
        uint32_t playTime = 0;          // seconds
        uint32_t mapId = 0;             // 0xAABB0000 for map mAA_BB
        uint32_t areaId = 0;            // AA * 100 + sub area, e.g. 4005 Firelink Shrine
        // ChrAsm::equipItemIds (weapons, ammunition, armor, rings, ...); 0xffffffff when empty.
        std::array<uint32_t, 0x16> equipItemIds{};

        // From the event flags, one per BONFIRES / BOSSES / NPCS entry (WorldProgress.h).
        std::vector<bool> bonfiresLit;
        std::vector<bool> bossesDefeated;
        std::vector<NpcState> npcStates;
        // Questline stage text per NPCS entry (NPC_STAGES), empty when none is set.
        std::vector<std::string_view> npcStages;
    };

    struct DS3SaveFile {
        std::vector<DS3CharacterInfo> characters;
        BND4Entry menuEntry;
        BND4Entry sideCarEnty;
    };
    DS3CharacterInfo parse_ds3_character(const BND4Entry& entry, const BND4Entry& menuEntry, const uint8_t& index);
    DS3SaveFile parse_ds3_file(const SL2File& sl2);
}
