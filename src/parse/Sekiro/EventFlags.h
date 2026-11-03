#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>

#include "SaveData.h"

namespace fssm::parse::sekiro {
    // Event flag bytes, borrowed from the save data.
    struct EventFlagView {
        const uint8_t* data = nullptr;
        size_t size = 0;
    };

    // Flag id digits: G AA B T NNN = group, map area, map block, thousands, bit.
    // The buffer holds groups 0, 1, 2, 5, 6 and 7 in order with 1, 33, 33, 33, 33 and
    // 33 slots of 0x500 bytes (10 chunks of 1000 flags). Slot 0 holds the global
    // flags (AA B = 0), the others one map block each. Bit n of a chunk is bit
    // 31 - n % 32 of little endian u32 n / 32.
    EventFlagView event_flags(const SaveEntry& entry);

    // nullopt for flags the buffer doesn't hold (other groups, maps without a slot).
    std::optional<bool> get_event_flag(EventFlagView eventFlags, uint32_t flagId);
}
