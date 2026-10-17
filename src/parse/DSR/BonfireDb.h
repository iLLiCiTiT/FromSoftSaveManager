#pragma once
#include <cstdint>
#include <vector>

#include "SaveData.h"

namespace fssm::parse::dsr {
    // NS_FRPG::FrpgNetBonfireDbItem as saved in net data chunk 8 (writer FUN_1407a3560,
    // reader FUN_1407a37a0). The game adds an item when a map registers its bonfires
    // (RegisterBonfire in the map's event script), so the list holds every bonfire of
    // the maps the character has loaded.
    struct BonfireDbItem {
        uint32_t entityId;          // +0x08, bonfire entity (warp point is entity + 1000)
        // +0x0c: 0 unlit, 10 lit, 20 / 30 / 40 after kindling. The bonfire object
        // buckets it into a kindle level (FUN_1407a40b0) and shows fire effect
        // 90000 + level, level 0 being the unlit bonfire (FUN_1404ccf20). New Game+
        // keeps it from the previous cycle, as it keeps kindling.
        int16_t kindle;
        int16_t unk_0e;             // +0x0e, clamped together with kindle (FUN_1407a3380)
        // +0x10: set for the bonfires that start kindled (Firelink Shrine, Daughter of
        // Chaos, Anor Londo, Stone Dragon, Firelink Altar); the reader raises their
        // kindle to at least 20, so they read as lit once their map has loaded.
        uint8_t startsKindled;
        uint8_t unk_11;
        uint8_t unk_12;
    };

    constexpr uint16_t BONFIRE_DB_CHUNK_ID = 8;

    // Items of the bonfire DB chunk; empty when the slot has none.
    std::vector<BonfireDbItem> bonfire_db(const CharacterSlot& slot);
}
