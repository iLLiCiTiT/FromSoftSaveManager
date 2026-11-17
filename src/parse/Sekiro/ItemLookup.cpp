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
        std::vector<InventoryItem> out = decode_inventory(held);
        std::unordered_map<uint32_t, size_t> rows;
        for (size_t i = 0; i < out.size(); ++i) rows.emplace(out[i].itemId, i);
        // Storage adds its count to the held row; an item kept only in storage gets
        // its own row with nothing held.
        for (const auto& item : decode_inventory(storage)) {
            if (auto it = rows.find(item.itemId); it != rows.end()) {
                out[it->second].storageQuantity += item.quantity;
                continue;
            }
            InventoryItem storedOnly = item;
            storedOnly.storageQuantity = item.quantity;
            storedOnly.quantity = 0;
            rows.emplace(item.itemId, out.size());
            out.push_back(storedOnly);
        }
        return out;
    }
}
