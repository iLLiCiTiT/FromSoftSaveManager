#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>


namespace fssm::parse::ds3 {
    enum class Infusion {
        None,
        Heavy,
        Sharp,
        Refined,
        Simple,
        Crystal,
        Fire,
        Chaos,
        Lighting,
        Deep,
        Dark,
        Poison,
        Blood,
        Raw,
        Blessed,
        Hollow,
    };
    enum class ItemCategory {
        Tools,
        Materials,
        KeyItems,
        Spells,
        MeleeWeapons,
        RangedWeapons,
        Catalysts,
        Shields,
        HeadArmor,
        ChestArmor,
        HandsArmor,
        LegsArmor,
        ArrowsBolts,
        Rings,
        CovenantItem,
    };
    const std::unordered_map<ItemCategory, std::string_view> CategoryIconsMap = {
        {ItemCategory::Tools, "inv_tools"},
        {ItemCategory::Materials, "inv_materials"},
        {ItemCategory::KeyItems, "inv_key_items"},
        {ItemCategory::Spells, "inv_spells"},
        {ItemCategory::MeleeWeapons, "inv_melee_weapons"},
        {ItemCategory::RangedWeapons, "inv_ranged_weapons"},
        {ItemCategory::Catalysts, "inv_catalysts"},
        {ItemCategory::Shields, "inv_shields"},
        {ItemCategory::HeadArmor, "inv_head"},
        {ItemCategory::ChestArmor, "inv_chest"},
        {ItemCategory::HandsArmor, "inv_hands"},
        {ItemCategory::LegsArmor, "inv_legs"},
        {ItemCategory::ArrowsBolts, "inv_arrows_bolts"},
        {ItemCategory::Rings, "inv_rings"},
        {ItemCategory::CovenantItem, "inv_covenant"},
    };
    inline std::string_view getCategoryIcon(ItemCategory category) {return CategoryIconsMap.at(category);}
    struct BaseItem {
        uint32_t id = 0;
        uint32_t max_inventory = 99;
        uint16_t max_storage = 0;
        uint16_t order = 99;
        ItemCategory category = ItemCategory::Tools;
        std::string_view image;
        std::string_view label = "Unknown";
        std::string_view infusion_label;
    };
    extern const std::array<BaseItem, 1138> ALL_ITEMS;

    using ItemRef = std::reference_wrapper<const BaseItem>;
    using ItemMap = std::unordered_map<uint32_t, ItemRef>;

    extern const ItemMap ITEMS_MAPPING;

    std::optional<BaseItem> findBaseItem(const uint32_t& id);
}
