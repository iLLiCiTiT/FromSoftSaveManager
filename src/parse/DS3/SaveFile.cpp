#include "SaveFile.h"

#include "SaveData.h"

#include <cstring>
#include <unordered_set>

namespace fssm::parse::ds3 {
    Infusion infusionFromInt(const uint32_t& infusion) {
        switch (infusion) {
            case 0:
                return Infusion::None;
            case 100:
                return Infusion::Heavy;
            case 200:
                return Infusion::Sharp;
            case 300:
                return Infusion::Refined;
            case 400:
                return Infusion::Simple;
            case 500:
                return Infusion::Crystal;
            case 600:
                return Infusion::Fire;
            case 700:
                return Infusion::Chaos;
            case 800:
                return Infusion::Lighting;
            case 900:
                return Infusion::Deep;
            case 1000:
                return Infusion::Dark;
            case 1100:
                return Infusion::Poison;
            case 1200:
                return Infusion::Blood;
            case 1300:
                return Infusion::Raw;
            case 1400:
                return Infusion::Blessed;
            case 1500:
                return Infusion::Hollow;
            default:
                return Infusion::None;
        }
    }
    std::optional<InventoryItem> InventoryItem::fromId(uint32_t itemId, const uint32_t& amount) {
        switch (itemId) {
            // TODO these filters should happen in UI
            case 110000: // Empty weapon slot
            case 269335456: // No head armor
            case 269336456: // No chest armor
            case 269337456: // No hands armor
            case 269338456: // No legs armor
            case 4294967295: // Empty item slot
                return std::nullopt;
            default:
                break;
        }
        uint16_t infusion = 0;
        uint8_t upgradeLevel = 0;
        // Estus flask
        if (1073741974 <= itemId && itemId <= 1073741995) {
            uint32_t diff = itemId - 1073741974;
            upgradeLevel = diff / 2;
            itemId -= diff;
        // Ashen Estus flask
        } else if (1073742014 <= itemId && itemId <= 1073742035) {
            uint32_t diff = itemId - 1073742014;
            upgradeLevel = diff / 2;
            itemId -= diff;
        } else if (1000000 < itemId && itemId <= 23020000) {
            upgradeLevel = itemId % 100;
            itemId -= upgradeLevel;
            infusion = itemId % 10000;
            itemId -= infusion;
        }
        Infusion infusionEnum = infusionFromInt(infusion);
        InventoryItem invItem = {
            .itemId = itemId,
            .amount = amount,
            .upgradeLevel = upgradeLevel,
            .infusion = infusionEnum,
        };
        if (
            amount == 1
            && (itemId == 1073741974 || itemId == 1073742014)
            && itemId % 2 == 0
        ) invItem.amount = 0;

        auto baseOpt = findBaseItem(itemId);
        if (baseOpt.has_value()) {
            invItem.baseItem = baseOpt.value();
        }
        return invItem;
    }

    namespace {
        std::u16string name_from(const std::array<char16_t, 17>& name) {
            std::u16string out;
            for (char16_t c : name) {
                if (c == 0) break;
                out.push_back(c);
            }
            return out;
        }

        constexpr uint32_t GOODS_TYPE = 0x40000000;

        // Inventory indices and spell ids the character has equipped.
        struct EquippedSet {
            std::unordered_set<uint16_t> indices;
            std::unordered_set<uint32_t> spells;
        };

        int32_t read_i32(const uint8_t* data) {
            int32_t value;
            std::memcpy(&value, data, sizeof(value));
            return value;
        }

        EquippedSet equipped_set(const EquipGameData& equip) {
            EquippedSet out;
            for (uint32_t index : equip.equipInventoryIndices) {
                if (index != 0xffffffff) out.indices.insert(uint16_t(index & 0xfff));
            }
            // Quick slots and belt: pairs of {handle, inventory index}.
            const auto addPairs = [&](const uint8_t* data, size_t size) {
                for (size_t i = 0; i + 8 <= size; i += 8) {
                    const int32_t index = read_i32(data + i + 4);
                    if (index >= 0) out.indices.insert(uint16_t(index & 0xfff));
                }
            };
            addPairs(equip.items.unk_18.data(), equip.items.unk_18.size());
            addPairs(equip.items.unk_6c.data(), equip.items.unk_6c.size());
            // Attuned spells: pairs of {spell id, -1}.
            for (size_t i = 0; i + 8 <= equip.magic.unk_18.size(); i += 8) {
                const int32_t spell = read_i32(equip.magic.unk_18.data() + i);
                if (spell > 0) out.spells.insert(uint32_t(spell));
            }
            return out;
        }

