#include "SaveFile.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

#include "BonfireDb.h"
#include "EventFlags.h"
#include "WorldProgress.h"

namespace fssm::parse::dsr {
    void fillBaseItem(InventoryItem& invItem) {
        auto itemOpt = findBaseItem(invItem.itemType, invItem.itemId);
        std::optional<BaseItem> baseItem = std::nullopt;
        if (itemOpt.has_value()) {
            baseItem = itemOpt.value();
        } else if (1330000 <= invItem.itemId && invItem.itemId < 1332000) {
            // Pyromancy Flame has different upgrade levels
            // Pyromancy Flame
            invItem.upgradeLevel = (invItem.itemId - 1330000) / 100;
            invItem.itemId = 1330000;
            baseItem = findBaseItem(invItem.itemType, invItem.itemId).value();
        } else if (1332000 <= invItem.itemId && invItem.itemId <= 1332500) {
            // Ascended Pyromancy Flame
            invItem.upgradeLevel = (invItem.itemId - 1332000) / 100;
            invItem.itemId = 1332000;
            baseItem = findBaseItem(invItem.itemType, invItem.itemId).value();
        } else if (311000 <= invItem.itemId && invItem.itemId <= 312705) {
            // Sword of Artorias cursed variations
            invItem.upgradeLevel = invItem.itemId % 100;
            invItem.itemId = 311000;
            baseItem = findBaseItem(invItem.itemType, invItem.itemId).value();
        } else {
            invItem.upgradeLevel = invItem.itemId % 100;
            uint32_t new_id = invItem.itemId - invItem.upgradeLevel;
            itemOpt = findBaseItem(invItem.itemType, new_id);
            if (itemOpt.has_value()) {
                invItem.itemId = new_id;
                baseItem = itemOpt.value();
            } else {
                invItem.infusion = new_id % 1000;
                new_id -= invItem.infusion;
                itemOpt = findBaseItem(invItem.itemType, new_id);
                if (itemOpt.has_value()) {
                    invItem.itemId = new_id;
                    baseItem = itemOpt.value();
                }
            }
        }

        if (baseItem.has_value()) {
            invItem.baseItem = baseItem.value();
            return;
        }

        invItem.knownItem = false;
        switch (invItem.itemType) {
            case 0:
                invItem.baseItem.category = ItemCategory::WeaponsShields;
                break;
            case 268435456:
                invItem.baseItem.category = ItemCategory::Armor;
                break;
            case 536870912:
                invItem.baseItem.category = ItemCategory::Rings;
                break;
            case 1073741824:
                if (invItem.itemId < 800) {
                    invItem.baseItem.category = ItemCategory::Consumables;
                } else if (1000 <= invItem.itemId && invItem.itemId < 2000) {
                    invItem.baseItem.category = ItemCategory::Materials;
                } else if (invItem.itemId > 3000) {
                    invItem.baseItem.category = ItemCategory::Spells;
                } else {
                    invItem.baseItem.category = ItemCategory::KeyItems;
                }
                break;
            default:
                invItem.baseItem.category = ItemCategory::Consumables;
                break;
        }
        invItem.baseItem.label = "Unknown " + std::to_string(invItem.itemId);
    }

    static std::u16string name_to_string(const std::array<char16_t, 17>& name) {
        std::u16string out;
        for (char16_t ch : name) {
            if (ch == 0) break;
            out.push_back(ch);
        }
        return out;
    }

    static std::vector<bool> lit_bonfires(const CharacterSlot& slot) {
        const std::vector<BonfireDbItem> db = bonfire_db(slot);
        std::vector<bool> lit;
        lit.reserve(BONFIRES.size());
        for (const auto& bonfire : BONFIRES) {
            bool isLit = false;
            for (const auto& item : db) {
                if (item.entityId == bonfire.entityId && item.kindle > 0) isLit = true;
            }
            lit.push_back(isLit);
        }
        return lit;
    }

    static std::vector<bool> defeated_bosses(const CharacterSlot& slot) {
        std::vector<bool> defeated;
        defeated.reserve(BOSSES.size());
        for (const auto& boss : BOSSES) defeated.push_back(get_event_flag(slot, boss.flagId).value_or(false));
        return defeated;
    }

    static std::vector<NpcState> npc_states(const CharacterSlot& slot) {
        std::vector<NpcState> states;
        states.reserve(NPCS.size());
        const auto isSet = [&slot](uint32_t flag) { return flag != 0 && get_event_flag(slot, flag).value_or(false); };
        const auto anySet = [&isSet](const auto& flags) { return std::any_of(flags.begin(), flags.end(), isSet); };
        for (const auto& npc : NPCS) {
            if (anySet(npc.deadFlags)) {
                states.push_back(NpcState::Dead);
            } else if (anySet(npc.hostileFlags)) {
                states.push_back(NpcState::Hostile);
            } else {
                bool loaded = false;
                for (uint32_t flag = npc.firstFlag; flag <= npc.lastFlag && !loaded; ++flag) loaded = isSet(flag);
                states.push_back(loaded ? NpcState::Alive : NpcState::Unknown);
            }
        }
        return states;
    }

