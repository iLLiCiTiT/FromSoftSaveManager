#include "EventFlags.h"

#include <array>

namespace fssm::parse::dsr {
    namespace {
        constexpr size_t FLAG_SLOT_SIZE = 0x500;
        constexpr size_t CHUNK_SIZE = 0x80;
        constexpr std::array<uint32_t, 10> GROUP_SLOTS = {1, 18, 0, 0, 0, 18, 18, 18, 0, 0};

        struct MapSlot {
            uint8_t area;
            uint8_t block;
            uint32_t slot;
        };
        constexpr std::array<MapSlot, 17> MAP_SLOTS = {
            MapSlot{10, 0, 1},      // Depths
            MapSlot{10, 1, 2},      // Undead Burg, Undead Parish
            MapSlot{10, 2, 3},      // Firelink Shrine
            MapSlot{11, 0, 4},      // Painted World of Ariamis
            MapSlot{12, 0, 5},      // Darkroot Garden
            MapSlot{12, 1, 6},      // Oolacile
            MapSlot{13, 0, 7},      // The Catacombs
            MapSlot{13, 1, 8},      // Tomb of the Giants
            MapSlot{13, 2, 9},      // The Great Hollow, Ash Lake
            MapSlot{14, 0, 10},     // Blighttown
            MapSlot{14, 1, 11},     // Demon Ruins, Lost Izalith
            MapSlot{15, 0, 12},     // Sen's Fortress
            MapSlot{15, 1, 13},     // Anor Londo
            MapSlot{16, 0, 14},     // New Londo Ruins, Darkroot Basin
            MapSlot{17, 0, 15},     // The Duke's Archives
            MapSlot{18, 0, 16},     // Kiln of the First Flame
            MapSlot{18, 1, 17},     // Northern Undead Asylum
        };

        std::optional<uint32_t> map_slot(uint32_t area, uint32_t block) {
            if (area == 0 && block == 0) return 0;
            for (const auto& map : MAP_SLOTS) {
                if (map.area == area && map.block == block) return map.slot;
            }
            return std::nullopt;
        }
    }

    std::optional<bool> get_event_flag(const std::vector<uint8_t>& eventFlags, uint32_t flagId) {
        if (flagId >= 100000000) return std::nullopt;
        const uint32_t group = flagId / 10000000 % 10;
        const uint32_t area = flagId / 100000 % 100;
        const uint32_t block = flagId / 10000 % 10;
        const uint32_t thousands = flagId / 1000 % 10;
        const uint32_t bit = flagId % 1000;

        const auto slot = map_slot(area, block);
        if (!slot.has_value() || *slot >= GROUP_SLOTS[group]) return std::nullopt;
        size_t groupBase = 0;
        for (uint32_t g = 0; g < group; ++g) groupBase += GROUP_SLOTS[g] * FLAG_SLOT_SIZE;

        const size_t offset = groupBase + *slot * FLAG_SLOT_SIZE + thousands * CHUNK_SIZE + (bit >> 5) * 4;
        if (offset + 4 > eventFlags.size()) return std::nullopt;
        const uint32_t word = uint32_t(eventFlags[offset])
            | uint32_t(eventFlags[offset + 1]) << 8
            | uint32_t(eventFlags[offset + 2]) << 16
            | uint32_t(eventFlags[offset + 3]) << 24;
        return (word >> (31 - (bit & 31))) & 1;
    }

    std::optional<bool> get_event_flag(const CharacterSlot& slot, uint32_t flagId) {
        return get_event_flag(slot.eventFlags, flagId);
    }
}
