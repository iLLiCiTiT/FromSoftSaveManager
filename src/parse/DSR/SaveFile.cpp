#include "SaveFile.h"
#include <cstring>
#include <iostream>
#include <locale>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include "../Utils.h"

namespace fssm::parse::dsr {
    uint32_t bytes_to_u32(const std::vector<uint8_t>& b, const uint32_t& offset)
    {
        if (b.size() % sizeof(uint32_t) != 0) {
            throw std::runtime_error("Byte size not a multiple of 4");
        }
        uint32_t out;
        // Safe: copying from bytes into properly aligned uint32_t storage
        std::memcpy(&out, b.data() + offset, 4);
        return out;
    }

    void fillBaseItem(InventoryItem& invItem) {
        auto itemOpt = findBaseItem(invItem.itemType, invItem.itemId);
        std::optional<BaseItem> baseItem = std::nullopt;
        if (itemOpt.has_value()) {
            baseItem = itemOpt.value();
        } else if (1330000 <= invItem.itemId && invItem.itemId < 1332000) {
            // Pyromancy Flame has different upgrade levels
            // Pyromancy Flame
            invItem.upgradeLevel = (invItem.itemId - 1330000) / 100;
            invItem.itemId = 1330000;
            baseItem = findBaseItem(invItem.itemType, invItem.itemId).value();
        } else if (1332000 <= invItem.itemId && invItem.itemId <= 1332500) {
            // Ascended Pyromancy Flame
            invItem.upgradeLevel = (invItem.itemId - 1332000) / 100;
            invItem.itemId = 1332000;
            baseItem = findBaseItem(invItem.itemType, invItem.itemId).value();
        } else if (311000 <= invItem.itemId && invItem.itemId <= 312705) {
            // Sword of Artorias cursed variations
            invItem.upgradeLevel = invItem.itemId % 100;
            invItem.itemId = 311000;
            baseItem = findBaseItem(invItem.itemType, invItem.itemId).value();
        } else {
            invItem.upgradeLevel = invItem.itemId % 100;
            uint32_t new_id = invItem.itemId - invItem.upgradeLevel;
            itemOpt = findBaseItem(invItem.itemType, new_id);
            if (itemOpt.has_value()) {
                invItem.itemId = new_id;
                baseItem = itemOpt.value();
            } else {
                invItem.infusion = new_id % 1000;
                new_id -= invItem.infusion;
                itemOpt = findBaseItem(invItem.itemType, new_id);
                if (itemOpt.has_value()) {
                    invItem.itemId = new_id;
                    baseItem = itemOpt.value();
                }
            }
        }

        if (baseItem.has_value()) {
            invItem.baseItem = baseItem.value();
            return;
        }

        invItem.knownItem = false;
        switch (invItem.itemType) {
            case 0:
                invItem.baseItem.category = ItemCategory::WeaponsShields;
                break;
            case 268435456:
                invItem.baseItem.category = ItemCategory::Armor;
                break;
            case 536870912:
                invItem.baseItem.category = ItemCategory::Rings;
                break;
            case 1073741824:
                if (invItem.itemId < 800) {
                    invItem.baseItem.category = ItemCategory::Consumables;
                } else if (1000 <= invItem.itemId && invItem.itemId < 2000) {
                    invItem.baseItem.category = ItemCategory::Materials;
                } else if (invItem.itemId > 3000) {
                    invItem.baseItem.category = ItemCategory::Spells;
                } else {
                    invItem.baseItem.category = ItemCategory::KeyItems;
                }
                break;
            default:
                invItem.baseItem.category = ItemCategory::Consumables;
                break;
        }
        invItem.baseItem.label = "Unknown " + std::to_string(invItem.itemId);
    }

    static MapId readMapId(ContentReader& reader) {
        MapId mapId;
        mapId.unk0 = reader.read_u8_le();
        mapId.unk1 = reader.read_u8_le();
        mapId.block = reader.read_u8_le();
        mapId.area = reader.read_u8_le();
        return mapId;
    }

    static Vector4 readVector4(ContentReader& reader) {
        Vector4 v;
        v.x = reader.read_f32_le();
        v.y = reader.read_f32_le();
        v.z = reader.read_f32_le();
        v.w = reader.read_f32_le();
        return v;
    }