    static DSRCharacterInfo character_info_from_slot(int charIdx, const CharacterSlot& slot) {
        const PlayerGameData& player = slot.player;
        const PlayerGameDataStats& st = player.stats;
        const EquipGameData& equip = player.equip;

        DSRCharacterInfo ci;
        ci.index = charIdx;
        ci.name = name_to_string(st.name);
        ci.humanity = st.humanity;
        ci.souls = st.souls;
        ci.earnedSouls = st.earnedSouls;
        ci.hollowState = st.hollowState;
        ci.covenantId = st.covenantId;
        ci.covenantLevels = st.covenantLevels;

        ci.hpCurrent = st.hp;
        ci.hpMax = st.hpMax;
        ci.hpBase = st.hpBase;
        ci.staminaCurrent = st.stamina;
        ci.staminaMax = st.staminaMax;
        ci.staminaBase = st.staminaBase;

        ci.level = st.level;
        ci.vitality = st.vitality;
        ci.attunement = st.attunement;
        ci.endurance = st.endurance;
        ci.strength = st.strength;
        ci.dexterity = st.dexterity;
        ci.resistance = st.resistance;
        ci.intelligence = st.intelligence;
        ci.faith = st.faith;

        ci.gender = st.gender;
        ci.classId = st.classId;
        ci.physiqueId = st.physiqueId;
        ci.giftId = st.giftId;

        ci.toxicRes = st.toxicResistance;
        ci.bleedRes = st.bleedResistance;
        ci.poisonRes = st.poisonResistance;
        ci.curseRes = st.curseResistance;

        ci.faceId = st.faceId;
        ci.hairStyleId = st.hairStyleId;
        ci.hairColorId = st.hairColorId;

        const ChrAsm& asm_ = equip.chrAsm;
        ci.lHandSlot1 = asm_.leftHand1;
        ci.lHandSlot2 = asm_.leftHand2;
        ci.rHandSlot1 = asm_.rightHand1;
        ci.rHandSlot2 = asm_.rightHand2;
        ci.lArrowsSlot = asm_.leftArrows;
        ci.lBoltsSlot = asm_.leftBolts;
        ci.rArrowsSlot = asm_.rightArrows;
        ci.rBoltsSlot = asm_.rightBolts;
        ci.head_slot = asm_.head;
        ci.body_slot = asm_.body;
        ci.arms_slot = asm_.arms;
        ci.legs_slot = asm_.legs;
        ci.lRingSlot = asm_.leftRing;
        ci.rRingSlot = asm_.rightRing;
        ci.q1Slot = asm_.quickItems[0];
        ci.q2Slot = asm_.quickItems[1];
        ci.q3Slot = asm_.quickItems[2];
        ci.q4Slot = asm_.quickItems[3];
        ci.q5Slot = asm_.quickItems[4];

        const auto& invItems = equip.inventory.items;
        ci.inventoryItems.reserve(invItems.size());
        for (uint32_t idx = 0; idx < invItems.size(); ++idx) {
            const EquipInventoryItem& src = invItems[idx];
            if (src.itemId == 0xFFFFFFFFu) continue;
            InventoryItem invItem;
            invItem.itemType = src.itemType;
            invItem.itemId = src.itemId;
            invItem.amount = src.amount;
            invItem.order = src.order;
            invItem.durability = src.durability;
            invItem.idx = idx;
            fillBaseItem(invItem);
            ci.inventoryItems.push_back(invItem);
        }

        ci.attunementSlots.assign(equip.magic.attunement.begin(), equip.magic.attunement.end());
        ci.equippedGestures = equip.gestureEquip.gestures;

        const auto& boxItems = player.repository.items;
        ci.bottomlessBoxItems.reserve(boxItems.size());
        for (uint32_t idx = 0; idx < boxItems.size(); ++idx) {
            const RepositoryItem& src = boxItems[idx];
            if (src.itemId == 0xFFFFFFFFu) continue;
            InventoryItem invItem;
            invItem.itemId = src.itemId;
            invItem.order = src.order;
            invItem.amount = src.amount;
            invItem.durability = src.durability;
            invItem.idx = idx;
            // Bottomless box stores the item type in the high bits of the id
            if (invItem.itemId >= 1073741824) {
                invItem.itemType = 1073741824;
                invItem.itemId -= 1073741824;
            } else if (invItem.itemId >= 536870912) {
                invItem.itemType = 536870912;
                invItem.itemId -= 536870912;
            } else if (invItem.itemId >= 268435456) {
                invItem.itemType = 268435456;
                invItem.itemId -= 268435456;
            }
            fillBaseItem(invItem);
            ci.bottomlessBoxItems.push_back(invItem);
        }
        ci.bonfiresLit = lit_bonfires(slot);
        ci.bossesDefeated = defeated_bosses(slot);
        ci.npcStates = npc_states(slot);
        return ci;
    }

    DSRSaveFile parse_dsr_file(const SL2File& sl2) {
        if (sl2.entries.size() < 11) {
            throw std::runtime_error("DSR save has fewer than 11 entries");
        }
        // USER_DATA010 holds which character slots are in use
        const SystemSlot systemSlot = parse_system_slot(sl2.entries[10].content);

        std::vector<DSRCharacterInfo> characters;
        characters.reserve(10);
        for (int charIdx = 0; charIdx < 10; ++charIdx) {
            if (systemSlot.profiles.occupied[charIdx] == 0) continue;
            const CharacterSlot slot = parse_character_slot(sl2.entries[charIdx].content);
            characters.push_back(character_info_from_slot(charIdx, slot));
        }
        DSRSaveFile save_file = {
            .characters = characters,
            .sideCarEnty = sl2.entries[10]
        };
        return save_file;
    }
}
