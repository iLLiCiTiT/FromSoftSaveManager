#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>

namespace fssm::parse::er {
    // Event flag bytes, borrowed from the save data.
    struct EventFlagView {
        const uint8_t* data = nullptr;
        size_t size = 0;
    };

    // Size of the event flag region of a character slot.
    constexpr size_t EVENT_FLAGS_SIZE = 0x1bf99f;

    // Flags are stored in blocks of 1000 (FLAG_BLOCK_SIZE bytes, bit 7 of a byte
    // first); a block's position is its rank in the game's block table. Known ranks
    // are in FLAG_BLOCKS (WorldProgress.h, from ER-Save-Editor).
    //
    // nullopt for flags of blocks whose position is not known.
    std::optional<bool> get_event_flag(EventFlagView eventFlags, uint32_t flagId);
}