    static Transform readTransform(ContentReader& reader) {
        Transform t;
        for (float& v : t.position) v = reader.read_f32_le();
        for (float& v : t.rotation) v = reader.read_f32_le();
        return t;
    }

    static void expectPos(const ContentReader& reader, size_t expected, const char* section) {
        if (reader.pos() != expected) {
            throw std::runtime_error(
                std::string("DSR slot layout mismatch after ") + section
                + " (offset " + std::to_string(reader.pos())
                + ", expected " + std::to_string(expected) + ")"
            );
        }
    }

    static SlotHeader readSlotHeader(ContentReader& reader) {
        SlotHeader h;
        h.version = reader.read_u32_le();
        h.mapId = readMapId(reader);
        h.playTimeMs = reader.read_u32_le();
        h.gameDataOffset = reader.read_u32_le();
        h.gameDataSize = reader.read_u32_le();
        h.playerGameDataOffset = reader.read_u32_le();
        h.playerGameDataSize = reader.read_u32_le();
        h.unkBlock1Offset = reader.read_u32_le();
        h.unkBlock0Size = reader.read_u32_le();
        h.positionBlockOffset = reader.read_u32_le();
        h.positionBlockSize = reader.read_u32_le();
        h.clearCountBlockOffset = reader.read_u32_le();
        h.clearCountBlockSize = reader.read_u32_le();
        h.eventFlagsOffset = reader.read_u32_le();
        h.eventFlagsSize = reader.read_u32_le();
        h.mapEntityStateOffset = reader.read_u32_le();
        h.mapEntityStateSize = reader.read_u32_le();
        h.warpBlockOffset = reader.read_u32_le();
        h.mapEntityStateAndWarpSize = reader.read_u32_le();
        h.networkBlockOffset = reader.read_u32_le();
        h.networkBlockSize = reader.read_u32_le();
        h.unkBlock2Offset = reader.read_u32_le();
        h.unkBlock2Size = reader.read_u32_le();
        return h;
    }

    static AttributeInfo readAttributeInfo(ContentReader& reader) {
        AttributeInfo a;
        a.unk00 = reader.read_u32_le();
        a.hp = reader.read_u32_le();
        a.hpMax = reader.read_u32_le();
        a.hpBase = reader.read_u32_le();
        a.mp = reader.read_u32_le();
        a.mpMax = reader.read_u32_le();
        a.mpBase = reader.read_u32_le();
        a.unk1C = reader.read_u32_le();
        a.stamina = reader.read_u32_le();
        a.staminaMax = reader.read_u32_le();
        a.staminaBase = reader.read_u32_le();
        a.unk2C = reader.read_u32_le();
        a.vitality = reader.read_u32_le();
        a.unk34 = reader.read_u32_le();
        a.attunement = reader.read_u32_le();
        a.unk3C = reader.read_u32_le();
        a.endurance = reader.read_u32_le();
        a.unk44 = reader.read_u32_le();
        a.strength = reader.read_u32_le();
        a.unk4C = reader.read_u32_le();
        a.dexterity = reader.read_u32_le();
        a.unk54 = reader.read_u32_le();
        a.intelligence = reader.read_u32_le();
        a.unk5C = reader.read_u32_le();
        a.faith = reader.read_u32_le();
        a.unk64 = reader.read_u32_le();
        a.unk68 = reader.read_u32_le();
        a.unk6C = reader.read_u32_le();
        a.unk70 = reader.read_u32_le();
        a.humanity = reader.read_u32_le();
        a.resistance = reader.read_u32_le();
        a.unk7C = reader.read_u32_le();
        a.level = reader.read_u32_le();
        a.souls = reader.read_u32_le();
        a.soulMemory = reader.read_u64_le();
        a.unk90 = reader.read_u32_le();
        a.hollowState = reader.read_u32_le();
        a.name = reader.read_u16_string(17);
        a.gender = reader.read_u8_le();
        a.unkBB = reader.read_bytes<3>();
        a.classId = reader.read_u8_le();
        a.physiqueId = reader.read_u8_le();
        a.giftId = reader.read_u8_le();
        a.unkC1 = reader.read_bytes<3>();
        a.multiplayerCount = reader.read_u32_le();
        a.coopSuccessCount = reader.read_u32_le();
        a.unkCC = reader.read_bytes<0xE>();
        a.covenantLevels = reader.read_bytes<10>();
        a.unkE4 = reader.read_bytes<0xC>();
        a.toxicRes = reader.read_u32_le();
        a.bleedRes = reader.read_u32_le();
        a.poisonRes = reader.read_u32_le();
        a.curseRes = reader.read_u32_le();
        a.unk102 = reader.read_bytes<3>();
        a.covenantId = reader.read_u8_le();
        a.faceId = reader.read_u8_le();
        a.hairStyleId = reader.read_u8_le();
        a.hairColorId = reader.read_u8_le();
        a.unk107 = reader.read_u8_le();
        a.unk108 = reader.read_bytes<4>();
        a.unk10C = reader.read_u32_le();
        a.unk110 = reader.read_u32_le();
        a.unk114 = reader.read_u32_le();
        a.unk118 = reader.read_u32_le();
        a.name2 = reader.read_u16_string(17);
        a.unk13E = reader.read_bytes<0xA>();
        reader.copyTo(a.gameVersion.data(), a.gameVersion.size());
        a.unk150 = reader.read_bytes<0x54>();
        return a;
    }

