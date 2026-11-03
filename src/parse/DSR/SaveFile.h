#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "../SL2File.h"
#include "Items.h"
#include "SaveData.h"


namespace fssm::parse::dsr {
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

    // From an NPC's event flag block (Npc in WorldProgress.h).
    enum class NpcState : uint8_t {
        Unknown,        // no flag of the block set: the NPC's map hasn't loaded yet
        Alive,
        Hostile,
        Dead,
    };

    struct DSRCharacterInfo {
        int index;
        std::u16string name;
        // From the slot's profile summary (Data List screen).
        uint32_t playTime = 0;          // seconds
        std::string_view areaName;      // empty when the PlaceName id is unknown
        uint32_t humanity;
        uint32_t souls;
        uint32_t earnedSouls;
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

        uint8_t toxicRes;
        uint8_t bleedRes;
        uint8_t poisonRes;
        uint8_t curseRes;

        uint8_t faceId;
        uint8_t hairStyleId;
        uint8_t hairColorId;

        // Item ids of equipped items (0xFFFFFFFF when empty)
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
        uint32_t lRingSlot;
        uint32_t rRingSlot;
        uint32_t q1Slot;
        uint32_t q2Slot;
        uint32_t q3Slot;
        uint32_t q4Slot;
        uint32_t q5Slot;
        std::vector<InventoryItem> inventoryItems;
        std::vector<AttunementSlot> attunementSlots;
        std::array<uint32_t, 8> equippedGestures;
        std::vector<InventoryItem> bottomlessBoxItems;

        // One per BONFIRES / BOSSES / NPCS entry (WorldProgress.h): lit in the bonfire DB
        // (BonfireDb.h), defeat flag set and NPC state from the event flags.
        std::vector<bool> bonfiresLit;
        std::vector<bool> bossesDefeated;
        std::vector<NpcState> npcStates;
        // Questline stage text per NPCS entry (NPC_STAGES), empty when none is set.
        std::vector<std::string_view> npcStages;
        // One per NPC_STEPS entry: any of the step's flags set.
        std::vector<bool> npcStepsDone;
    };

    struct DSRSaveFile {
        std::vector<DSRCharacterInfo> characters;
        BND4Entry sideCarEnty;
    };

    DSRSaveFile parse_dsr_file(const SL2File& sl2);
}
