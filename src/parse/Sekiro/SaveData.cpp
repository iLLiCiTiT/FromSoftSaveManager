#include "SaveData.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace fssm::parse::sekiro {
    namespace {
        void ensure(bool cond, const std::string& msg) {
            if (!cond) throw std::runtime_error("Sekiro save layout mismatch: " + msg);
        }
    }

    const Record* SaveEntry::find(RecordId id) const {
        for (const auto& record : records) {
            if (record.id() == id) return &record;
        }
        return nullptr;
    }

    SaveEntry parse_entry(const std::vector<uint8_t>& data) {
        SaveEntry entry;
        if (std::all_of(data.begin(), data.end(), [](uint8_t b) { return b == 0; })) {
            entry.empty = true;
            entry.trailing = data;
            return entry;
        }
        size_t pos = 0;
        while (true) {
            ensure(pos + sizeof(RecordHeader) <= data.size(), "record header past end");
            Record record;
            std::memcpy(&record.header, data.data() + pos, sizeof(RecordHeader));
            pos += sizeof(RecordHeader);
            ensure(pos + record.header.size <= data.size(),
                   "record " + std::to_string(record.header.id) + " payload past end");
            record.data.assign(data.begin() + pos, data.begin() + pos + record.header.size);
            pos += record.header.size;
            const bool isEnd = record.id() == RecordId::End;
            entry.records.push_back(std::move(record));
            if (isEnd) break;
        }
        entry.trailing.assign(data.begin() + pos, data.end());
        return entry;
    }

    std::optional<GaitemData> read_gaitem(const SaveEntry& entry) {
        const Record* record = entry.find(RecordId::Gaitem);
        if (record == nullptr || record->data.size() != sizeof(GaitemData)) return std::nullopt;
        GaitemData out;
        std::memcpy(&out, record->data.data(), sizeof(GaitemData));
        return out;
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
}