    static PlayerGameData readPlayerGameData(ContentReader& reader, const SlotHeader& header) {
        PlayerGameData pgd;
        reader.seek(header.playerGameDataOffset);
        pgd.attributes = readAttributeInfo(reader);
        expectPos(reader, header.playerGameDataOffset + 0x1A4, "AttributeInfo");

        pgd.unk1A4 = reader.read_u32_le();
        for (UnkIdRecord& rec : pgd.unkRecords) {
            rec.id = reader.read_i32_le();
            rec.value = reader.read_u32_le();
            rec.unk08 = reader.read_u32_le();
            rec.unk0C = reader.read_u32_le();
        }

        // EquipGameData
        for (int32_t& idx : pgd.equipItemToInventoryIndex) idx = reader.read_i32_le();
        pgd.chrAsm.equippedWeaponStyle = reader.read_u32_le();
        pgd.chrAsm.lHandEquippedIndex = reader.read_u32_le();
        pgd.chrAsm.rHandEquippedIndex = reader.read_u32_le();
        for (uint32_t& v : pgd.chrAsm.unk0C) v = reader.read_u32_le();
        for (uint32_t& v : pgd.chrAsm.equipItems) v = reader.read_u32_le();

        EquipInventoryData& inv = pgd.inventory;
        inv.normalItemsCount = reader.read_u32_le();
        inv.keyItemsCount = reader.read_u32_le();
        inv.itemsCapacity = reader.read_u32_le();
        if (inv.itemsCapacity != INVENTORY_ITEMS_COUNT) {
            throw std::runtime_error("Unexpected DSR inventory capacity");
        }
        inv.items.resize(inv.itemsCapacity);
        for (EquipInventoryItem& item : inv.items) {
            item.category = reader.read_u32_le();
            item.itemId = reader.read_u32_le();
            item.quantity = reader.read_u32_le();
            item.sortId = reader.read_u32_le();
            item.isValid = reader.read_u32_le();
            item.durability = reader.read_u32_le();
            item.durabilityHits = reader.read_u32_le();
        }
        inv.itemsEndIndex = reader.read_u32_le();

        for (MagicSlot& slot : pgd.magic.magicSlots) {
            slot.magicId = reader.read_i32_le();
            slot.count = reader.read_u32_le();
        }
        pgd.magic.selectedSlot = reader.read_i32_le();

        for (int32_t& idx : pgd.quickItems.quickSlots) idx = reader.read_i32_le();
        pgd.quickItems.selectedQuickSlotItem = reader.read_i32_le();

        for (GestureSlot& slot : pgd.gestureSlots) {
            slot.gestureId = reader.read_u16_le();
            slot.index = reader.read_u16_le();
        }
        pgd.unkGestureSlots = reader.read_u32_le();

        pgd.face.hairColor = readVector4(reader);
        pgd.face.eyeColor = readVector4(reader);
        pgd.face.data = reader.read_bytes<FACE_DATA_SIZE>();
        pgd.face.unk70 = reader.read_u32_le();

        StorageBoxData& box = pgd.storageBox;
        for (uint32_t& v : box.unk00) v = reader.read_u32_le();
        box.items.resize(STORAGE_BOX_ITEMS_COUNT);
        for (StorageItem& item : box.items) {
            item.itemId = reader.read_u32_le();
            item.sortId = reader.read_u32_le();
            item.quantity = reader.read_u16_le();
            item.isValid = reader.read_u16_le();
            item.durability = reader.read_u32_le();
            item.durabilityHits = reader.read_u32_le();
            for (uint32_t& v : item.unk14) v = reader.read_u32_le();
        }
        box.unkEnd = reader.read_u32_le();

        for (GestureEntry& gesture : pgd.gestures.gestures) {
            gesture.value = reader.read_u16_le();
            gesture.index = reader.read_u16_le();
        }
        pgd.gestures.terminator = reader.read_i32_le();

        uint32_t regionsCount = reader.read_u32_le();
        pgd.unkRegionsHeader = reader.read_u32_le();
        size_t pgdEnd = static_cast<size_t>(header.playerGameDataOffset) + header.playerGameDataSize;
        if (reader.pos() + static_cast<size_t>(regionsCount) * 4 > pgdEnd) {
            throw std::runtime_error("DSR regions count out of range");
        }
        pgd.regionIds.reserve(regionsCount);
        for (uint32_t i = 0; i < regionsCount; ++i) {
            pgd.regionIds.push_back(reader.read_u32_le());
        }
        expectPos(reader, pgdEnd, "PlayerGameData");
        return pgd;
    }

