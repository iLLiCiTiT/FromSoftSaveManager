#pragma once
#include <cstdint>
#include <string_view>

namespace fssm::parse::ds3 {
    // Area name shown on the load game screen, from ProfileSummaryEntry::mapId
    // (0xAABB0000 for map mAA_BB) and the sub area code (AA * 100 + n).
    // Sub area codes are only partially known; unknown ones fall back to the
    // map's main area. Empty when the map is unknown.
    std::string_view findAreaName(uint32_t mapId, uint32_t areaId);
}
