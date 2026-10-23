#include "SaveFile.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <unordered_set>

#include "EventFlags.h"
#include "NpcProgress.h"
#include "PlaceNames.h"
#include "WorldProgress.h"
#include "../Utils.h"


namespace fssm::parse::er {
UserData10 parseUserData10(const BND4Entry& entry) {
    UserData10 output;
    ContentReader reader = ContentReader(entry.content);
    output.version = reader.read_u32_le();
    output.steamId = reader.read_u64_le();
    reader.copyTo(&output.settings, sizeof(output.settings));
    output.menuSystemSaveLoad.unknown1 = reader.read_u16_le();
    output.menuSystemSaveLoad.unknown2 = reader.read_u16_le();
    output.menuSystemSaveLoad.length = reader.read_u32_le();
    output.menuSystemSaveLoad.data = reader.read_vec_u8(output.menuSystemSaveLoad.length);
    reader.copyTo(&output.slotsSummary.occupied, sizeof(output.slotsSummary.occupied));
    constexpr size_t SLOT_SUMMARY_SIZE = 0x24c;
    for (SlotSummary& slot : output.slotsSummary.profiles) {
        const size_t start = reader.pos();
        slot.name = reader.read_u16_string(17);
        slot.level = reader.read_u32_le();
        slot.playTime = reader.read_u32_le();
        slot.unknown2 = reader.read_u32_le();
        slot.mapId = reader.read_u32_le();
        slot.placeNameId = reader.read_u32_le();
        reader.seek(start + SLOT_SUMMARY_SIZE);
    }
    // TODO do rest
    return output;
}

namespace {
// Slot layout after the gaitem table, as far as the inventory needs it
// (offsets from the end of the gaitem table, checked against live saves):
//   +0x000 PlayerGameData (0x1b0), +0x1b0 SpEffects (13 x 0x10),
//   +0x280 equip inventory indices (0x58), +0x2d8 arm style / active slots (0x1c),
//   +0x2f4 equipped item ids (0x58), +0x34c equipped gaitem handles (0x58),
//   +0x3a4 held EquipInventoryData,
//   then spells (14 x 8 + 4), quick items (10 x 8), active quick slot, pouch (6 x 8),
//   great rune (8), gestures (6 x 4), acquired projectiles (count + count x 8),
//   equipped armaments and items (0x9c), physick (0xc), face data (0x12f)
//   and the storage chest EquipInventoryData.
// After the storage chest, up to the event flags (field order of ER-Save-Editor,
// Apache-2.0, src/save/common/save_slot.rs): gesture game data (0x40 x 4), unlocked
// regions (count + count x 4), ride game data (0x28), 1 + 0x40 + 12 bytes, menu
// profile (0x1008), trophy equip data (0x34), GaItemData (8 + 0x1b58 x 16), tutorial
// data (0x408) and 0x1d bytes.
constexpr size_t EQUIPPED_HANDLES_OFFSET = 0x34c;
constexpr size_t INVENTORY_OFFSET = 0x3a4;

// From the end of the storage chest; false when the region doesn't fit the slot.
bool skipToEventFlags(ContentReader& reader) {
    if (reader.pos() + 0x40 * 4 + 4 > reader.size()) return false;
    reader.skip(0x40 * 4);                  // gesture game data
    const uint32_t regionCount = reader.read_u32_le();
    if (regionCount >= 0x1000) return false;
    reader.skip(int(regionCount) * 4);      // unlocked regions
    reader.skip(0x28);                      // ride game data
    reader.skip(1 + 0x40 + 12);
    reader.skip(0x1008);                    // menu profile
    reader.skip(0x34);                      // trophy equip data
    reader.skip(8 + 0x1b58 * 16);           // GaItemData
    reader.skip(0x408);                     // tutorial data
    reader.skip(0x1d);
    return reader.pos() + EVENT_FLAGS_SIZE <= reader.size();
}

EquipInventoryData readInventory(ContentReader& reader, size_t commonSlots, size_t keySlots) {
    EquipInventoryData data;
    const auto readList = [&](std::vector<InventoryEntry>& out, size_t slots) {
        out.resize(slots);
        for (auto& entry : out) {
            entry.handle = reader.read_u32_le();
            entry.quantity = reader.read_u32_le();
            entry.acquisitionIndex = reader.read_u32_le();
        }
    };
    data.commonCount = reader.read_u32_le();
    readList(data.commonItems, commonSlots);
    data.keyCount = reader.read_u32_le();
    readList(data.keyItems, keySlots);
    data.nextEquipIndex = reader.read_u32_le();
    data.nextAcquisitionSortId = reader.read_u32_le();
    return data;
}

std::vector<InventoryItem> resolveItems(const EquipInventoryData& data, const ERCharacterInfo& character) {
    std::unordered_map<uint32_t, uint32_t> gaItemIds;
    for (const auto& gaItem : character.gaItems) {
        if (gaItem.itemId != 0 && gaItem.itemId != 0xffffffff) gaItemIds[gaItem.handle] = gaItem.itemId;
    }
    std::unordered_set<uint32_t> equipped(character.equippedHandles.begin(), character.equippedHandles.end());
    std::unordered_set<uint32_t> pouch(character.quickItemHandles.begin(), character.quickItemHandles.end());
    pouch.insert(character.pouchHandles.begin(), character.pouchHandles.end());
    pouch.insert(character.greatRuneHandle);
    equipped.erase(0);
    equipped.erase(0xffffffff);
    pouch.erase(0);
    pouch.erase(0xffffffff);

    std::vector<InventoryItem> items;
    const auto add = [&](const std::vector<InventoryEntry>& entries, bool keyItem) {
        for (const auto& entry : entries) {
            if (entry.handle == 0 || entry.handle == 0xffffffff) continue;
            InventoryItem item;
            item.handle = entry.handle;
            item.quantity = entry.quantity;
            item.acquisitionIndex = entry.acquisitionIndex;
            item.keyItem = keyItem;
            switch (entry.handle >> 28) {
                case 0xB: item.itemId = 0x40000000 | (entry.handle & 0x0fffffff); break;
                case 0xA: item.itemId = 0x20000000 | (entry.handle & 0x0fffffff); break;
                default: {
                    auto it = gaItemIds.find(entry.handle);
                    if (it != gaItemIds.end()) item.itemId = it->second;
                }
            }
            item.equipped = equipped.count(entry.handle) != 0;
            item.inPouch = pouch.count(entry.handle) != 0;
            if (auto base = findBaseItem(item.itemId)) item.baseItem = *base;
            items.push_back(item);
        }
    };
    add(data.commonItems, false);
    add(data.keyItems, true);
    return items;
}
}

ERCharacterInfo parseERCharacter(const BND4Entry& entry, const UserData10& userData10, const uint8_t& index) {
    ERCharacterInfo output;
    output.index = index;
    output.playTime = userData10.slotsSummary.profiles[index].playTime;
    output.placeName = findPlaceName(userData10.slotsSummary.profiles[index].placeNameId);

    ContentReader reader = ContentReader(entry.content);
    output.version = reader.read_u32_le();
    output.mapId = reader.read_u32_le();
    reader.skip(8);
    reader.skip(16);

    size_t gaItemsCount = (output.version <= 81) ? 5118 : 5120;
    output.gaItems.reserve(gaItemsCount);
    for (int itemIdx = 0; itemIdx < gaItemsCount; ++itemIdx) {
        uint32_t handle = reader.read_u32_le();
        uint32_t itemId = reader.read_u32_le();

        uint32_t gaUnknown1 = 0;
        uint32_t gaUnknown2 = 0;
        uint32_t gemHandle = 0;
        uint8_t gaUnknown3 = 0;
        if (itemId != 0) {
            if ((itemId & 0xf0000000) == 0) {
                gaUnknown1 = reader.read_u32_le();
                gaUnknown2 = reader.read_u32_le();
                gemHandle = reader.read_u32_le();
                gaUnknown3 = reader.read_u8_le();
            }
            if ((itemId & 0xf0000000) == 0x10000000) {
                gaUnknown1 = reader.read_u32_le();
                gaUnknown2 = reader.read_u32_le();
            }
        }
        output.gaItems.push_back({
            handle,
            itemId,
            gaUnknown1,
            gaUnknown2,
            gemHandle,
            gaUnknown3
        });
    }

    const size_t playerStart = reader.pos();
    reader.skip(4);
    reader.skip(4);
    output.hpCurrent = reader.read_u32_le();
    output.hpMax = reader.read_u32_le();
    output.hpBase = reader.read_u32_le();
    output.fpCurrent = reader.read_u32_le();
    output.fpMax = reader.read_u32_le();
    output.fpBase = reader.read_u32_le();
    reader.skip(4);
    output.staminaCurrent = reader.read_u32_le();
    output.staminaMax = reader.read_u32_le();
    output.staminaBase = reader.read_u32_le();
    reader.skip(4);
    output.vigor = reader.read_u32_le();
    output.mind = reader.read_u32_le();
    output.endurance = reader.read_u32_le();
    output.strength = reader.read_u32_le();
    output.dexterity = reader.read_u32_le();
    output.intelligence = reader.read_u32_le();
    output.faith = reader.read_u32_le();
    output.arcane = reader.read_u32_le();
    reader.skip(4);
    reader.skip(4);
    reader.skip(4);
    output.level = reader.read_u32_le();
    output.runes = reader.read_u32_le();
    output.earnedRunes = reader.read_u32_le();

    reader.skip(40);

    output.name = reader.read_u16_string(16);

    reader.seek(playerStart + EQUIPPED_HANDLES_OFFSET);
    for (auto& handle : output.equippedHandles) handle = reader.read_u32_le();
    reader.seek(playerStart + INVENTORY_OFFSET);
    output.inventory = readInventory(reader, 0xa80, 0x180);
    reader.skip(14 * 8 + 4);                // equipped spells, active spell slot
    for (auto& handle : output.quickItemHandles) {
        handle = reader.read_u32_le();
        reader.skip(4);                     // equip inventory index
    }
    reader.skip(4);                         // active quick slot
    for (auto& handle : output.pouchHandles) {
        handle = reader.read_u32_le();
        reader.skip(4);
    }
    output.greatRuneHandle = reader.read_u32_le();
    reader.skip(4);
    reader.skip(6 * 4);                     // gestures
    const uint32_t projectileCount = reader.read_u32_le();
    if (projectileCount < 0x1000) {
        reader.skip(int(projectileCount) * 8);
        reader.skip(0x9c + 0xc + 0x12f);    // equipped armaments and items, physick, face data
        output.storageBox = readInventory(reader, 0x780, 0x80);
    }

    if (projectileCount < 0x1000 && skipToEventFlags(reader)) {
        const EventFlagView eventFlags{entry.content.data() + reader.pos(), EVENT_FLAGS_SIZE};
        for (const auto& grace : GRACES) {
            output.gracesDiscovered.push_back(get_event_flag(eventFlags, grace.flagId).value_or(false));
        }
        for (const auto& boss : BOSSES) {
            output.bossesDefeated.push_back(get_event_flag(eventFlags, boss.flagId).value_or(false));
        }
        for (const auto& npc : NPCS) {
            const auto isSet = [&](uint32_t offset) {
                return get_event_flag(eventFlags, npc.statusFlag + offset).value_or(false);
            };
            output.npcStates.push_back(isSet(3) ? NpcState::Dead
                                       : isSet(1) || isSet(2) ? NpcState::Hostile
                                       : isSet(0) ? NpcState::Friendly : NpcState::Unknown);
            // The questline stage: the first set flag after the status that has a text
            // (merchants keep several groups set, Boc a few side flags).
            std::string_view stage;
            auto it = std::lower_bound(NPC_STAGES.begin(), NPC_STAGES.end(), npc.statusFlag + 5,
                                       [](const NpcStage& s, uint32_t flag) { return s.flag < flag; });
            for (; it != NPC_STAGES.end() && it->flag < npc.statusFlag + 19 && stage.empty(); ++it) {
                if (get_event_flag(eventFlags, it->flag).value_or(false)) stage = it->text;
            }
            output.npcStages.push_back(stage);
        }
        const auto isSet = [&](uint32_t flag) {
            return flag != 0 && get_event_flag(eventFlags, flag).value_or(false);
        };
        for (const auto& step : NPC_STEPS) {
            bool done = false;
            for (size_t i = step.firstFlag; i < size_t(step.firstFlag) + step.flagCount && !done; ++i) {
                done = isSet(NPC_STEP_FLAGS[i]);
            }
            output.npcStepsDone.push_back(done);
        }
        for (const auto& ending : ENDINGS) {
            const bool reached = isSet(ending.reachedFlags[0]) || isSet(ending.reachedFlags[1]);
            const bool available = (ending.unlockFlag == 0 || isSet(ending.unlockFlag)) && !isSet(ending.lockFlag);
            output.endings.push_back(reached ? EndingState::Reached
                                     : available ? EndingState::Available : EndingState::Locked);
        }
    }

    output.inventoryItems = resolveItems(output.inventory, output);
    output.storageItems = resolveItems(output.storageBox, output);
    return output;
}

ERSaveFile parse_er_file(const SL2File& sl2) {
    UserData10 userData10 = parseUserData10(sl2.entries[10]);

    ERSaveFile saveFile {
        .userData10 = userData10,
        .sideCarEnty = sl2.entries[11],
    };
    for (int i = 0; i < 10; ++i) {
        if (userData10.slotsSummary.occupied[i] == 0) continue;
        saveFile.characters.push_back(parseERCharacter(sl2.entries[i], userData10, i));
    }
    return saveFile;
}
}