    CharacterSlot parse_character_slot(const std::vector<uint8_t>& content) {
        ContentReader reader(content);
        CharacterSlot slot;
        SlotHeader& h = slot.header;
        h = readSlotHeader(reader);
        if (h.version != SLOT_VERSION) {
            throw std::runtime_error("Unsupported DSR slot version " + std::to_string(h.version));
        }

        slot.playerGameData = readPlayerGameData(reader, h);

        slot.unkBlock0 = reader.read_bytes<0x40>();
        expectPos(reader, h.positionBlockOffset, "unkBlock0");

        PositionBlock& pos = slot.positionBlock;
        pos.unk00 = reader.read_u8_le();
        for (Transform& t : pos.transforms) t = readTransform(reader);
        for (int32_t& v : pos.unk31) v = reader.read_i32_le();
        pos.mapId = readMapId(reader);
        pos.unk41 = reader.read_u32_le();
        pos.unk45 = reader.read_u32_le();
        pos.unk49 = reader.read_u32_le();
        pos.unk4D = reader.read_bytes<0x2D>();
        for (int32_t& v : pos.unk7A) v = reader.read_i32_le();
        pos.unk8A = reader.read_u32_le();
        expectPos(reader, h.clearCountBlockOffset, "positionBlock");

        slot.clearCountBlock.unk00 = reader.read_u32_le();
        slot.clearCountBlock.clearCount = reader.read_u32_le();
        expectPos(reader, h.unkBlock1Offset, "clearCountBlock");

        for (UnkRecord16& rec : slot.unkBlock1.records16) {
            rec.unk00 = reader.read_i32_le();
            rec.unk04 = reader.read_i32_le();
            rec.unk08 = reader.read_i32_le();
            rec.unk0C = reader.read_u32_le();
        }
        for (UnkRecord8& rec : slot.unkBlock1.records8) {
            rec.unk00 = reader.read_i32_le();
            rec.unk04 = reader.read_u32_le();
        }
        slot.unkBlock1.unkC08 = reader.read_bytes<0x43>();
        expectPos(reader, h.eventFlagsOffset, "unkBlock1");

        if (h.eventFlagsSize != EVENT_FLAGS_SIZE + 1) {
            throw std::runtime_error("Unexpected DSR event flags size");
        }
        slot.eventFlags.flags = reader.read_vec_u8(EVENT_FLAGS_SIZE);
        slot.eventFlags.unkEnd = reader.read_u8_le();
        expectPos(reader, h.mapEntityStateOffset, "eventFlags");

        size_t mapStateEnd = static_cast<size_t>(h.mapEntityStateOffset) + h.mapEntityStateSize;
        while (reader.pos() < mapStateEnd) {
            uint32_t chunkSize = reader.read_u32_le();
            if (chunkSize < 4 || reader.pos() + chunkSize > mapStateEnd) {
                throw std::runtime_error("Invalid DSR map entity state chunk");
            }
            std::array<uint8_t, 4> fourcc = reader.read_bytes<4>();
            MapEntityStateChunk chunk;
            // Stored as little endian integer, e.g. " RHC" for "CHR "
            chunk.tag = std::string(fourcc.rbegin(), fourcc.rend());
            chunk.data = reader.read_vec_u8(chunkSize - 4);
            slot.mapEntityState.push_back(std::move(chunk));
        }
        expectPos(reader, h.warpBlockOffset, "mapEntityState");

        WarpBlock& warp = slot.warpBlock;
        warp.position = readVector4(reader);
        warp.rotation = readVector4(reader);
        warp.unk20 = reader.read_u32_le();
        for (uint32_t& v : warp.unk24) v = reader.read_u32_le();
        warp.stablePosition = readVector4(reader);
        warp.stableRotation = readVector4(reader);
        expectPos(reader, h.networkBlockOffset, "warpBlock");

        slot.networkBlock = reader.read_vec_u8(h.networkBlockSize);
        expectPos(reader, h.unkBlock2Offset, "networkBlock");
        slot.unkBlock2 = reader.read_vec_u8(h.unkBlock2Size);

        slot.trailingData = reader.read_vec_u8(reader.size() - reader.pos());
        return slot;
    }

