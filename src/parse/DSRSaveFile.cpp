#include "DSRSaveFile.h"
#include <cstring>
#include <iostream>
#include <locale>
#include <optional>

namespace fsm::parse {
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
            const auto& c = sl2.entries[charIdx].content;
            if (c.empty() || c[0] == 0x00) continue;
            DSRCharacterInfo ci;
            ci.index = charIdx;
            ci.hpCurrent = bytes_to_u32(c, 96);
            ci.hpMax = bytes_to_u32(c, 100);
            ci.hpBase = bytes_to_u32(c, 104);

            ci.staminaCurrent = bytes_to_u32(c, 124);
            ci.staminaMax = bytes_to_u32(c, 128);
            ci.staminaBase = bytes_to_u32(c, 132);

            ci.vitality = bytes_to_u32(c, 140);
            ci.attunement = bytes_to_u32(c, 148);
            ci.endurance = bytes_to_u32(c, 156);
            ci.strength = bytes_to_u32(c, 164);
            ci.dexterity = bytes_to_u32(c, 172);
            ci.intelligence = bytes_to_u32(c, 180);
            ci.faith = bytes_to_u32(c, 188);
            ci.humanity = bytes_to_u32(c, 208);

            ci.resistance = bytes_to_u32(c, 212);

            ci.level = bytes_to_u32(c, 220);
            ci.souls = bytes_to_u32(c, 224);
            ci.earnedSouls = bytes_to_u32(c, 228);
            ci.hollowState = bytes_to_u32(c, 236);

            std::copy(c.begin() + 310, c.begin() + 320, std::begin(ci.covenantLevels));

            ci.sex = bytes_to_u32(c, 278);
            ci.classId = c[282];
            ci.physiqueId = c[283];
            ci.giftId = c[284];

            ci.toxicRes = bytes_to_u32(c, 332);
            ci.bleedRes = bytes_to_u32(c, 336);
            ci.poisonRes = bytes_to_u32(c, 340);
            ci.curseRes = bytes_to_u32(c, 344);

            ci.covenantId = c[351];

            std::vector<uint8_t> name_b;
            name_b.assign(c.begin() + 244, c.begin() + 268);

            std::u16string name;
            for (size_t i = 0; i + 1 < name_b.size(); i += 2) {
                char16_t ch = static_cast<char16_t>(name_b[i] | (static_cast<char16_t>(name_b[i + 1]) << 8));
                if (ch == 0) break;
                name.push_back(ch);
            }

            ci.name = name;

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
            ci.lHandSlot1 = bytes_to_u32(c, 768);
            ci.lHandSlot2 = bytes_to_u32(c, 772);
            ci.rHandSlot1 = bytes_to_u32(c, 776);
            ci.rHandSlot2 = bytes_to_u32(c, 780);
            ci.lArrowsSlot = bytes_to_u32(c, 784);
            ci.lBoltsSlot = bytes_to_u32(c, 788);
            ci.rArrowsSlot = bytes_to_u32(c, 792);
            ci.rBoltsSlot = bytes_to_u32(c, 796);
            ci.head_slot = bytes_to_u32(c, 800);
            ci.body_slot = bytes_to_u32(c, 804);
            ci.arms_slot = bytes_to_u32(c, 808);
            ci.legs_slot = bytes_to_u32(c, 812);
            // uint32_t unknown_21_flag = bytes_to_u32(c, 816);
            ci.lRingSlot = bytes_to_u32(c, 820);
            ci.rRingSlot = bytes_to_u32(c, 824);
            ci.q1Slot = bytes_to_u32(c, 828);
            ci.q2Slot = bytes_to_u32(c, 832);
            ci.q3Slot = bytes_to_u32(c, 836);
            ci.q4Slot = bytes_to_u32(c, 840);
            ci.q5Slot = bytes_to_u32(c, 844);
            uint32_t backpackCount = bytes_to_u32(c, 848);
            uint32_t unknown_22_flag = bytes_to_u32(c, 852);
            uint32_t maxInventoryCount = bytes_to_u32(c, 856);

