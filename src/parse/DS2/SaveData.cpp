#include "SaveData.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fssm::parse::ds2 {
    namespace {
        void ensure(bool cond, const std::string& msg) {
            if (!cond) throw std::runtime_error("DS2 save layout mismatch: " + msg);
        }

        template <class T>
        std::optional<T> read_record(const SaveEntry& entry, RecordType type) {
            static_assert(std::is_trivially_copyable_v<T>);
            const Record* record = entry.find(type);
            if (record == nullptr || record->data.size() != sizeof(T)) return std::nullopt;
            T out;
            std::memcpy(&out, record->data.data(), sizeof(T));
            return out;
        }

        constexpr size_t OBJ_MAP_SIZE = 0x6008;
        constexpr size_t OBJ_GENERATOR_SIZE = 0xc08;
    }

    const Record* SaveEntry::find(RecordType type) const {
        for (const auto& record : records) {
            if (record.type() == type) return &record;
        }
        return nullptr;
    }

    SaveEntry parse_entry(const std::vector<uint8_t>& data) {
        SaveEntry entry;
        size_t pos = 0;
        while (true) {
            ensure(pos + sizeof(RecordHeader) <= data.size(), "record header past end");
            Record record;
            std::memcpy(&record.header, data.data() + pos, sizeof(RecordHeader));
            pos += sizeof(RecordHeader);
            ensure(record.header.type <= static_cast<uint32_t>(RecordType::GetObjItem),
                   "unknown record type " + std::to_string(record.header.type));
            ensure(pos + record.header.size <= data.size(), "record payload past end");
            record.data.assign(data.begin() + pos, data.begin() + pos + record.header.size);
            pos += record.header.size;
            const bool isEnd = record.type() == RecordType::End;
            entry.records.push_back(std::move(record));
            if (isEnd) break;
        }
        entry.trailing.assign(data.begin() + pos, data.end());
        return entry;
    }

    std::vector<uint8_t> serialize_entry(const SaveEntry& entry) {
        std::vector<uint8_t> out;
        for (const auto& record : entry.records) {
            RecordHeader header = record.header;
            header.size = static_cast<uint32_t>(record.data.size());
            const auto* h = reinterpret_cast<const uint8_t*>(&header);
            out.insert(out.end(), h, h + sizeof(header));
            out.insert(out.end(), record.data.begin(), record.data.end());
        }
        out.insert(out.end(), entry.trailing.begin(), entry.trailing.end());
        return out;
    }

    std::optional<LoadInfo> read_load_info(const SaveEntry& entry) {
        return read_record<LoadInfo>(entry, RecordType::LoadInfo);
    }

    std::optional<PlayerData> read_player(const SaveEntry& entry) {
        return read_record<PlayerData>(entry, RecordType::Player);
    }

    std::optional<GameData> read_game(const SaveEntry& entry) {
        return read_record<GameData>(entry, RecordType::Game);
    }

    std::optional<EnemyData> read_enemy(const SaveEntry& entry) {
        return read_record<EnemyData>(entry, RecordType::Enemy);
    }

    std::optional<InventoryData> read_inventory(const SaveEntry& entry) {
        return read_record<InventoryData>(entry, RecordType::ItemInventory);
    }

    std::optional<EventData> read_event(const SaveEntry& entry) {
        return read_record<EventData>(entry, RecordType::Event);
    }

    std::optional<bool> get_event_flag(const EventData& event, uint32_t flagId) {
        const uint32_t group = flagId / 10000;
        const uint32_t bit = flagId % 10000;
        const std::array<uint8_t, 0x4e2>* flags = group == 10     ? &event.flags10
                                                : group == 1000 ? &event.flags1000
                                                                : nullptr;
        if (flags == nullptr || bit / 8 >= flags->size()) return std::nullopt;
        return ((*flags)[bit / 8] & (0x80 >> (bit % 8))) != 0;
    }

    bool is_bonfire_lit(const EventData& event, uint16_t bonfireId) {
        for (size_t i = 0; i < event.bonfireIds.size(); ++i) {
            if (event.bonfireIds[i] == bonfireId) return (event.bonfireLit[i] & 1) != 0;
        }
        return false;
    }

    std::optional<ObjData> read_obj(const SaveEntry& entry) {
        const Record* record = entry.find(RecordType::Obj);
        if (record == nullptr) return std::nullopt;
        const auto& d = record->data;
        size_t pos = 0;
        auto read_u32 = [&](uint32_t& v) {
            if (pos + 4 > d.size()) return false;
            std::memcpy(&v, d.data() + pos, 4);
            pos += 4;
            return true;
        };
        ObjData obj;
        uint32_t mapCount = 0;
        if (!read_u32(mapCount) || mapCount > 3 || pos + mapCount * OBJ_MAP_SIZE > d.size()) return std::nullopt;
        obj.maps.resize(mapCount);
        for (auto& map : obj.maps) {
            std::memcpy(map.data(), d.data() + pos, OBJ_MAP_SIZE);
            pos += OBJ_MAP_SIZE;
        }
        uint32_t generatorCount = 0;
        if (!read_u32(generatorCount) || pos + size_t(generatorCount) * OBJ_GENERATOR_SIZE > d.size()) {
            return std::nullopt;
        }
        obj.generators.resize(generatorCount);
        for (auto& gen : obj.generators) {
            std::memcpy(gen.data(), d.data() + pos, OBJ_GENERATOR_SIZE);
            pos += OBJ_GENERATOR_SIZE;
        }
        obj.unwritten.assign(d.begin() + pos, d.end());
        return obj;
    }
}
