#include "Items.h"

#include <cstring>
#include <unordered_map>

namespace fssm::parse::ds2 {
    std::string_view infusionLabel(Infusion infusion) {
        switch (infusion) {
            case Infusion::None: return "";
            case Infusion::Fire: return "Fire";
            case Infusion::Magic: return "Magic";
            case Infusion::Lightning: return "Lightning";
            case Infusion::Dark: return "Dark";
            case Infusion::Poison: return "Poison";
            case Infusion::Bleed: return "Bleed";
            case Infusion::Raw: return "Raw";
            case Infusion::Enchanted: return "Enchanted";
            case Infusion::Mundane: return "Mundane";
        }
        return "";
    }

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
        constexpr uint32_t ESTUS_FLASK_ID = 60155000;

        bool hasDurability(ItemCategory category) {
            switch (category) {
                case ItemCategory::Weapons:
                case ItemCategory::Catalysts:
                case ItemCategory::RangedWeapons:
                case ItemCategory::Shields:
                case ItemCategory::HeadArmor:
                case ItemCategory::ChestArmor:
                case ItemCategory::HandsArmor:
                case ItemCategory::LegsArmor:
                case ItemCategory::Rings:
                    return true;
                default:
                    return false;
            }
        }
    }

    std::vector<InventoryItem> decode_inventory(const InventoryData& inventory) {
        std::vector<InventoryItem> out;
        for (uint32_t slot = 0; slot < inventory.entries.size(); ++slot) {
            const InventoryEntry& entry = inventory.entries[slot];
            if (entry.empty()) continue;

            InventoryItem item;
            item.slot = slot;
            item.itemId = entry.itemId;
            item.upgradeLevel = static_cast<uint8_t>(entry.reinforcement & 0xff);
            const auto infusion = static_cast<uint8_t>((entry.reinforcement >> 8) & 0xff);
            if (infusion <= static_cast<uint8_t>(Infusion::Mundane)) item.infusion = static_cast<Infusion>(infusion);

            if (auto base = findBaseItem(entry.itemId)) {
                item.knownItem = true;
                item.baseItem = *base;
            } else {
                item.baseItem.id = entry.itemId;
            }

            if (entry.itemId == ESTUS_FLASK_ID) {
                item.amount = (entry.value >> 8) & 0xff;    // current charges
            } else if (hasDurability(item.baseItem.category)) {
                float durability;
                std::memcpy(&durability, &entry.value, sizeof(durability));
                item.durability = durability;
                item.amount = 1;
            } else {
                item.amount = entry.value;
            }
            out.push_back(item);
        }
        return out;
    }
}
