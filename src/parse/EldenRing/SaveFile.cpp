#include "SaveFile.h"

#include <iostream>
#include <optional>
#include <ostream>

#include "../Utils.h"


namespace fssm::parse::er {
UserData10 parseUserData10(const BND4Entry& entry) {
    UserData10 output;
    ContentReader reader = ContentReader(entry.content);
    output.version = reader.read_u32_le();
    output.steamId = reader.read_u64_le();
    reader.copyTo(&output.settings, sizeof(output.settings));
    output.menuSystemSaveLoad.unknown1 = reader.read_u16_le();
    output.menuSystemSaveLoad.unknown2 = reader.read_u16_le();
    output.menuSystemSaveLoad.length = reader.read_u32_le();
    output.menuSystemSaveLoad.data = reader.read_vec_u8(output.menuSystemSaveLoad.length);
    reader.copyTo(&output.slotsSummary.occupied, sizeof(output.slotsSummary.occupied));
    // TODO do rest
    return output;
}

ERCharacterInfo parseERCharacter(const BND4Entry& entry, const UserData10& userData10, const uint8_t& index) {
    ERCharacterInfo output;
    output.index = index;

    ContentReader reader = ContentReader(entry.content);
    output.version = reader.read_u32_le();
    output.mapId = reader.read_u32_le();
    reader.skip(8);
    reader.skip(16);

    size_t gaItemsCount = (output.version <= 81) ? 5118 : 5120;
    output.gaItems.reserve(gaItemsCount);
    for (int itemIdx = 0; itemIdx < gaItemsCount; ++itemIdx) {
        uint32_t handle = reader.read_u32_le();
        uint32_t itemId = reader.read_u32_le();

        uint32_t gaUnknown1 = 0;
        uint32_t gaUnknown2 = 0;
        uint32_t gemHandle = 0;
        uint8_t gaUnknown3 = 0;
        if (itemId != 0) {
            if ((itemId & 0xf0000000) == 0) {
                gaUnknown1 = reader.read_u32_le();
                gaUnknown2 = reader.read_u32_le();
                gemHandle = reader.read_u32_le();
                gaUnknown3 = reader.read_u8_le();
            }
            if ((itemId & 0xf0000000) == 0x10000000) {
                gaUnknown1 = reader.read_u32_le();
                gaUnknown2 = reader.read_u32_le();
            }
        }
        output.gaItems.push_back({
            handle,
            itemId,
            gaUnknown1,
            gaUnknown2,
            gemHandle,
            gaUnknown3
        });
    }

    reader.skip(4);
    reader.skip(4);
    output.hpCurrent = reader.read_u32_le();
    output.hpMax = reader.read_u32_le();
    output.hpBase = reader.read_u32_le();
    output.fpCurrent = reader.read_u32_le();
    output.fpMax = reader.read_u32_le();
    output.fpBase = reader.read_u32_le();
    reader.skip(4);
    output.staminaCurrent = reader.read_u32_le();
    output.staminaMax = reader.read_u32_le();
    output.staminaBase = reader.read_u32_le();
    reader.skip(4);
    output.vigor = reader.read_u32_le();
    output.mind = reader.read_u32_le();
    output.endurance = reader.read_u32_le();
    output.strength = reader.read_u32_le();
    output.dexterity = reader.read_u32_le();
    output.intelligence = reader.read_u32_le();
    output.faith = reader.read_u32_le();
    output.arcane = reader.read_u32_le();
    reader.skip(4);
    reader.skip(4);
    reader.skip(4);
    output.level = reader.read_u32_le();
    output.runes = reader.read_u32_le();
    output.earnedRunes = reader.read_u32_le();

    reader.skip(40);

    output.name = reader.read_u16_string(16);

    return output;
}

ERSaveFile parse_er_file(const SL2File& sl2) {
    UserData10 userData10 = parseUserData10(sl2.entries[10]);

    ERSaveFile saveFile {
        .userData10 = userData10,
        .sideCarEnty = sl2.entries[11],
    };
    for (int i = 0; i < 10; ++i) {
        if (userData10.slotsSummary.occupied[i] == 0) continue;
        saveFile.characters.push_back(parseERCharacter(sl2.entries[i], userData10, i));
    }
    return saveFile;
}
}
