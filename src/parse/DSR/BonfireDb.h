#pragma once
#include <cstdint>
#include <vector>

#include "SaveData.h"

namespace fssm::parse::dsr {
    // The list holds every bonfire of the maps the character has loaded.
    struct BonfireDbItem {
        uint32_t entityId;          // +0x08, bonfire entity (warp point is entity + 1000)
        // +0x0c: 0 unlit, 10 lit, 20 / 30 / 40 after kindling. The bonfire object
        // buckets it into a kindle level and shows fire effect
        // 90000 + level, level 0 being the unlit bonfire. New Game+
        // keeps it from the previous cycle, as it keeps kindling.
        int16_t kindle;
        int16_t unk_0e;
        uint8_t startsKindled;
        uint8_t unk_11;
        uint8_t unk_12;
    };

    constexpr uint16_t BONFIRE_DB_CHUNK_ID = 8;

    // Items of the bonfire DB chunk; empty when the slot has none.
    std::vector<BonfireDbItem> bonfire_db(const CharacterSlot& slot);
}
