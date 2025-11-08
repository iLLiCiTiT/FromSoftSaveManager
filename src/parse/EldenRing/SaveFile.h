#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../SL2File.h"


namespace fssm::parse::er {
struct GaItem {
    uint32_t handle;
    uint32_t itemId;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t gamhandle;
    uint8_t unknown3;
};
struct ERCharacterInfo {
    int index;
    uint32_t version;
    uint32_t mapId;
    std::array<uint8_t, 8> unknown1;
    std::array<uint8_t, 16> unknown2;

    std::vector<GaItem> gaItems;

    std::u16string name;
    uint32_t runes;
    uint32_t earnedRunes;

    uint32_t hpCurrent;
    uint32_t hpMax;
    uint32_t hpBase;
    uint32_t fpCurrent;
    uint32_t fpMax;
    uint32_t fpBase;
    uint32_t staminaCurrent;
    uint32_t staminaMax;
    uint32_t staminaBase;

    uint32_t level;
    uint32_t vigor;
    uint32_t mind;
    uint32_t endurance;
    uint32_t strength;
    uint32_t dexterity;
    uint32_t intelligence;
    uint32_t faith;
    uint32_t arcane;
};

struct MenuSystemSaveLoad {
    uint16_t unknown1;
    uint16_t unknown2;
    uint32_t length;
    std::vector<uint8_t> data;
};

struct SummaryEquipmentGaItem {
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t armStyle;
    uint32_t leftHandActiveSlot;
    uint32_t rightHandActiveSlot;
    uint32_t leftArrowActiveSlot;
    uint32_t rightArrowActiveSlot;
    uint32_t leftBoltActiveSlot;
    uint32_t rightBoltActiveSlot;
    std::array<uint32_t, 3> leftHandArmaments;
    std::array<uint32_t, 3> rightHandArmaments;
    std::array<uint32_t, 2> arrows;
    std::array<uint32_t, 2> bolts;
    uint32_t unknown3;
    uint32_t head;
    uint32_t chest;
    uint32_t arms;
    uint32_t legs;
    uint32_t unknown4;
    std::array<uint32_t, 4> talismans;
    uint32_t unknown5;
};

struct SummaryEquipmentItem {
    std::array<uint32_t, 3> leftHandArmaments;
    std::array<uint32_t, 3> rightHandArmaments;
    uint32_t unknown1;
    std::array<uint32_t, 2> arrows;
    std::array<uint32_t, 2> bolts;
    uint32_t unknown2;
    uint32_t head;
    uint32_t chest;
    uint32_t arms;
    uint32_t legs;
    uint32_t unknown3;
    std::array<uint32_t, 4> talismans;
    std::array<uint32_t, 6> unknown4;
};

struct SlotSummary {
    std::u16string name;
    uint32_t level;
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
    uint32_t unknown5;
    std::array<uint8_t, 288> unknown6;
    SummaryEquipmentGaItem equipmentGaItem;
    SummaryEquipmentItem equipmentItem;
    uint8_t unknown7;
    uint8_t unknown8;
    uint8_t unknown9;
    uint8_t unknown10;
    uint8_t unknown11;
    uint8_t unknown12;
    int32_t unknown13;
};

struct SlotsSummary {
    std::array<uint8_t, 10> occupied;
    // std::array<SlotSummary, 10> slots;
};

struct UserData10 {
    uint32_t version;
    uint64_t steamId;
    std::array<uint8_t, 320> settings;
    MenuSystemSaveLoad menuSystemSaveLoad;
    SlotsSummary slotsSummary;
    // TODO add rest
};

struct ERSaveFile {
    UserData10 userData10;
    BND4Entry sideCarEnty;
    std::vector<ERCharacterInfo> characters;
};

ERSaveFile parse_er_file(const SL2File& sl2);
}
