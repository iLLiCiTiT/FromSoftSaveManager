#include "Items.h"

#include <unordered_map>

namespace fssm::parse::sekiro {
    std::optional<BaseItem> findBaseItem(uint32_t id) {
        static const std::unordered_map<uint32_t, const BaseItem*> byId = [] {
            std::unordered_map<uint32_t, const BaseItem*> m;
            m.reserve(ALL_ITEMS.size());
            for (const auto& item : ALL_ITEMS) m.emplace(item.id, &item);
            return m;
        }();
        auto it = byId.find(id);
        if (it == byId.end()) return std::nullopt;
        return *it->second;
    }

    std::vector<InventoryItem> decode_inventory(const ItemList& list) {
        std::vector<InventoryItem> out;
        auto add = [&](const InventoryEntry& entry, bool keyItem) {
            if (entry.empty()) return;
            const auto base = findBaseItem(entry.itemId);
            if (!base) return;          // internal items the game doesn't show
            InventoryItem item;
            item.itemId = entry.itemId;
            item.quantity = entry.quantity;
            item.keyItem = keyItem;
            item.knownItem = true;
            item.baseItem = *base;
            out.push_back(item);
        };
        for (const auto& entry : list.normalItems) add(entry, false);
        for (const auto& entry : list.keyItems) add(entry, true);
        return out;
    }
}
