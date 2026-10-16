#include "Areas.h"

#include <array>

namespace fssm::parse::ds3 {
    namespace {
        struct SubArea {
            uint32_t areaId;
            std::string_view name;
        };
        // Codes seen in saves.
        constexpr std::array<SubArea, 1> SUB_AREAS = {
            SubArea{4005, "Firelink Shrine"},
        };

        struct MapArea {
            uint8_t area;
            uint8_t block;
            std::string_view name;
        };
        constexpr std::array<MapArea, 18> MAP_AREAS = {
            MapArea{30, 0, "High Wall of Lothric"},
            MapArea{30, 1, "Lothric Castle"},
            MapArea{31, 0, "Undead Settlement"},
            MapArea{32, 0, "Archdragon Peak"},
            MapArea{33, 0, "Road of Sacrifices"},
            MapArea{34, 1, "Grand Archives"},
            MapArea{35, 0, "Cathedral of the Deep"},
            MapArea{37, 0, "Irithyll of the Boreal Valley"},
            MapArea{38, 0, "Catacombs of Carthus"},
            MapArea{39, 0, "Irithyll Dungeon"},
            MapArea{40, 0, "Cemetery of Ash"},
            MapArea{41, 0, "Kiln of the First Flame"},
            MapArea{45, 0, "Painted World of Ariandel"},
            MapArea{46, 0, "Arena"},
            MapArea{47, 0, "Arena"},
            MapArea{50, 0, "The Dreg Heap"},
            MapArea{51, 0, "The Ringed City"},
            MapArea{51, 1, "Filianore's Rest"},
        };
    }

    std::string_view findAreaName(uint32_t mapId, uint32_t areaId) {
        for (const auto& sub : SUB_AREAS) {
            if (sub.areaId == areaId) return sub.name;
        }
        const uint8_t area = uint8_t(mapId >> 24);
        const uint8_t block = uint8_t(mapId >> 16);
        std::string_view fallback;
        for (const auto& map : MAP_AREAS) {
            if (map.area != area) continue;
            if (map.block == block) return map.name;
            if (fallback.empty()) fallback = map.name;
        }
        return fallback;
    }
}
