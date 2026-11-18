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

    namespace {
        // Rows the game keeps in the inventory but never lists: counters such as
        // "Memory" or "Regenerative Power" have no icon or only a placeholder text.
        bool is_internal(const BaseItem& item) {
            if (item.image.empty() || item.description.empty()) return true;
            const std::string_view suffix = " description";
            return item.description.size() == item.label.size() + suffix.size()
                && item.description.substr(0, item.label.size()) == item.label
                && item.description.substr(item.label.size()) == suffix;
        }
    }

    std::vector<InventoryItem> decode_inventory(const ItemList& list) {
        std::vector<InventoryItem> out;
        auto add = [&](const InventoryEntry& entry, bool keyItem) {
            if (entry.empty()) return;
            const auto base = findBaseItem(entry.itemId);
            if (!base || is_internal(*base)) return;     // internal items the game doesn't show
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

    std::vector<InventoryItem> decode_inventory(const ItemList& held, const ItemList& storage) {
        std::unordered_map<uint32_t, uint32_t> stored;
        for (const auto& entry : storage.normalItems) if (!entry.empty()) stored[entry.itemId] += entry.quantity;
        for (const auto& entry : storage.keyItems) if (!entry.empty()) stored[entry.itemId] += entry.quantity;
        std::vector<InventoryItem> out = decode_inventory(held);
        for (auto& item : out) {
            if (auto it = stored.find(item.itemId); it != stored.end()) item.storageQuantity = it->second;
        }
        return out;
    }
}
