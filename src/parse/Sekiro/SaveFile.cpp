#include "SaveFile.h"

#include "EventFlags.h"
#include "WorldProgress.h"

#include <array>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>

namespace fssm::parse::sekiro {
    namespace {
        // ProfileSummary record of the system entry: one 0x218 byte summary per
        // character slot (what the Load Game screen shows). Offsets are of slot 0;
        // add slot * PROFILE_STRIDE. Located by matching a live save against the
        // game's Load Game screen.
        constexpr size_t PROFILE_STRIDE = 0x218;
        constexpr size_t PROFILE_ACTIVE = 0x34;         // u32, 1 when the slot is used
        constexpr size_t PROFILE_PLAY_TIME = 0x38;      // u32 seconds
        constexpr size_t PROFILE_PLACE_NAME = 0x44;     // u32 PlaceName id of the last idol / area
        constexpr size_t PROFILE_SAVE_TIME = 0x210;     // u64 FILETIME
        constexpr size_t PROFILE_VITALITY = 0x220;      // u32
        constexpr size_t PROFILE_ATTACK_POWER = 0x224;  // u32

        // PlayerGameData record of a character entry.
        constexpr size_t PLAYER_HP = 0x4c4;             // u32 current, max, base max
        constexpr size_t PLAYER_MAX_HP = 0x4c8;
        constexpr size_t PLAYER_POSTURE = 0x4e0;        // u32 current, max, base max
        constexpr size_t PLAYER_MAX_POSTURE = 0x4e4;
        constexpr size_t PLAYER_ATTACK_POWER = 0x4f4;
        constexpr size_t PLAYER_SEN = 0x528;

        // Seconds between 1601-01-01 (FILETIME epoch) and 1970-01-01.
        constexpr int64_t FILETIME_UNIX_OFFSET = 11644473600LL;

        // /msg/engus/item.msgbnd.dcx, 地名.fmg
        constexpr std::array<std::pair<uint32_t, std::string_view>, 11> PLACE_NAMES = {{
            {1000, "Hirata Estate"},
            {1100, "Ashina Outskirts"},
            {1110, "Ashina Castle"},
            {1120, "Ashina Reservoir"},
            {1300, "Abandoned Dungeon"},
            {1500, "Mibu Village"},
            {1510, "Ashina Depths"},
            {1700, "Sunken Valley"},
            {1710, "Sunken Valley Passage"},
            {2000, "Senpou Temple, Mt. Kongo"},
            {2500, "Fountainhead Palace"},
        }};

        template <typename T>
        std::optional<T> read_at(const std::vector<uint8_t>& data, size_t offset) {
            if (offset + sizeof(T) > data.size()) return std::nullopt;
            T value;
            std::memcpy(&value, data.data() + offset, sizeof(T));
            return value;
        }

        template <typename T>
        T read_or(const std::vector<uint8_t>& data, size_t offset, T fallback) {
            return read_at<T>(data, offset).value_or(fallback);
        }

        std::vector<bool> found_idols(EventFlagView eventFlags) {
            std::vector<bool> found;
            found.reserve(IDOLS.size());
            for (const auto& idol : IDOLS) found.push_back(get_event_flag(eventFlags, idol.flagId).value_or(false));
            return found;
        }

        std::vector<bool> defeated_bosses(EventFlagView eventFlags) {
            std::vector<bool> defeated;
            defeated.reserve(BOSSES.size());
            for (const auto& boss : BOSSES) {
                bool isDefeated = false;
                for (const uint32_t flagId : boss.flagIds) {
                    if (flagId != 0 && get_event_flag(eventFlags, flagId).value_or(false)) isDefeated = true;
                }
                defeated.push_back(isDefeated);
            }
            return defeated;
        }

        template <typename Flags>
        bool any_flag_set(EventFlagView eventFlags, const Flags& flagIds) {
            for (const uint32_t flagId : flagIds) {
                if (flagId != 0 && get_event_flag(eventFlags, flagId).value_or(false)) return true;
            }
            return false;
        }

        NpcState npc_state(EventFlagView eventFlags, const Npc& npc) {
            if (any_flag_set(eventFlags, npc.deadFlags)) return NpcState::Dead;
            if (any_flag_set(eventFlags, npc.hostileFlags)) return NpcState::Hostile;
            for (uint32_t flagId = npc.firstFlag; flagId < npc.firstFlag + 20; ++flagId) {
                if (get_event_flag(eventFlags, flagId).value_or(false)) return NpcState::Alive;
            }
            return NpcState::Unknown;
        }

