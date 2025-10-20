#include "DSRSaveFile.h"
#include <codecvt>
#include <cstring>
#include <iostream>
#include <locale>
#include <math.h>

std::string utf16_to_utf8(const std::u16string& s)
{
    // Note: deprecated in C++17, removed in some lib std builds; still widely available.
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes(s);
}

namespace fsm::parse {
    uint32_t bytes_to_u32(const std::vector<uint8_t>& b, const int& offset)
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
        for (int char_idx = 0; char_idx < (int)sl2.entries.size() && char_idx < 10; ++char_idx) {
            const auto& c = sl2.entries[char_idx].content;
            if (c.empty() || c[0] == 0x00) continue;
            DSRCharacterInfo ci;

            ci.hp_current = bytes_to_u32(c, 96);
            ci.hp_max = bytes_to_u32(c, 100);
            ci.hp_base = bytes_to_u32(c, 104);

            ci.stamina_current = bytes_to_u32(c, 124);
            ci.stamina_max = bytes_to_u32(c, 128);
            ci.stamina_base = bytes_to_u32(c, 132);

            ci.vitality = bytes_to_u32(c, 140);
            ci.attunement = bytes_to_u32(c, 148);
            ci.endurance = bytes_to_u32(c, 156);
            ci.strength = bytes_to_u32(c, 164);
            ci.dexterity = bytes_to_u32(c, 172);
            ci.intelligence = bytes_to_u32(c, 180);
            ci.faith = bytes_to_u32(c, 188);
            ci.humanity = bytes_to_u32(c, 208);

            ci.resistance = bytes_to_u32(c, 212);

            ci.level = bytes_to_u32(c, 216);
            ci.souls = bytes_to_u32(c, 220);
            ci.earned_souls = bytes_to_u32(c, 224);
            ci.hollow_state = bytes_to_u32(c, 232);

            std::copy(c.begin() + 310, c.begin() + 319, std::begin(ci.covenant_levels));
            // &ci.covenant_levels = {c[310],c[311],c[312],c[313],c[314],c[315],c[316],c[317],c[318],c[319]};

            ci.toxic_res = bytes_to_u32(c, 332);
            ci.bleed_res = bytes_to_u32(c, 336);
            ci.poison_res = bytes_to_u32(c, 340);
            ci.curse_res = bytes_to_u32(c, 344);

            ci.covenant_id = c[351];

            std::vector<uint8_t> name_b;
            name_b.assign(c.begin() + 244, c.begin() + 258);

            std::u16string name_u16;
            for (size_t i = 0; i + 1 < name_b.size(); i += 2) {
                char16_t ch = static_cast<char16_t>(name_b[i] | (static_cast<char16_t>(name_b[i + 1]) << 8));
                if (ch == 0) break;
                name_u16.push_back(ch);
            }

            ci.name = utf16_to_utf8(name_u16);

            // uint32_t l_ring_slot_item_type = bytes_to_u32(c, 712);
            // uint32_t r_ring_slot_item_type = bytes_to_u32(c, 716);
            // uint32_t q1_slot_item_type = bytes_to_u32(c, 720);
            // uint32_t q2_slot_item_type = bytes_to_u32(c, 724);
            // uint32_t q3_slot_item_type = bytes_to_u32(c, 728);
            // uint32_t q4_slot_item_type = bytes_to_u32(c, 732);
            // uint32_t q5_slot_item_type = bytes_to_u32(c, 736);
            // uint32_t one_double_handling = bytes_to_u32(c, 740);
            // uint32_t l_hand_flag = bytes_to_u32(c, 744);
            // uint32_t r_hand_flag = bytes_to_u32(c, 748);
            // uint32_t unknown_17_flag = bytes_to_u32(c, 752);
            // uint32_t unknown_18_flag = bytes_to_u32(c, 756);
            // uint32_t unknown_19_flag = bytes_to_u32(c, 760);
            // uint32_t unknown_20_flag = bytes_to_u32(c, 764);
            ci.l_hand_slot_1 = bytes_to_u32(c, 768);
            ci.l_hand_slot_2 = bytes_to_u32(c, 772);
            ci.r_hand_slot_1 = bytes_to_u32(c, 776);
            ci.r_hand_slot_2 = bytes_to_u32(c, 780);
            ci.l_arrows_slot = bytes_to_u32(c, 784);
            ci.l_bolts_slot = bytes_to_u32(c, 788);
            ci.r_arrows_slot = bytes_to_u32(c, 792);
            ci.r_bolts_slot = bytes_to_u32(c, 796);
            ci.head_slot = bytes_to_u32(c, 800);
            ci.body_slot = bytes_to_u32(c, 804);
            ci.arms_slot = bytes_to_u32(c, 808);
            ci.legs_slot = bytes_to_u32(c, 812);
            // uint32_t unknown_21_flag = bytes_to_u32(c, 816);
            ci.l_ring_slot = bytes_to_u32(c, 820);
            ci.r_ring_slot = bytes_to_u32(c, 824);
            ci.q1_slot = bytes_to_u32(c, 828);
            ci.q2_slot = bytes_to_u32(c, 832);
            ci.q3_slot = bytes_to_u32(c, 836);
            ci.q4_slot = bytes_to_u32(c, 840);
            ci.q5_slot = bytes_to_u32(c, 844);
            uint32_t backpack_count = bytes_to_u32(c, 848);
            uint32_t unknown_22_flag = bytes_to_u32(c, 852);
            uint32_t max_inventory_count = bytes_to_u32(c, 856);

            ci.inventory_items.reserve(max_inventory_count);
            int inventory_offset = 860;
            for (uint32_t idx = 0; idx < max_inventory_count; ++idx) {
                int offset = inventory_offset + (idx * 28);
                uint32_t item_id = bytes_to_u32(c, offset + 4);
                if (item_id == 0xFFFFFFFFu) continue;
                std::map<uint32_t, BaseItem>::const_iterator tmp_it = ITEMS_MAPPING.find(item_id);
                uint8_t upgrade_level = 0;
                uint8_t infusion = 0;
                std::optional<BaseItem> base_item = std::nullopt;
                if (tmp_it == ITEMS_MAPPING.end()) {
                    // Pyromancy Flame has different upgrade levels
                    if (1330000 <= item_id && item_id < 1332000) {
                        // Pyromancy Flame
                        upgrade_level = (item_id - 1330000) / 100;
                        item_id = 1330000;
                        base_item = ITEMS_MAPPING.at(item_id);
                    } else if (1332000 <= item_id && item_id <= 1332500) {
                        // Ascended Pyromancy Flame
                        upgrade_level = (item_id - 1332000) / 100;
                        item_id = 1332000;
                        base_item = ITEMS_MAPPING.at(item_id);
                    } else if (311000 <= item_id && item_id <= 312705) {
                        // Sword of Artorias cursed variations
                        upgrade_level = item_id % 100;
                        item_id = 311000;
                        base_item = ITEMS_MAPPING.at(item_id);
                    } else {
                        upgrade_level = item_id % 100;
                        uint32_t new_id = item_id - upgrade_level;
                        tmp_it = ITEMS_MAPPING.find(new_id);
                        if (tmp_it != ITEMS_MAPPING.end()) {
                            item_id = new_id;
                            base_item = ITEMS_MAPPING.at(item_id);
                        } else {
                            infusion = new_id % 1000;
                            new_id -= infusion;
                            tmp_it = ITEMS_MAPPING.find(new_id);
                            if (tmp_it != ITEMS_MAPPING.end()) {
                                item_id = new_id;
                                base_item = ITEMS_MAPPING.at(item_id);
                            }
                        }
                    }
                } else {
                    base_item = tmp_it->second;
                }
                // Read rest of item information
                uint32_t item_type = bytes_to_u32(c, offset);
                uint32_t amount = bytes_to_u32(c, offset + 8);
                uint32_t order = bytes_to_u32(c, offset + 12);
                // uint32_t _un1 = bytes_to_u32(c, offset + 16);
                uint32_t durability = bytes_to_u32(c, offset + 20);
                // uint32_t _un2 = bytes_to_u32(c, offset + 24);

                if (!base_item.has_value()) {
                    BaseItem new_item;
                    switch (item_type) {
                        case 0:
                            new_item.category = "weapons_shields";
                            break;
                        case 268435456:
                            new_item.category = "armor";
                            break;
                        case 536870912:
                            new_item.category = "rings";
                            break;
                        case 1073741824:
                            if (item_id < 800) {
                                new_item.category = "consumables";
                            } else if (1000 <= item_id && item_id < 2000) {
                                new_item.category = "materials";
                            } else if (item_id > 3000) {
                                new_item.category = "spells";
                            } else {
                                new_item.category = "key_items";
                            }
                            break;
                        default:
                            new_item.category = "consumables";
                            break;
                    }
                    new_item.label = "Unknown " + std::to_string(item_id);
                    base_item = new_item;
                }
                ci.inventory_items.push_back({
                    .item_id = item_id,
                    .upgrade_level = upgrade_level,
                    .infusion = infusion,
                    .amount = amount,
                    .durability = durability,
                    .order = order,
                    .idx = idx,
                    .base_item = base_item.value()
                });
            }
            characters.push_back(std::move(ci));
        }
        DSRSaveFile save_file = {
            .characters = characters,
            .side_car_enty = sl2.entries[10]
        };
        return save_file;
    }
}
