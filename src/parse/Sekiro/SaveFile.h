#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "../SL2File.h"
#include "Items.h"
#include "SaveData.h"

namespace fssm::parse::sekiro {
    // From an NPC's dead and hostile flags (Npc in WorldProgress.h).
    enum class NpcState : uint8_t {
        Unknown,        // no flag of the block set: not met yet
        Alive,
        Hostile,
        Dead,           // or gone for good
    };

    struct SekiroCharacterInfo {
        int index = 0;
        uint32_t playTime = 0;          // seconds
        uint32_t placeNameId = 0;       // PlaceName text id, see findPlaceName()
        int64_t lastSaved = 0;          // unix seconds (UTC), 0 when unknown
        uint32_t vitality = 0;
        uint32_t attackPower = 0;
        uint32_t hp = 0;
        uint32_t maxHp = 0;
        uint32_t posture = 0;
        uint32_t maxPosture = 0;
        uint32_t sen = 0;
        std::vector<InventoryItem> inventoryItems;

        // From the event flags, one per IDOLS / BOSSES entry (WorldProgress.h).
        std::vector<bool> idolsFound;
        std::vector<bool> bossesDefeated;
        // One per NPCS entry: its state and questline stage text (empty when no stage is set).
        std::vector<NpcState> npcStates;
        std::vector<std::string_view> npcStages;
        // One per NPCS entry: sick with Dragonrot (a flag of its Dragonrot group set).
        std::vector<bool> npcDragonrot;
        // One per NPC_STEPS entry.
        std::vector<bool> npcStepsDone;
        // One per PROSTHETIC_TOOLS entry, from the inventory.
        std::vector<bool> prostheticToolsFound;
    };

    struct SekiroSaveFile {
        std::vector<SekiroCharacterInfo> characters;
    };

    // English name of a PlaceName text id ("Ashina Outskirts" for 1100), empty if unknown.
    std::string_view findPlaceName(uint32_t placeNameId);

    SekiroSaveFile parse_sekiro_file(const SL2File& sl2);
}
