#include "SaveFile.h"

#include <algorithm>
#include <stdexcept>

#include "StatTables.h"
#include "WorldProgress.h"

namespace fssm::parse::ds2 {
    static std::u16string name_to_string(const char16_t* name, size_t size) {
        std::u16string out;
        for (size_t i = 0; i < size && name[i] != 0; ++i) out.push_back(name[i]);
        return out;
    }

    DS2SaveFile parse_ds2_file(const SL2File& sl2) {
        if (sl2.entries.size() <= CHARACTER_ENTRY_OFFSET + CHARACTER_SLOTS) {
            throw std::runtime_error("DS2 save has too few entries");
        }
        const SaveEntry system = parse_entry(sl2.entries[SYSTEM_ENTRY_INDEX].content);
        const auto loadInfo = read_load_info(system);
        if (!loadInfo) throw std::runtime_error("DS2 save has no LoadInfo record");

        DS2SaveFile out;
        for (size_t slot = 0; slot < CHARACTER_SLOTS; ++slot) {
            const LoadInfoSlot& summary = loadInfo->slotInfos[slot];
            if ((summary.flags & 1) == 0) continue;

            DS2CharacterInfo ci;
            ci.index = static_cast<int>(slot);
            ci.name = name_to_string(summary.name.data(), summary.name.size());
            ci.level = summary.level;
            ci.playTime = summary.playTime;
            ci.mapId = summary.mapId;
            std::copy_n(summary.stats.begin(), ci.stats.size(), ci.stats.begin());
            ci.soulsToNextLevel = soulsToNextLevel(ci.level);
            ci.equipLoad = equipLoadForVitality(ci.stats[2]);

            const SaveEntry character = parse_entry(sl2.entries[CHARACTER_ENTRY_OFFSET + slot].content);
            if (const auto player = read_player(character)) {
                ci.souls = player->souls;
                ci.soulMemory = player->soulMemory;
                ci.hp = player->hp;
            }
            if (const auto inventory = read_inventory(character)) {
                ci.inventoryItems = decode_inventory(*inventory);
            }
            const size_t worldIndex = CHARACTER_WORLD_ENTRY_OFFSET + slot;
            const auto event = worldIndex < sl2.entries.size()
                                   ? read_event(parse_entry(sl2.entries[worldIndex].content))
                                   : std::nullopt;
            if (event) {
                for (const auto& bonfire : BONFIRES) ci.bonfiresLit.push_back(is_bonfire_lit(*event, bonfire.id));
                for (const auto& boss : BOSSES) {
                    ci.bossesDefeated.push_back(get_event_flag(*event, boss.flagId).value_or(false));
                }
            }
            out.characters.push_back(std::move(ci));
        }
        return out;
    }
}