            ci.inventoryItems.reserve(maxInventoryCount);
            // TODO Use constant
            uint32_t inventoryOffset = 860;
            std::optional<fssm::parse::dsr::BaseItem> itemOpt;
            for (uint32_t idx = 0; idx < maxInventoryCount; ++idx) {
                uint32_t offset = inventoryOffset + (idx * 28);
                uint32_t itemId = bytes_to_u32(c, offset + 4);
                if (itemId == 0xFFFFFFFFu) continue;
                InventoryItem invItem;
                invItem.itemType = bytes_to_u32(c, offset);
                invItem.itemId = itemId;
                itemOpt = fssm::parse::dsr::findBaseItem(invItem.itemType, invItem.itemId);
                std::optional<fssm::parse::dsr::BaseItem> baseItem = std::nullopt;
                if (itemOpt.has_value()) {
                    baseItem = itemOpt.value();
                } else if (1330000 <= invItem.itemId && invItem.itemId < 1332000) {
                    // Pyromancy Flame has different upgrade levels
                    // Pyromancy Flame
                    invItem.upgradeLevel = (invItem.itemId - 1330000) / 100;
                    invItem.itemId = 1330000;
                    baseItem = fssm::parse::dsr::findBaseItem(invItem.itemType, invItem.itemId).value();
                } else if (1332000 <= invItem.itemId && invItem.itemId <= 1332500) {
                    // Ascended Pyromancy Flame
                    invItem.upgradeLevel = (invItem.itemId - 1332000) / 100;
                    invItem.itemId = 1332000;
                    baseItem = fssm::parse::dsr::findBaseItem(invItem.itemType, invItem.itemId).value();
                } else if (311000 <= invItem.itemId && invItem.itemId <= 312705) {
                    // Sword of Artorias cursed variations
                    invItem.upgradeLevel = invItem.itemId % 100;
                    invItem.itemId = 311000;
                } else {
                    invItem.upgradeLevel = invItem.itemId % 100;
                    uint32_t new_id = invItem.itemId - invItem.upgradeLevel;
                    itemOpt = fssm::parse::dsr::findBaseItem(invItem.itemType, new_id);
                    if (itemOpt.has_value()) {
                        invItem.itemId = new_id;
                        baseItem = itemOpt.value();
                    } else {
                        invItem.infusion = new_id % 1000;
                        new_id -= invItem.infusion;
                        itemOpt = fssm::parse::dsr::findBaseItem(invItem.itemType, new_id);
                        if (itemOpt.has_value()) {
                            invItem.itemId = new_id;
                            baseItem = itemOpt.value();
                        }
                    }
                }
                // Read rest of item information
                invItem.amount = bytes_to_u32(c, offset + 8);
                invItem.order = bytes_to_u32(c, offset + 12);
                // uint32_t _un1 = bytes_to_u32(c, offset + 16);
                invItem.durability = bytes_to_u32(c, offset + 20);
                // uint32_t _un2 = bytes_to_u32(c, offset + 24);

                if (!baseItem.has_value()) {
                    invItem.knownItem = false;
                    fssm::parse::dsr::BaseItem newBaseItem;
                    switch (invItem.itemType) {
                        case 0:
                            newBaseItem.category = "weapons_shields";
                            break;
                        case 268435456:
                            newBaseItem.category = "armor";
                            break;
                        case 536870912:
                            newBaseItem.category = "rings";
                            break;
                        case 1073741824:
                            if (invItem.itemId < 800) {
                                newBaseItem.category = "consumables";
                            } else if (1000 <= invItem.itemId && itemId < 2000) {
                                newBaseItem.category = "materials";
                            } else if (invItem.itemId > 3000) {
                                newBaseItem.category = "spells";
                            } else {
                                newBaseItem.category = "key_items";
                            }
                            break;
                        default:
                            newBaseItem.category = "consumables";
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
        DSRSaveFile save_file = {
            .characters = characters,
            .sideCarEnty = sl2.entries[10]
        };
        return save_file;
    }
}