        std::string_view npc_stage(EventFlagView eventFlags, const Npc& npc) {
            for (const auto& stage : NPC_STAGES) {
                if (stage.flagId >= npc.firstFlag && stage.flagId < npc.firstFlag + 20
                    && get_event_flag(eventFlags, stage.flagId).value_or(false)) {
                    return stage.text;
                }
            }
            return {};
        }

        std::vector<bool> found_prosthetic_tools(const ItemList& items) {
            std::vector<bool> found(PROSTHETIC_TOOLS.size(), false);
            const auto check = [&](const InventoryEntry& entry) {
                if (entry.empty() || (entry.itemId >> 28) != 0) return;     // weapons only
                for (size_t i = 0; i < PROSTHETIC_TOOLS.size(); ++i) {
                    const uint32_t weaponId = PROSTHETIC_TOOLS[i].weaponId;
                    if (entry.itemId >= weaponId && entry.itemId < weaponId + 1000) found[i] = true;
                }
            };
            for (const auto& entry : items.normalItems) check(entry);
            for (const auto& entry : items.keyItems) check(entry);
            return found;
        }
    }

    std::string_view findPlaceName(uint32_t placeNameId) {
        for (const auto& [id, name] : PLACE_NAMES) {
            if (id == placeNameId) return name;
        }
        return {};
    }

    SekiroSaveFile parse_sekiro_file(const SL2File& sl2) {
        if (sl2.entries.size() <= SYSTEM_ENTRY_INDEX) {
            throw std::runtime_error("Sekiro save has too few entries");
        }
        const SaveEntry system = parse_entry(sl2.entries[SYSTEM_ENTRY_INDEX].content);
        const Record* profile = system.find(RecordId::ProfileSummary);
        if (profile == nullptr) throw std::runtime_error("Sekiro save has no ProfileSummary record");

        SekiroSaveFile out;
        for (size_t slot = 0; slot < CHARACTER_SLOTS; ++slot) {
            const size_t base = slot * PROFILE_STRIDE;
            const auto& summary = profile->data;
            if (read_or<uint32_t>(summary, base + PROFILE_ACTIVE, 0) != 1) continue;

            const SaveEntry character = parse_entry(sl2.entries[slot].content);
            if (character.empty) continue;

            SekiroCharacterInfo ci;
            ci.index = static_cast<int>(slot);
            ci.playTime = read_or<uint32_t>(summary, base + PROFILE_PLAY_TIME, 0);
            ci.placeNameId = read_or<uint32_t>(summary, base + PROFILE_PLACE_NAME, 0);
            if (const auto fileTime = read_at<uint64_t>(summary, base + PROFILE_SAVE_TIME); fileTime && *fileTime) {
                ci.lastSaved = static_cast<int64_t>(*fileTime / 10000000ULL) - FILETIME_UNIX_OFFSET;
            }
            ci.vitality = read_or<uint32_t>(summary, base + PROFILE_VITALITY, 0);
            ci.attackPower = read_or<uint32_t>(summary, base + PROFILE_ATTACK_POWER, 0);

            if (const Record* player = character.find(RecordId::PlayerGameData)) {
                const auto& data = player->data;
                ci.hp = read_or<uint32_t>(data, PLAYER_HP, 0);
                ci.maxHp = read_or<uint32_t>(data, PLAYER_MAX_HP, 0);
                ci.posture = read_or<uint32_t>(data, PLAYER_POSTURE, 0);
                ci.maxPosture = read_or<uint32_t>(data, PLAYER_MAX_POSTURE, 0);
                ci.sen = read_or<uint32_t>(data, PLAYER_SEN, 0);
                // The summary of the last slot ends before its attack power.
                ci.attackPower = read_or<uint32_t>(data, PLAYER_ATTACK_POWER, ci.attackPower);
            }
            if (const auto gaitem = read_gaitem(character)) {
                ci.inventoryItems = decode_inventory(gaitem->inventory.items, gaitem->storage);
                ci.prostheticToolsFound = found_prosthetic_tools(gaitem->inventory.items);
            }
            const auto eventFlags = event_flags(character);
            ci.idolsFound = found_idols(eventFlags);
            ci.bossesDefeated = defeated_bosses(eventFlags);
            for (const auto& npc : NPCS) {
                ci.npcStates.push_back(npc_state(eventFlags, npc));
                ci.npcStages.push_back(npc_stage(eventFlags, npc));
            }
            for (const auto& step : NPC_STEPS) ci.npcStepsDone.push_back(any_flag_set(eventFlags, step.flags));
            out.characters.push_back(std::move(ci));
        }
        return out;
    }
}
