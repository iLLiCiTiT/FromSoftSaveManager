#include "SaveFile.h"

#include <cstring>
#include <iostream>

#include "../Utils.h"

constexpr std::array<uint8_t, 8> g_SkipValue = {0, 0, 0, 0, 255, 255, 255, 255};

int findInitialOffset(const std::vector<uint8_t>& content) {
    int offset = 108;
    std::array<uint8_t, 8> tmp;
    for (int i = 0; i < 6144; ++i) {
        std::memcpy(tmp.data(), content.data() + offset, 8);
        if (tmp == g_SkipValue) {
            offset += 8;
        } else {
            offset += 60;
        }
    }
    return offset;
}

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

    DS3CharacterInfo parse_ds3_character(const BND4Entry& entry, const BND4Entry& menuEntry, const uint8_t& index) {
        // Get name from menu entry
        int menuOffset = 4254 + (554 * index);
        std::vector<uint8_t> name_b;
        name_b.assign(menuEntry.content.begin() + menuOffset, menuEntry.content.begin() + menuOffset + 32);
        std::u16string name = parse_name(name_b);

        ContentReader reader(entry.content);

        reader.skip(108);

        std::array<uint8_t, 8> tmp;
        for (int i = 0; i < 6144; ++i) {
            reader.copyTo(&tmp, sizeof(tmp));
            if (tmp != g_SkipValue) reader.skip(52);
        }
        reader.skip(8);

        uint32_t hpCurrent = reader.read_u32_le();
        uint32_t hpMax = reader.read_u32_le();
        uint32_t hpBase = reader.read_u32_le();
        uint32_t fpCurrent = reader.read_u32_le();
        uint32_t fpMax = reader.read_u32_le();
        uint32_t fpBase = reader.read_u32_le();
        // always 0
        reader.skip(4);
        uint32_t staminaCurrent = reader.read_u32_le();
        uint32_t staminaMax = reader.read_u32_le();
        uint32_t staminaBase = reader.read_u32_le();
        // always 0
        reader.skip(4);
        uint32_t vigor = reader.read_u32_le();
        uint32_t attunement = reader.read_u32_le();
        uint32_t endurance = reader.read_u32_le();
        uint32_t strength = reader.read_u32_le();
        uint32_t dexterity = reader.read_u32_le();
        uint32_t intelligence = reader.read_u32_le();
        uint32_t faith = reader.read_u32_le();
        uint32_t luck = reader.read_u32_le();
        // always 0
        reader.skip(4);
        // always 0
        reader.skip(4);
        uint32_t vitality = reader.read_u32_le();
        uint32_t level = reader.read_u32_le();
        uint32_t souls = reader.read_u32_le();
        uint32_t collectedSouls = reader.read_u32_le();
        // - offset +100

        reader.skip(100);

        uint32_t toxicRes = reader.read_u32_le();
        uint32_t bleedRes = reader.read_u32_le();
        uint32_t poisonRes = reader.read_u32_le();
        uint32_t curseRes = reader.read_u32_le();
        uint32_t frostRes = reader.read_u32_le();
        // - offset +220

        reader.skip(10);

        uint8_t hollowing = reader.read_u8_le();
        reader.skip(3);
        uint8_t estusMax = reader.read_u8_le();
        uint8_t ashenEstusMax = reader.read_u8_le();

        reader.skip(548);
        // - offset +784

        uint32_t invCount = reader.read_u32_le();
        std::vector<InventoryItem> inventoryItems;
        inventoryItems.reserve(1920);
        for (int i = 0; i < 1920; ++i) {
            reader.skip(4);
            uint32_t itemId = reader.read_u32_le();
            uint32_t amount = reader.read_u32_le();
            reader.skip(4);
            if (itemId == 0 || amount == 0) continue;

            auto invItemOpt = InventoryItem::fromId(itemId, amount);
            if (invItemOpt.has_value()) inventoryItems.push_back(invItemOpt.value());
        }
        // - offset +31508

        reader.skip(4);

        std::vector<InventoryItem> keyItems;
        keyItems.reserve(128);
        for (int i = 0; i < 128; ++i) {
            reader.skip(4);
            uint32_t itemId = reader.read_u32_le();
            uint32_t amount = reader.read_u32_le();
            reader.skip(4);
            if (itemId == 0 || amount == 0) continue;

            auto invItemOpt = InventoryItem::fromId(itemId, amount);
            if (invItemOpt.has_value()) keyItems.push_back(invItemOpt.value());
        }
        // - offset +33560

        reader.skip(2304);

        std::array<uint8_t, 7> usedGestures = {0, 0, 0, 0, 0, 0, 0};
        for (int i = 0; i < 7; ++i) {
            usedGestures[i] = reader.read_u8_le();
        }
        // - offset +35896

        reader.skip(25);

        uint32_t maybeToolsCount = reader.read_u32_le();
        // - offset +35900
        // std::vector<InventoryItem> maybeTools;
        // maybeTools.reserve(maybeToolsCount);
        // for (int i = 0; i < maybeToolsCount; ++i) {
        //     int itemOffset = offset + (i * 8);
        //     uint32_t itemId = read_u32_le(c + itemOffset);
        //     auto invItemOpt = InventoryItem::fromId(itemId, 1);
        //     if (invItemOpt.has_value()) maybeTools.push_back(invItemOpt.value());
        // }
        reader.skip(maybeToolsCount * 8);

        // auto lHand1Equip = InventoryItem::fromId(read_u32_le(c + offset), 1);
        // auto rHand1Equip = InventoryItem::fromId(read_u32_le(c + offset + 4), 1);
        // auto lHand2Equip = InventoryItem::fromId(read_u32_le(c + offset + 8), 1);
        // auto rHand2Equip = InventoryItem::fromId(read_u32_le(c + offset + 12), 1);
        // auto lHand3Equip = InventoryItem::fromId(read_u32_le(c + offset + 16), 1);
        // auto rHand3Equip = InventoryItem::fromId(read_u32_le(c + offset + 20), 1);
        // auto arrows1Equip = InventoryItem::fromId(read_u32_le(c + offset + 24), 1);
        // auto bolts1Equip = InventoryItem::fromId(read_u32_le(c + offset + 28), 1);
        // auto arrows2Equip = InventoryItem::fromId(read_u32_le(c + offset + 32), 1);
        // auto bolts2Equip = InventoryItem::fromId(read_u32_le(c + offset + 36), 1);
        // auto headEquip = InventoryItem::fromId(read_u32_le(c + offset + 48), 1);
        // auto chestEquip = InventoryItem::fromId(read_u32_le(c + offset + 52), 1);
        // auto handsEquip = InventoryItem::fromId(read_u32_le(c + offset + 56), 1);
        // auto legsEquip = InventoryItem::fromId(read_u32_le(c + offset + 60), 1);
        // auto ring1Equip = InventoryItem::fromId(read_u32_le(c + offset + 68), 1);
        // auto ring2Equip = InventoryItem::fromId(read_u32_le(c + offset + 72), 1);
        // auto ring3Equip = InventoryItem::fromId(read_u32_le(c + offset + 76), 1);
        // auto ring4Equip = InventoryItem::fromId(read_u32_le(c + offset + 80), 1);
        // auto covenantEquip = InventoryItem::fromId(read_u32_le(c + offset + 84), 1);
        // auto quickItem1Equip = InventoryItem::fromId(read_u32_le(c + offset + 88), 1);
        // auto quickItem2Equip = InventoryItem::fromId(read_u32_le(c + offset + 92), 1);
        // auto quickItem3Equip = InventoryItem::fromId(read_u32_le(c + offset + 96), 1);
        // auto quickItem4Equip = InventoryItem::fromId(read_u32_le(c + offset + 100), 1);
        // auto quickItem5Equip = InventoryItem::fromId(read_u32_le(c + offset + 104), 1);
        // auto quickItem6Equip = InventoryItem::fromId(read_u32_le(c + offset + 108), 1);
        // auto quickItem7Equip = InventoryItem::fromId(read_u32_le(c + offset + 112), 1);
        // auto quickItem8Equip = InventoryItem::fromId(read_u32_le(c + offset + 116), 1);
        // auto quickItem9Equip = InventoryItem::fromId(read_u32_le(c + offset + 120), 1);
        // auto quickItem10Equip = InventoryItem::fromId(read_u32_le(c + offset + 124), 1);
        // auto toolBelt1Equip = InventoryItem::fromId(read_u32_le(c + offset + 128), 1);
        // auto toolBelt2Equip = InventoryItem::fromId(read_u32_le(c + offset + 132), 1);
        // auto toolBelt3Equip = InventoryItem::fromId(read_u32_le(c + offset + 136), 1);
        // auto toolBelt4Equip = InventoryItem::fromId(read_u32_le(c + offset + 140), 1);
        // auto toolBelt5Equip = InventoryItem::fromId(read_u32_le(c + offset + 144), 1);

        reader.skip(400);
        std::vector<InventoryItem> storageBoxItems;
        inventoryItems.reserve(1920);
        for (int i = 0; i < 1920; ++i) {
            reader.skip(4);
            uint32_t itemId = reader.read_u32_le();
            uint32_t amount = reader.read_u32_le();
            reader.skip(4);
            if (itemId == 0 || amount == 0) continue;

            auto invItemOpt = InventoryItem::fromId(itemId, amount);
            if (invItemOpt.has_value()) storageBoxItems.push_back(invItemOpt.value());
        }

        return {
            .index = index,
            .name = name,
            .hpCurrent = hpCurrent,
            .hpMax = hpMax,
            .hpBase = hpBase,
            .fpCurrent = fpCurrent,
            .fpMax = fpMax,
            .fpBase = fpBase,
            .staminaCurrent = staminaCurrent,
            .staminaMax = staminaMax,
            .staminaBase = staminaBase,
            .level = level,
            .souls = souls,
            .collectedSouls = collectedSouls,
            .vigor = vigor,
            .attunement = attunement,
            .endurance = endurance,
            .strength = strength,
            .dexterity = dexterity,
            .intelligence = intelligence,
            .faith = faith,
            .luck = luck,
            .vitality = vitality,
            .bleedRes = bleedRes,
            .poisonRes = poisonRes,
            .curseRes = curseRes,
            .frostRes = frostRes,
            .hollowing = hollowing,
            .estusMax = estusMax,
            .ashenEstusMax = ashenEstusMax,
            .inventoryItems = inventoryItems,
            .keyItems = keyItems,
            .storageBoxItems = storageBoxItems,
        };
    }

    DS3SaveFile parse_ds3_file(const SL2File& sl2) {
        auto& menuEntry = sl2.entries[10];
        uint64_t steamId = read_u64_le(menuEntry.content.data() + 4);
        std::array<uint8_t, 10> occupiedSlots;
        std::memcpy(occupiedSlots.data(), menuEntry.content.data() + 4244, 10);

        std::vector<DS3CharacterInfo> characters;
        characters.reserve(10);
        for (int i = 0; i < 10; ++i) {
            if (occupiedSlots[i] == 1)
                characters.push_back(parse_ds3_character(sl2.entries[i], menuEntry, i));
        }

        return {
            characters,
            menuEntry,
            sl2.entries[11]
        };
    }
}