        std::vector<InventoryItem> decode_items(const std::vector<InventoryEntry>& entries,
                                                const EquippedSet* equipped = nullptr) {
            std::vector<InventoryItem> items;
            items.reserve(entries.size());
            for (const auto& e : entries) {
                if (e.itemId == 0 || e.quantity == 0) continue;
                auto invItemOpt = InventoryItem::fromId(e.itemId, e.quantity);
                if (!invItemOpt.has_value()) continue;
                InventoryItem& item = invItemOpt.value();
                item.inventoryIndex = uint16_t(e.unk_0c & 0xfff);
                if (equipped != nullptr) {
                    item.equipped = equipped->indices.count(item.inventoryIndex) != 0
                        || ((e.itemId & 0xf0000000) == GOODS_TYPE && equipped->spells.count(e.itemId & 0x0fffffff) != 0);
                }
                items.push_back(item);
            }
            return items;
        }

        DS3CharacterInfo character_info(const CharacterSlot& slot, const ProfileSummaryEntry& profile, uint8_t index) {
            const auto& st = slot.player.stats;
            const EquippedSet equipped = equipped_set(slot.player.equip);
            return {
                .index = index,
                .name = name_from(profile.name),
                .hpCurrent = st.hp,
                .hpMax = st.hpMax,
                .hpBase = st.hpBase,
                .fpCurrent = st.fp,
                .fpMax = st.fpMax,
                .fpBase = st.fpBase,
                .staminaCurrent = st.stamina,
                .staminaMax = st.staminaMax,
                .staminaBase = st.staminaBase,
                .level = st.level,
                .souls = st.souls,
                .collectedSouls = st.earnedSouls,
                .vigor = st.vigor,
                .attunement = st.attunement,
                .endurance = st.endurance,
                .strength = st.strength,
                .dexterity = st.dexterity,
                .intelligence = st.intelligence,
                .faith = st.faith,
                .luck = st.luck,
                .vitality = st.vitality,
                .bleedRes = st.bleedResistance,
                .poisonRes = st.poisonResistance,
                .curseRes = st.curseResistance,
                .frostRes = st.frostResistance,
                .hollowing = st.hollowing,
                .estusMax = st.estusMax,
                .ashenEstusMax = st.ashenEstusMax,
                .inventoryItems = decode_items(slot.player.equip.inventory.list.normal, &equipped),
                .keyItems = decode_items(slot.player.equip.inventory.list.key, &equipped),
                .storageBoxItems = decode_items(slot.player.storage.list.normal),
                .playTime = profile.unk_28,
                .mapId = profile.mapId,
                .areaId = profile.unk_34,
                .equipItemIds = slot.player.equip.chrAsm.equipItemIds,
            };
        }
    }

    DS3CharacterInfo parse_ds3_character(const BND4Entry& entry, const BND4Entry& menuEntry, const uint8_t& index) {
        const SystemSlot system = parse_system_slot(menuEntry.content);
        return character_info(parse_character_slot(entry.content), system.profiles.profiles.at(index), index);
    }

    DS3SaveFile parse_ds3_file(const SL2File& sl2) {
        auto& menuEntry = sl2.entries[10];
        const SystemSlot system = parse_system_slot(menuEntry.content);

        std::vector<DS3CharacterInfo> characters;
        characters.reserve(10);
        for (uint8_t i = 0; i < 10; ++i) {
            if (system.profiles.occupied[i] == 1) {
                const CharacterSlot slot = parse_character_slot(sl2.entries[i].content);
                characters.push_back(character_info(slot, system.profiles.profiles[i], i));
            }
        }

        return {
            characters,
            menuEntry,
            sl2.entries[11]
        };
    }
}
