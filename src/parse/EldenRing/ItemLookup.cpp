#include "Items.h"

#include <unordered_map>

namespace fssm::parse::er {
    std::optional<BaseItem> findBaseItem(uint32_t id) {
        static const std::unordered_map<uint32_t, const BaseItem*> byId = [] {
            std::unordered_map<uint32_t, const BaseItem*> m;
            m.reserve(ALL_ITEMS.size());
            for (const auto& item : ALL_ITEMS) m.emplace(item.id, &item);
            return m;
        }();
        for (uint32_t candidate : {id, (id & 0xF0000000u) | ((id & 0x0FFFFFFFu) / 100 * 100)}) {
            auto it = byId.find(candidate);
            if (it != byId.end()) return *it->second;
        }
        return std::nullopt;
    }
}
