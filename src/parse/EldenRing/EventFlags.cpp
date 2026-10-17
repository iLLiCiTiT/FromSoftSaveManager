#include "EventFlags.h"

#include <algorithm>

#include "WorldProgress.h"

namespace fssm::parse::er {
    std::optional<bool> get_event_flag(EventFlagView eventFlags, uint32_t flagId) {
        const uint32_t blockId = flagId / 1000;
        const auto block = std::lower_bound(FLAG_BLOCKS.begin(), FLAG_BLOCKS.end(), blockId,
                                            [](const FlagBlock& b, uint32_t id) { return b.blockId < id; });
        if (block == FLAG_BLOCKS.end() || block->blockId != blockId) return std::nullopt;
        const uint32_t index = flagId % 1000;
        const size_t offset = size_t(block->rank) * FLAG_BLOCK_SIZE + index / 8;
        if (offset >= eventFlags.size) return std::nullopt;
        return (eventFlags.data[offset] >> (7 - index % 8)) & 1;
    }
}
