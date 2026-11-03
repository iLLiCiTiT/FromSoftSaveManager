#pragma once
#include <cstdint>
#include <optional>
#include <vector>

#include "SaveData.h"

namespace fssm::parse::dsr {
    // Event flags of a character (CharacterSlot::eventFlags, 0x16d00 bytes).
    //
    // Flag id digits: G AA B T NNN = group, map area, map block, thousands, bit.
    // The buffer holds groups 0, 1, 5, 6 and 7 in order with 1, 18, 18, 18 and 18
    // slots of 0x500 bytes (10 chunks of 1000 flags). Slot 0 holds the global flags
    // (ids below 10000000, AA B = 0), the others one map block each. Bit n of a chunk
    // is bit 31 - n % 32 of little endian u32 n / 32.
    //
    // nullopt for flags the buffer doesn't hold (other groups, maps without a slot).
    std::optional<bool> get_event_flag(const std::vector<uint8_t>& eventFlags, uint32_t flagId);
    std::optional<bool> get_event_flag(const CharacterSlot& slot, uint32_t flagId);
}
