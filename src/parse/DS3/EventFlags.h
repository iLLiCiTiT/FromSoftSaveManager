#pragma once
#include <cstdint>
#include <optional>
#include <vector>

#include "SaveData.h"

namespace fssm::parse::ds3 {
    // Event flags of a character (CharacterSlot::eventFlags, bank 0 of SprjEventFlagMan,
    // layout from FUN_1404c6ad0).
    //
    // Flag id digits: G AA B T NNN = group, map area, map block, thousands, bit.
    // The buffer holds groups 0, 1, 2, 5, 6 and 7 in order with 1, 33, 33, 33, 33 and
    // 33 slots of 0x500 bytes (10 chunks of 1000 flags). Slot 0 holds the global
    // flags (AA B = 0), the others one map block each. Bit n of a chunk is bit
    // 31 - n % 32 of little endian u32 n / 32.
    //
    // nullopt for flags the buffer doesn't hold (other groups, maps without a slot).
    std::optional<bool> get_event_flag(const std::vector<uint8_t>& eventFlags, uint32_t flagId);
    std::optional<bool> get_event_flag(const CharacterSlot& slot, uint32_t flagId);
}