    static MenuCharacterData readMenuCharacterData(ContentReader& reader) {
        MenuCharacterData m;
        m.name = reader.read_u16_string(17);
        m.unk22 = reader.read_u16_le();
        m.level = reader.read_u32_le();
        m.playTimeSeconds = reader.read_u32_le();
        m.soulMemory = reader.read_u32_le();
        m.mapId = readMapId(reader);
        m.placeNameId = reader.read_u32_le();
        m.unk38 = reader.read_bytes<0x28>();
        m.hairColor = readVector4(reader);
        m.eyeColor = readVector4(reader);
        m.faceData = reader.read_bytes<FACE_DATA_SIZE>();
        m.unkE4 = reader.read_bytes<0x1C>();
        m.unk100 = reader.read_bytes<0x24>();
        for (uint32_t& v : m.equipItems) v = reader.read_u32_le();
        m.unk174 = reader.read_bytes<0xC>();
        m.gender = reader.read_u8_le();
        m.classId = reader.read_u8_le();
        m.giftId = reader.read_u8_le();
        m.physiqueId = reader.read_u8_le();
        m.faceId = reader.read_u8_le();
        m.hairStyleId = reader.read_u8_le();
        m.hairColorId = reader.read_u8_le();
        m.unk187 = reader.read_u8_le();
        m.unk188 = reader.read_u32_le();
        m.unk18C = reader.read_u32_le();
        return m;
    }

    SystemData parse_system_data(const std::vector<uint8_t>& content) {
        ContentReader reader(content);
        SystemData data;
        data.unk00 = reader.read_bytes<0xB0>();
        data.occupiedSlots = reader.read_bytes<SLOT_COUNT>();
        data.unkBA = reader.read_bytes<6>();
        for (MenuCharacterData& m : data.menuCharacters) {
            m = readMenuCharacterData(reader);
        }
        data.unk1060 = reader.read_vec_u8(reader.size() - reader.pos());
        return data;
    }

