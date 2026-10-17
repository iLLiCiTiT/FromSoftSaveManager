#include "BonfireDb.h"

namespace fssm::parse::dsr {
    namespace {
        constexpr uint16_t ITEM_ID = 0;
        constexpr uint16_t END_ID = 0xffff;
        constexpr size_t ITEM_HEADER_SIZE = 8;  // u16 id, u16 version, u32 size
        constexpr size_t ITEM_SIZE = 0xb;

        template <typename T>
        T read_le(const std::vector<uint8_t>& data, size_t offset) {
            T value = 0;
            for (size_t i = 0; i < sizeof(T); ++i) value |= T(data[offset + i]) << (8 * i);
            return value;
        }
    }

    std::vector<BonfireDbItem> bonfire_db(const CharacterSlot& slot) {
        std::vector<BonfireDbItem> items;
        for (const auto& chunk : slot.netChunks.chunks) {
            if (chunk.id != BONFIRE_DB_CHUNK_ID) continue;
            const auto& data = chunk.data;
            // Sub chunks like the chunk buffer itself: 4 aligned, ended by id 0xffff.
            size_t pos = 0;
            while (pos + ITEM_HEADER_SIZE <= data.size()) {
                const auto id = read_le<uint16_t>(data, pos);
                const auto size = read_le<uint32_t>(data, pos + 4);
                pos += ITEM_HEADER_SIZE;
                if (id == END_ID || pos + size > data.size()) break;
                if (id == ITEM_ID && size >= ITEM_SIZE) {
                    items.push_back(BonfireDbItem{
                        .entityId = read_le<uint32_t>(data, pos),
                        .kindle = int16_t(read_le<uint16_t>(data, pos + 4)),
                        .unk_0e = int16_t(read_le<uint16_t>(data, pos + 6)),
                        .startsKindled = data[pos + 8],
                        .unk_11 = data[pos + 9],
                        .unk_12 = data[pos + 10],
                    });
                }
                pos = (pos + size + 3) & ~size_t(3);
            }
        }
        return items;
    }
}
