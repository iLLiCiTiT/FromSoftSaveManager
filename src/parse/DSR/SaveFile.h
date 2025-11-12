#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../SL2File.h"
#include "Items.h"


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

    struct AttunementSlot {
        uint8_t itemId;
        uint8_t remainingUses;
    };

    struct DSRCharacterInfo {
        int index;
        std::u16string name;
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

        uint8_t sex;
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

        uint8_t lHandSlot1;
        uint8_t lHandSlot2;
        uint8_t rHandSlot1;
        uint8_t rHandSlot2;
        uint8_t lArrowsSlot;
        uint8_t lBoltsSlot;
        uint8_t rArrowsSlot;
        uint8_t rBoltsSlot;
        uint8_t head_slot;
        uint8_t body_slot;
        uint8_t arms_slot;
        uint8_t legs_slot;
        uint8_t lRingSlot;
        uint8_t rRingSlot;
        uint8_t q1Slot;
        uint8_t q2Slot;
        uint8_t q3Slot;
        uint8_t q4Slot;
        uint8_t q5Slot;
        std::vector<InventoryItem> inventoryItems;
        std::vector<AttunementSlot> attunementSlots;
        std::array<uint16_t, 18> usedGestures;
        std::vector<InventoryItem> bottomlessBoxItems;
    };

    struct DSRSaveFile {
        std::vector<DSRCharacterInfo> characters;
        BND4Entry sideCarEnty;
    };

    DSRSaveFile parse_dsr_file(const SL2File& sl2);
}