    std::optional<std::pair<uint32_t, uint32_t>> eventFlagOffset(uint32_t flagId) {
        // Same mapping the game uses for its event flags memory, 8 digit id
        //   'GAAASNNN' (group, area, section, number).
        if (flagId > 99999999) return std::nullopt;
        uint32_t group = flagId / 10000000;
        uint32_t area = (flagId / 10000) % 1000;
        uint32_t section = (flagId / 1000) % 10;
        uint32_t number = flagId % 1000;

        uint32_t offset;
        switch (group) {
            case 0: offset = 0x00000; break;
            case 1: offset = 0x00500; break;
            case 5: offset = 0x05F00; break;
            case 6: offset = 0x0B900; break;
            case 7: offset = 0x11300; break;
            default: return std::nullopt;
        }
        static constexpr std::array<uint32_t, 18> AREAS = {
            0, 100, 101, 102, 110, 120, 121, 130, 131,
            132, 140, 141, 150, 151, 160, 170, 180, 181
        };
        auto it = std::find(AREAS.begin(), AREAS.end(), area);
        if (it == AREAS.end()) return std::nullopt;
        offset += static_cast<uint32_t>(it - AREAS.begin()) * 0x500;
        offset += section * 128;
        offset += (number - (number % 32)) / 8;
        uint32_t mask = 0x80000000u >> (number % 32);
        return std::make_pair(offset, mask);
    }

    std::optional<bool> DSRCharacterInfo::isEventFlagSet(uint32_t flagId) const {
        auto location = eventFlagOffset(flagId);
        if (!location.has_value()) return std::nullopt;
        const auto& [offset, mask] = location.value();
        const std::vector<uint8_t>& flags = slot.eventFlags.flags;
        if (offset + 4 > flags.size()) return std::nullopt;
        uint32_t value = read_u32_le(flags.data() + offset);
        return (value & mask) != 0;
    }

