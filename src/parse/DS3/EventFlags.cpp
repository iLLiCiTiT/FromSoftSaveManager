#include "EventFlags.h"

#include <array>

namespace fssm::parse::ds3 {
    namespace {
        constexpr size_t SLOT_SIZE = 0x500;
        constexpr size_t CHUNK_SIZE = 0x80;
        // Slots per flag group (table at 0x14278b670).
        constexpr std::array<uint32_t, 10> GROUP_SLOTS = {1, 33, 33, 0, 0, 33, 33, 33, 0, 0};

        struct MapSlot {
            uint8_t area;
            uint8_t block;
            uint32_t slot;
        };
        // 1 + rank of the map block in the game's world map list, confirmed on saves
        // (arenas m46/m47/m53/m54 are not).
        constexpr std::array<MapSlot, 16> MAP_SLOTS = {
            MapSlot{30, 0, 2},      // High Wall of Lothric
            MapSlot{30, 1, 3},      // Lothric Castle
            MapSlot{31, 0, 4},      // Undead Settlement
            MapSlot{32, 0, 6},      // Archdragon Peak
            MapSlot{33, 0, 8},      // Road of Sacrifices, Farron Keep
            MapSlot{34, 1, 10},     // Grand Archives
            MapSlot{35, 0, 11},     // Cathedral of the Deep
            MapSlot{37, 0, 14},     // Irithyll of the Boreal Valley
            MapSlot{38, 0, 15},     // Catacombs of Carthus
            MapSlot{39, 0, 16},     // Irithyll Dungeon
            MapSlot{40, 0, 17},     // Cemetery of Ash, Firelink Shrine
            MapSlot{41, 0, 18},     // Kiln of the First Flame
            MapSlot{45, 0, 19},     // Painted World of Ariandel
            MapSlot{50, 0, 22},     // The Dreg Heap
            MapSlot{51, 0, 23},     // The Ringed City
            MapSlot{51, 1, 24},     // Filianore's Rest
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
        for (uint32_t g = 0; g < group; ++g) groupBase += GROUP_SLOTS[g] * SLOT_SIZE;

        const size_t offset = groupBase + *slot * SLOT_SIZE + thousands * CHUNK_SIZE + (bit >> 5) * 4;
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
