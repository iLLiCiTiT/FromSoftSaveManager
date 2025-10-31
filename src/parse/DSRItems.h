#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>
#include <optional>

namespace fssm::parse::dsr {
    inline constexpr std::array<std::string_view, 10> DSR_COVENANT_LABELS = {
        "None",
        "Way of White",
        "Princess's Guard",
        "Warrior of Sunlight",
        "Darkwraith",
        "Path of the Dragon",
        "Gravelord Servant",
        "Forest Hunter",
        "Darkmoon Blade",
        "Chaos Servant",
    };

    inline constexpr std::array<std::string_view, 10> DSR_CLASSES = {
        "Warrior",
        "Knight",
        "Wanderer",
        "Thief",
        "Bandit",
        "Hunter",
        "Sorcerer",
        "Pyromancer",
        "Cleric",
        "Deprived"
    };

    inline constexpr std::array<std::string_view, 9> DSR_GIFTS = {
        "None",
        "Goddess's Blessing",
        "Black Firebomb",
        "Twin Humanities",
        "Binoculars",
        "Pendant",
        "Master Key",
        "Tiny Being's Ring",
        "Old Witch's Ring",
    };
    inline constexpr std::array<std::string_view, 9> DSR_PHYSIQUE = {
        "Average",
        "Slim",
        "Very Slim",
        "Large",
        "Very Large",
        "Large Upper Body",
        "Large Lower Body",
        "Top-heavy",
        "Tiny Head",
    };

    struct BaseItem {
        uint32_t id = 0;
        uint32_t type = 0;
        uint32_t max_stack_count = 999;
        // TODO use enum for category
        std::string_view category = "consumables";
        std::string_view image;
        std::string_view label = "Unknown";
    };
    extern const std::array<BaseItem, 708> ALL_ITEMS;

    using ItemRef = std::reference_wrapper<const BaseItem>;
    using ItemMap = std::unordered_map<uint32_t, ItemRef>;

    // Helper that builds a map for a given type
    inline constexpr uint32_t TYPE_WEAPON = 0x00000000;  // 0
    inline constexpr uint32_t TYPE_ARMOR  = 0x10000000;  // 268435456
    inline constexpr uint32_t TYPE_RING   = 0x20000000;  // 536870912
    inline constexpr uint32_t TYPE_OTHER  = 0x40000000;  // 1073741824

    // The actual static maps
    extern const ItemMap WEAPON_ITEMS_MAPPING;
    extern const ItemMap ARMOR_ITEMS_MAPPING;
    extern const ItemMap RING_ITEMS_MAPPING;
    extern const ItemMap OTHER_ITEMS_MAPPING;

    std::optional<BaseItem> findBaseItem(const uint32_t& type, const uint32_t& id);
}