    static DSRCharacterInfo createCharacterInfo(int charIdx, CharacterSlot&& slot) {
        DSRCharacterInfo ci;
        ci.index = charIdx;
        const PlayerGameData& pgd = slot.playerGameData;
        const AttributeInfo& a = pgd.attributes;

        ci.name = a.name;
        ci.humanity = a.humanity;
        ci.souls = a.souls;
        ci.earnedSouls = a.soulMemory;
        ci.hollowState = a.hollowState;
        ci.covenantId = a.covenantId;
        ci.covenantLevels = a.covenantLevels;
        ci.hpCurrent = a.hp;
        ci.hpMax = a.hpMax;
        ci.hpBase = a.hpBase;
        ci.staminaCurrent = a.stamina;
        ci.staminaMax = a.staminaMax;
        ci.staminaBase = a.staminaBase;
        ci.level = a.level;
        ci.vitality = a.vitality;
        ci.attunement = a.attunement;
        ci.endurance = a.endurance;
        ci.strength = a.strength;
        ci.dexterity = a.dexterity;
        ci.resistance = a.resistance;
        ci.intelligence = a.intelligence;
        ci.faith = a.faith;
        ci.gender = a.gender;
        ci.classId = a.classId;
        ci.physiqueId = a.physiqueId;
        ci.giftId = a.giftId;
        ci.toxicRes = a.toxicRes;
        ci.bleedRes = a.bleedRes;
        ci.poisonRes = a.poisonRes;
        ci.curseRes = a.curseRes;
        ci.faceId = a.faceId;
        ci.hairStyleId = a.hairStyleId;
        ci.hairColorId = a.hairColorId;

        const auto& equip = pgd.chrAsm.equipItems;
        auto equipItem = [&equip](EquipSlot s) { return equip[static_cast<size_t>(s)]; };
        ci.lHandSlot1 = equipItem(EquipSlot::LeftHand1);
        ci.lHandSlot2 = equipItem(EquipSlot::LeftHand2);
        ci.rHandSlot1 = equipItem(EquipSlot::RightHand1);
        ci.rHandSlot2 = equipItem(EquipSlot::RightHand2);
        ci.lArrowsSlot = equipItem(EquipSlot::Arrows1);
        ci.lBoltsSlot = equipItem(EquipSlot::Bolts1);
        ci.rArrowsSlot = equipItem(EquipSlot::Arrows2);
        ci.rBoltsSlot = equipItem(EquipSlot::Bolts2);
        ci.head_slot = equipItem(EquipSlot::Head);
        ci.body_slot = equipItem(EquipSlot::Body);
        ci.arms_slot = equipItem(EquipSlot::Arms);
        ci.legs_slot = equipItem(EquipSlot::Legs);
        ci.hairSlot = equipItem(EquipSlot::Hair);
        ci.lRingSlot = equipItem(EquipSlot::Ring1);
        ci.rRingSlot = equipItem(EquipSlot::Ring2);
        ci.q1Slot = equipItem(EquipSlot::Quick1);
        ci.q2Slot = equipItem(EquipSlot::Quick2);
        ci.q3Slot = equipItem(EquipSlot::Quick3);
        ci.q4Slot = equipItem(EquipSlot::Quick4);
        ci.q5Slot = equipItem(EquipSlot::Quick5);
        ci.playTimeMs = slot.header.playTimeMs;
        ci.clearCount = slot.clearCountBlock.clearCount;

        const auto& invItems = pgd.inventory.items;
        ci.inventoryItems.reserve(pgd.inventory.normalItemsCount + pgd.inventory.keyItemsCount);
        for (uint32_t idx = 0; idx < invItems.size(); ++idx) {
            const EquipInventoryItem& item = invItems[idx];
            if (item.itemId == EMPTY_ITEM_ID) continue;
            InventoryItem invItem;
            invItem.itemType = item.category;
            invItem.itemId = item.itemId;
            invItem.amount = item.quantity;
            invItem.order = item.sortId;
            invItem.durability = item.durability;
            invItem.idx = idx;
            fillBaseItem(invItem);
            ci.inventoryItems.push_back(invItem);
        }

        ci.attunementSlots.reserve(ATTUNEMENT_SLOTS_COUNT);
        for (const MagicSlot& magicSlot : pgd.magic.magicSlots) {
            ci.attunementSlots.push_back({
                static_cast<uint32_t>(magicSlot.magicId),
                magicSlot.count
            });
        }

        for (size_t i = 0; i < GESTURES_COUNT; ++i) {
            ci.unlockedGestures[i] = pgd.gestures.gestures[i].isUnlocked();
        }

        const auto& boxItems = pgd.storageBox.items;
        for (uint32_t idx = 0; idx < boxItems.size(); ++idx) {
            const StorageItem& item = boxItems[idx];
            if (item.itemId == EMPTY_ITEM_ID) continue;
            InventoryItem invItem;
            invItem.itemId = item.itemId;
            invItem.order = item.sortId;
            invItem.amount = item.quantity;
            invItem.durability = item.durability;
            invItem.idx = idx;
            // Category is stored in the upper bits of the item id
            if (invItem.itemId >= TYPE_OTHER) {
                invItem.itemType = TYPE_OTHER;
            } else if (invItem.itemId >= TYPE_RING) {
                invItem.itemType = TYPE_RING;
            } else if (invItem.itemId >= TYPE_ARMOR) {
                invItem.itemType = TYPE_ARMOR;
            } else {
                invItem.itemType = TYPE_WEAPON;
            }
            invItem.itemId -= invItem.itemType;
            fillBaseItem(invItem);
            ci.bottomlessBoxItems.push_back(invItem);
        }

        ci.slot = std::move(slot);
        return ci;
    }

    DSRSaveFile parse_dsr_file(const SL2File& sl2) {
        if (sl2.entries.size() < SLOT_COUNT + 1) {
            throw std::runtime_error("DSR save file is missing entries");
        }
        DSRSaveFile saveFile;
        saveFile.sideCarEnty = sl2.entries[SLOT_COUNT];
        saveFile.systemData = parse_system_data(saveFile.sideCarEnty.content);

        saveFile.characters.reserve(SLOT_COUNT);
        for (uint32_t charIdx = 0; charIdx < SLOT_COUNT; ++charIdx) {
            if (saveFile.systemData.occupiedSlots[charIdx] == 0) continue;
            CharacterSlot slot = parse_character_slot(sl2.entries[charIdx].content);
            saveFile.characters.push_back(createCharacterInfo(static_cast<int>(charIdx), std::move(slot)));
        }
        return saveFile;
    }
}
