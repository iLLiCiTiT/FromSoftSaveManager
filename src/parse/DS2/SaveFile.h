#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "../SL2File.h"
#include "Items.h"
#include "SaveData.h"

namespace fssm::parse::ds2 {
    struct DS2CharacterInfo {
        int index = 0;
        std::u16string name;
        uint32_t level = 0;
        uint32_t souls = 0;
        uint32_t soulMemory = 0;
        uint32_t soulsToNextLevel = 0;  // PlayerLevelUpSoulsParam
        uint32_t hp = 0;
        float equipLoad = 0.0f;         // max equip load from Vitality, without ring bonuses
        uint32_t playTime = 0;          // seconds
        int32_t mapId = 0;              // see findAreaName()
        // vigor, endurance, vitality, attunement, strength, dexterity,
        // intelligence, faith, adaptability
        std::array<uint16_t, 9> stats{};
        std::vector<InventoryItem> inventoryItems;
    };

    struct DS2SaveFile {
        std::vector<DS2CharacterInfo> characters;
    };

    DS2SaveFile parse_ds2_file(const SL2File& sl2);
}
