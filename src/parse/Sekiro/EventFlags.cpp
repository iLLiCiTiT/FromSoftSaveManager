#include "EventFlags.h"

#include <array>
#include <cstring>

namespace fssm::parse::sekiro {
    namespace {
        constexpr size_t EVENT_HEADER_SIZE = 0x10;
        constexpr size_t SLOT_SIZE = 0x500;
        constexpr size_t CHUNK_SIZE = 0x80;
        // Slots per flag group.
        constexpr std::array<uint32_t, 10> GROUP_SLOTS = {1, 33, 33, 0, 0, 33, 33, 33, 0, 0};

        struct MapSlot {
            uint8_t area;
            uint8_t block;
            uint32_t slot;
        };
        // 1 + rank of the map block in the game's world map list, confirmed on saves.
        constexpr std::array<MapSlot, 9> MAP_SLOTS = {
            MapSlot{10, 0, 1},      // Hirata Estate
            MapSlot{11, 0, 2},      // Ashina Outskirts
            MapSlot{11, 1, 3},      // Ashina Castle
            MapSlot{11, 2, 4},      // Ashina Reservoir
            MapSlot{13, 0, 7},      // Abandoned Dungeon
            MapSlot{15, 0, 8},      // Mibu Village
            MapSlot{17, 0, 10},     // Sunken Valley, Ashina Depths
            MapSlot{20, 0, 12},     // Senpou Temple
            MapSlot{25, 0, 13},     // Fountainhead Palace
        };

        std::optional<uint32_t> map_slot(uint32_t area, uint32_t block) {
            if (area == 0 && block == 0) return 0;
            for (const auto& map : MAP_SLOTS) {
                if (map.area == area && map.block == block) return map.slot;
            }
            return std::nullopt;
        }
    }

    EventFlagView event_flags(const SaveEntry& entry) {
        const Record* record = entry.find(RecordId::Event);
        if (record == nullptr || record->data.size() < EVENT_HEADER_SIZE + 4) return {};
        uint32_t size = 0;
        std::memcpy(&size, record->data.data() + EVENT_HEADER_SIZE, 4);
        const size_t start = EVENT_HEADER_SIZE + 4;
        if (size > record->data.size() - start) size = uint32_t(record->data.size() - start);
        return {record->data.data() + start, size};
    }

    std::optional<bool> get_event_flag(EventFlagView eventFlags, uint32_t flagId) {
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
        if (offset + 4 > eventFlags.size) return std::nullopt;
        const uint32_t word = uint32_t(eventFlags.data[offset])
            | uint32_t(eventFlags.data[offset + 1]) << 8
            | uint32_t(eventFlags.data[offset + 2]) << 16
            | uint32_t(eventFlags.data[offset + 3]) << 24;
        return (word >> (31 - (bit & 31))) & 1;
    }
}
