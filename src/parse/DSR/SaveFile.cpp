#include "SaveFile.h"
#include <cstring>
#include <iostream>
#include <locale>
#include <optional>
#include "../Utils.h"

namespace fssm::parse::dsr {
    uint32_t bytes_to_u32(const std::vector<uint8_t>& b, const uint32_t& offset)
    {
        if (b.size() % sizeof(uint32_t) != 0) {
            throw std::runtime_error("Byte size not a multiple of 4");
        }
        uint32_t out;
        // Safe: copying from bytes into properly aligned uint32_t storage
        std::memcpy(&out, b.data() + offset, 4);
        return out;
    }

    DSRSaveFile parse_dsr_file(const SL2File& sl2) {
        std::vector<DSRCharacterInfo> characters;
        characters.reserve(10);
        for (int charIdx = 0; charIdx < sl2.entries.size() && charIdx < 10; ++charIdx) {
            ContentReader reader(sl2.entries[charIdx].content);
            // TODO read from USERDATA_10 what characters are occupied
            if (reader.read_u8_le() == 0) continue;
            reader.skip(3);

            DSRCharacterInfo ci;
            ci.index = charIdx;

            reader.skip(92);
            ci.hpCurrent = reader.read_u32_le();
            ci.hpMax = reader.read_u32_le();
            ci.hpBase = reader.read_u32_le();

            reader.skip(16);

            ci.staminaCurrent = reader.read_u32_le();
            ci.staminaMax = reader.read_u32_le();
            ci.staminaBase = reader.read_u32_le();

            reader.skip(4);
            ci.vitality = reader.read_u32_le();
            reader.skip(4);
            ci.attunement = reader.read_u32_le();
            reader.skip(4);
            ci.endurance = reader.read_u32_le();
            reader.skip(4);
            ci.strength = reader.read_u32_le();
            reader.skip(4);
            ci.dexterity = reader.read_u32_le();
            reader.skip(4);
            ci.intelligence = reader.read_u32_le();
            reader.skip(4);
            ci.faith = reader.read_u32_le();

            reader.skip(16);

            ci.humanity = reader.read_u32_le();
            reader.skip(4);
            ci.resistance = reader.read_u32_le();
            ci.level = reader.read_u32_le();
            ci.souls = reader.read_u32_le();
            reader.skip(4);
            ci.earnedSouls = reader.read_u32_le();
            reader.skip(4);
            ci.hollowState = reader.read_u32_le();
            // - offset +236
            ci.name = reader.read_u16_string(12);
            reader.skip(10);

            ci.sex = reader.read_u32_le();
            ci.classId = reader.read_u8_le();
            ci.physiqueId = reader.read_u8_le();
            ci.giftId = reader.read_u8_le();

            reader.skip(25);
            // - offset +310

            reader.copyTo(&ci.covenantLevels, 10);
            // - offset +320
            reader.skip(12);

            ci.toxicRes = reader.read_u32_le();
            ci.bleedRes = reader.read_u32_le();
            ci.poisonRes = reader.read_u32_le();
            ci.curseRes = reader.read_u32_le();
            // - offset +348

            reader.skip(3);

            ci.covenantId = reader.read_u8_le();
            // - offset +352

            reader.skip(416);
            // - offset +768

            // uint32_t lRingSlotItemType = bytes_to_u32(c, 712);
            // uint32_t rRingSlotItemType = bytes_to_u32(c, 716);
            // uint32_t q1SlotItemType = bytes_to_u32(c, 720);
            // uint32_t q2SlotItemType = bytes_to_u32(c, 724);
            // uint32_t q3SlotItemType = bytes_to_u32(c, 728);
            // uint32_t q4SlotItemType = bytes_to_u32(c, 732);
            // uint32_t q5SlotItemType = bytes_to_u32(c, 736);
            // uint32_t one_double_handling = bytes_to_u32(c, 740);
            // uint32_t l_hand_flag = bytes_to_u32(c, 744);
            // uint32_t r_hand_flag = bytes_to_u32(c, 748);
            // uint32_t unknown_17_flag = bytes_to_u32(c, 752);
            // uint32_t unknown_18_flag = bytes_to_u32(c, 756);
            // uint32_t unknown_19_flag = bytes_to_u32(c, 760);
            // uint32_t unknown_20_flag = bytes_to_u32(c, 764);
            ci.lHandSlot1 = reader.read_u32_le();
            ci.lHandSlot2 = reader.read_u32_le();
            ci.rHandSlot1 = reader.read_u32_le();
            ci.rHandSlot2 = reader.read_u32_le();
            ci.lArrowsSlot = reader.read_u32_le();
            ci.lBoltsSlot = reader.read_u32_le();
            ci.rArrowsSlot = reader.read_u32_le();
            ci.rBoltsSlot = reader.read_u32_le();
            ci.head_slot = reader.read_u32_le();
            ci.body_slot = reader.read_u32_le();
            ci.arms_slot = reader.read_u32_le();
            ci.legs_slot = reader.read_u32_le();
            reader.skip(4);
            // uint32_t unknown_21_flag = bytes_to_u32(c, 816);
            ci.lRingSlot = reader.read_u32_le();
            ci.rRingSlot = reader.read_u32_le();
            ci.q1Slot = reader.read_u32_le();
            ci.q2Slot = reader.read_u32_le();
            ci.q3Slot = reader.read_u32_le();
            ci.q4Slot = reader.read_u32_le();
            ci.q5Slot = reader.read_u32_le();
            uint32_t backpackCount = reader.read_u32_le();
            uint32_t unknown_22_flag = reader.read_u32_le();
            uint32_t maxInventoryCount = reader.read_u32_le();

            ci.inventoryItems.reserve(maxInventoryCount);
            // TODO Use constant
            uint32_t inventoryOffset = 860;
            std::optional<BaseItem> itemOpt;
            for (uint32_t idx = 0; idx < maxInventoryCount; ++idx) {
                uint32_t offset = inventoryOffset + (idx * 28);
                InventoryItem invItem;
                invItem.itemType = reader.read_u32_le();
                invItem.itemId = reader.read_u32_le();
                invItem.amount = reader.read_u32_le();
                invItem.order = reader.read_u32_le();
                reader.skip(4);
                invItem.durability = reader.read_u32_le();
                reader.skip(4);
                if (invItem.itemId == 0xFFFFFFFFu) continue;
                itemOpt = findBaseItem(invItem.itemType, invItem.itemId);
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

                if (!baseItem.has_value()) {
                    invItem.knownItem = false;
                    BaseItem newBaseItem;
                    switch (invItem.itemType) {
                        case 0:
                            newBaseItem.category = ItemCategory::WeaponsShields;
                            break;
                        case 268435456:
                            newBaseItem.category = ItemCategory::Armor;
                            break;
                        case 536870912:
                            newBaseItem.category = ItemCategory::Rings;
                            break;
                        case 1073741824:
                            if (invItem.itemId < 800) {
                                newBaseItem.category = ItemCategory::Consumables;
                            } else if (1000 <= invItem.itemId && invItem.itemId < 2000) {
                                newBaseItem.category = ItemCategory::Materials;
                            } else if (invItem.itemId > 3000) {
                                newBaseItem.category = ItemCategory::Spells;
                            } else {
                                newBaseItem.category = ItemCategory::KeyItems;
                            }
                            break;
                        default:
                            newBaseItem.category = ItemCategory::Consumables;
                            break;
                    }
                    newBaseItem.label = "Unknown " + std::to_string(invItem.itemId);
                    baseItem = newBaseItem;
                }
                invItem.baseItem = baseItem.value();
                ci.inventoryItems.push_back(invItem);
            }
            characters.push_back(std::move(ci));
        }
        // TODO implemet rest of file
        DSRSaveFile save_file = {
            .characters = characters,
            .sideCarEnty = sl2.entries[10]
        };
        return save_file;
    }
}
