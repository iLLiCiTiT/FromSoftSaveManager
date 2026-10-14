#include "SaveData.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fssm::parse::dsr {
    namespace {
        void ensure(bool cond, const std::string& msg) {
            if (!cond) throw std::runtime_error("DSR slot layout mismatch: " + msg);
        }

        class SlotReader {
        public:
            explicit SlotReader(const std::vector<uint8_t>& data) : m_data(data), m_pos(0) {}

            void raw(void* dest, size_t n) {
                ensure(m_pos + n <= m_data.size(), "read past end at " + std::to_string(m_pos));
                std::memcpy(dest, m_data.data() + m_pos, n);
                m_pos += n;
            }
            template <class T>
            T read() {
                static_assert(std::is_trivially_copyable_v<T>);
                T v;
                raw(&v, sizeof(T));
                return v;
            }
            template <class T>
            void read(T& v) {
                static_assert(std::is_trivially_copyable_v<T>);
                raw(&v, sizeof(T));
            }
            std::vector<uint8_t> bytes(size_t n) {
                std::vector<uint8_t> v(n);
                if (n) raw(v.data(), n);
                return v;
            }
            size_t tell() const { return m_pos; }
            void seek(size_t pos) { m_pos = pos; }
            size_t size() const { return m_data.size(); }

        private:
            const std::vector<uint8_t>& m_data;
            size_t m_pos;
        };

        class SlotWriter {
        public:
            void raw(const void* src, size_t n) {
                auto p = static_cast<const uint8_t*>(src);
                m_data.insert(m_data.end(), p, p + n);
            }
            template <class T>
            void write(const T& v) {
                static_assert(std::is_trivially_copyable_v<T>);
                raw(&v, sizeof(T));
            }
            void bytes(const std::vector<uint8_t>& v) { raw(v.data(), v.size()); }
            void zeros(size_t n) { m_data.insert(m_data.end(), n, 0); }
            size_t tell() const { return m_data.size(); }
            std::vector<uint8_t>& data() { return m_data; }

        private:
            std::vector<uint8_t> m_data;
        };

        // ---------------------------------------------------------------- PlayerGameData

        PlayerGameData read_player_game_data(SlotReader& r) {
            PlayerGameData p;
            r.read(p.stats);
            r.read(p.unk_1f0);
            r.read(p.equip);
            r.read(p.face);
            r.read(p.repository);
            r.read(p.gestures);
            const auto count = r.read<uint64_t>();
            ensure(count <= 0x10000, "PlayerGameData vector count " + std::to_string(count));
            p.unk_618.resize(count);
            if (count) r.raw(p.unk_618.data(), count * sizeof(uint32_t));
            return p;
        }

        void write_player_game_data(SlotWriter& w, const PlayerGameData& p) {
            w.write(p.stats);
            w.write(p.unk_1f0);
            w.write(p.equip);
            w.write(p.face);
            w.write(p.repository);
            w.write(p.gestures);
            w.write<uint64_t>(p.unk_618.size());
            w.raw(p.unk_618.data(), p.unk_618.size() * sizeof(uint32_t));
        }

        // ---------------------------------------------------------------- Map state blobs

        constexpr uint64_t CHR_BITS_MAGIC = 0x1008030043485220;    // " RHC\0\x03\x08\x10"
        constexpr uint64_t OBJ_BITS_MAGIC = 0x100512004f424a20;    // " JBO\0\x12\x05\x10"
        constexpr uint32_t MAP_BITS_END = 0xffffffff;

        MapBitsBlob read_map_bits(SlotReader& r, uint64_t expectedMagic) {
            MapBitsBlob blob;
            const auto size = r.read<uint32_t>();
            if (size == 0) return blob;
            const size_t start = r.tell();
            blob.present = true;
            blob.magic = r.read<uint64_t>();
            ensure(blob.magic == expectedMagic, "map bits magic");
            while (true) {
                const auto mapId = r.read<uint32_t>();
                if (mapId == MAP_BITS_END) {
                    ensure(r.read<uint32_t>() == 0 && r.read<uint32_t>() == 0, "map bits terminator");
                    break;
                }
                MapBitsEntry e;
                e.mapId = mapId;
                const auto entrySize = r.read<uint32_t>();
                e.bitCount = r.read<uint32_t>();
                ensure(entrySize == ((e.bitCount + 7) >> 3) + 0xc, "map bits entry size");
                e.bits = r.bytes(entrySize - 0xc);
                blob.entries.push_back(std::move(e));
            }
            ensure(r.tell() - start == size, "map bits blob size");
            return blob;
        }

        void write_map_bits(SlotWriter& w, const MapBitsBlob& blob) {
            if (!blob.present) {
                w.write<uint32_t>(0);
                return;
            }
            uint32_t size = 8 + 12;
            for (const auto& e : blob.entries) size += 12 + static_cast<uint32_t>(e.bits.size());
            w.write(size);
            w.write(blob.magic);
            for (const auto& e : blob.entries) {
                w.write(e.mapId);
                w.write<uint32_t>(static_cast<uint32_t>(e.bits.size()) + 0xc);
                w.write(e.bitCount);
                w.bytes(e.bits);
            }
            w.write(MAP_BITS_END);
            w.write<uint32_t>(0);
            w.write<uint32_t>(0);
        }

        // ---------------------------------------------------------------- Net data chunks

        constexpr size_t NET_CHUNK_BUFFER_SIZE = 0x20000;
        constexpr uint16_t NET_CHUNK_END = 0xffff;

        size_t align4(size_t v) { return (v + 3) & ~size_t(3); }

        NetDataChunks read_net_chunks(SlotReader& r) {
            NetDataChunks out;
            out.unk_00 = r.read<uint32_t>();
            const size_t base = r.tell();
            size_t pos = 0;
            while (true) {
                pos = align4(pos);
                ensure(pos + 8 <= NET_CHUNK_BUFFER_SIZE, "net chunk header out of buffer");
                r.seek(base + pos);
                const auto id = r.read<uint16_t>();
                const auto version = r.read<uint16_t>();
                const auto size = r.read<uint32_t>();
                pos += 8;
                if (id == NET_CHUNK_END) {
                    out.endVersion = version;
                    out.endSize = size;
                    break;
                }
                ensure(pos + size <= NET_CHUNK_BUFFER_SIZE, "net chunk data out of buffer");
                out.chunks.push_back({id, version, r.bytes(size)});
                pos += size;
            }
            // Alignment padding and the rest of the buffer must be zero for an exact round trip.
            r.seek(base);
            const auto buffer = r.bytes(NET_CHUNK_BUFFER_SIZE);
            SlotWriter check;
            for (const auto& c : out.chunks) {
                check.zeros(align4(check.tell()) - check.tell());
                check.write(c.id);
                check.write(c.version);
                check.write<uint32_t>(static_cast<uint32_t>(c.data.size()));
                check.bytes(c.data);
            }
            check.zeros(align4(check.tell()) - check.tell());
            check.write(NET_CHUNK_END);
            check.write(out.endVersion);
            check.write(out.endSize);
            check.zeros(NET_CHUNK_BUFFER_SIZE - check.tell());
            ensure(check.data() == buffer, "net chunk buffer has unexpected padding");
            return out;
        }

        void write_net_chunks(SlotWriter& w, const NetDataChunks& in) {
            w.write(in.unk_00);
            const size_t base = w.tell();
            auto pad = [&] {
                const size_t rel = w.tell() - base;
                w.zeros(align4(rel) - rel);
            };
            for (const auto& c : in.chunks) {
                pad();
                w.write(c.id);
                w.write(c.version);
                w.write<uint32_t>(static_cast<uint32_t>(c.data.size()));
                w.bytes(c.data);
            }
            pad();
            w.write(NET_CHUNK_END);
            w.write(in.endVersion);
            w.write(in.endSize);
            const size_t used = w.tell() - base;
            if (used > NET_CHUNK_BUFFER_SIZE) throw std::runtime_error("net chunks exceed 0x20000 bytes");
            w.zeros(NET_CHUNK_BUFFER_SIZE - used);
        }
    }

    bool is_empty_slot(const std::vector<uint8_t>& data) {
        return std::all_of(data.begin(), data.end(), [](uint8_t b) { return b == 0; });
    }

    // Mirrors FUN_1402c61c0 (writer) / FUN_1402c6760 (reader).
    CharacterSlot parse_character_slot(const std::vector<uint8_t>& data) {
        ensure(data.size() == SLOT_SIZE, "slot size");
        SlotReader r(data);
        CharacterSlot s;
        r.read(s.header);
        const auto& h = s.header;
        ensure(h.version == CHARACTER_SLOT_VERSION, "unsupported slot version " + std::to_string(h.version));

        // Section 1: GameDataMan (FUN_140757990)
        ensure(r.tell() == h.gameDataOffset && r.tell() == h.playerGameDataOffset, "section 1 offset");
        s.player = read_player_game_data(r);
        ensure(r.tell() - h.playerGameDataOffset == h.playerGameDataSize, "PlayerGameData size");

        const size_t qwcStart = r.tell();
        r.read(s.qwc);
        ensure(r.tell() - qwcStart == h.qwcSize, "QwcData size");

        ensure(r.tell() == h.blobs44Offset, "blob44 offset");
        s.hasBlob44a = r.read<uint8_t>() != 0;
        s.blob44a = {};
        s.unk_48 = 0;
        if (s.hasBlob44a) {
            r.read(s.blob44a);
            s.unk_48 = r.read<uint32_t>();
        }
        s.hasBlob44b = r.read<uint8_t>() != 0;
        s.blob44b = {};
        if (s.hasBlob44b) r.read(s.blob44b);
        ensure(r.tell() - h.blobs44Offset == h.blobs44Size, "blob44 size");

        ensure(r.tell() == h.unk2u32Offset, "unk2u32 offset");
        s.unk_78 = r.read<uint32_t>();
        s.unk_7c = r.read<uint32_t>();

        ensure(r.tell() == h.trophyOffset, "TrophyEquipData offset");
        r.read(s.trophy);
        r.read(s.scalars);
        ensure(r.tell() == h.gameDataOffset + h.gameDataSize, "section 1 size");

        // Section 2: event flags (FUN_1404f5870), raw flag buffer + 1 byte.
        ensure(r.tell() == h.eventFlagsOffset && h.eventFlagsSize >= 1, "event flags offset");
        s.eventFlags = r.bytes(h.eventFlagsSize - 1);
        s.eventFlagsUnk = r.read<uint8_t>();

        // Section 3: map state (FUN_1402c6eb0)
        ensure(r.tell() == h.mapStateOffset, "map state offset");
        s.chrBits = read_map_bits(r, CHR_BITS_MAGIC);
        s.objBits = read_map_bits(r, OBJ_BITS_MAGIC);
        ensure(r.tell() - h.mapStateOffset == h.mapStateSize, "map state size");

        // Section 4 (FUN_1402c7370). Its stored size is measured from the map state
        // offset instead of its own start.
        ensure(r.tell() == h.saveLoadOffset, "section 4 offset");
        r.read(s.saveLoad);
        ensure(r.tell() - h.mapStateOffset == h.saveLoadSize, "section 4 size");

        // Section 5 (FUN_14077cf60)
        ensure(r.tell() == h.netChunksOffset, "net chunks offset");
        s.netChunks = read_net_chunks(r);
        ensure(r.tell() - h.netChunksOffset == h.netChunksSize, "net chunks size");

        // Section 6
        ensure(r.tell() == h.trailerOffset && h.trailerSize == sizeof(s.unk_c5c), "section 6");
        r.read(s.unk_c5c);

        // Version gated tail (FUN_140757e40, FUN_1402ca9a0, FUN_140757ec0)
        auto& t = s.tail;
        t.unk_a1 = r.read<uint8_t>();
        t.unk_a8 = r.read<uint32_t>();
        t.unk_b32 = r.read<uint8_t>();
        t.unk_b34 = r.read<uint32_t>();
        t.unk_b38 = r.read<uint32_t>();
        t.unk_bf3 = r.read<uint8_t>();
        r.read(t.unk_d0);
        t.unk_flag = r.read<uint8_t>();

        s.trailingGarbage = r.bytes(r.size() - r.tell());
        return s;
    }

    std::vector<uint8_t> serialize_character_slot(const CharacterSlot& s) {
        SlotWriter w;
        CharacterSlotHeader h = s.header;
        w.write(h);

        h.gameDataOffset = static_cast<uint32_t>(w.tell());
        h.playerGameDataOffset = h.gameDataOffset;
        write_player_game_data(w, s.player);
        h.playerGameDataSize = static_cast<uint32_t>(w.tell() - h.playerGameDataOffset);

        const size_t qwcStart = w.tell();
        w.write(s.qwc);
        h.qwcSize = static_cast<uint32_t>(w.tell() - qwcStart);

        h.blobs44Offset = static_cast<uint32_t>(w.tell());
        w.write<uint8_t>(s.hasBlob44a ? 1 : 0);
        if (s.hasBlob44a) {
            w.write(s.blob44a);
            w.write(s.unk_48);
        }
        w.write<uint8_t>(s.hasBlob44b ? 1 : 0);
        if (s.hasBlob44b) w.write(s.blob44b);
        h.blobs44Size = static_cast<uint32_t>(w.tell() - h.blobs44Offset);

        h.unk2u32Offset = static_cast<uint32_t>(w.tell());
        w.write(s.unk_78);
        w.write(s.unk_7c);
        h.unk2u32Size = static_cast<uint32_t>(w.tell() - h.unk2u32Offset);

        h.trophyOffset = static_cast<uint32_t>(w.tell());
        w.write(s.trophy);
        w.write(s.scalars);
        h.gameDataSize = static_cast<uint32_t>(w.tell() - h.gameDataOffset);

        h.eventFlagsOffset = static_cast<uint32_t>(w.tell());
        w.bytes(s.eventFlags);
        w.write(s.eventFlagsUnk);
        h.eventFlagsSize = static_cast<uint32_t>(w.tell() - h.eventFlagsOffset);

        h.mapStateOffset = static_cast<uint32_t>(w.tell());
        write_map_bits(w, s.chrBits);
        write_map_bits(w, s.objBits);
        h.mapStateSize = static_cast<uint32_t>(w.tell() - h.mapStateOffset);

        h.saveLoadOffset = static_cast<uint32_t>(w.tell());
        w.write(s.saveLoad);
        h.saveLoadSize = static_cast<uint32_t>(w.tell() - h.mapStateOffset);

        h.netChunksOffset = static_cast<uint32_t>(w.tell());
        write_net_chunks(w, s.netChunks);
        h.netChunksSize = static_cast<uint32_t>(w.tell() - h.netChunksOffset);

        h.trailerOffset = static_cast<uint32_t>(w.tell());
        w.write(s.unk_c5c);
        h.trailerSize = sizeof(s.unk_c5c);

        const auto& t = s.tail;
        w.write(t.unk_a1);
        w.write(t.unk_a8);
        w.write(t.unk_b32);
        w.write(t.unk_b34);
        w.write(t.unk_b38);
        w.write(t.unk_bf3);
        w.write(t.unk_d0);
        w.write(t.unk_flag);

        if (w.tell() + s.trailingGarbage.size() > SLOT_SIZE) {
            throw std::runtime_error("DSR character slot exceeds 0x60000 bytes");
        }
        w.bytes(s.trailingGarbage);
        w.zeros(SLOT_SIZE - w.tell());

        auto& out = w.data();
        std::memcpy(out.data(), &h, sizeof(h));
        return out;
    }

    // Mirrors FUN_140758570 (writer) / FUN_1407586a0 (reader).
    SystemSlot parse_system_slot(const std::vector<uint8_t>& data) {
        ensure(data.size() == SLOT_SIZE, "slot size");
        SlotReader r(data);
        SystemSlot s;
        s.version = r.read<uint32_t>();
        ensure(s.version == SYSTEM_SLOT_VERSION, "unsupported system slot version " + std::to_string(s.version));
        r.read(s.menu);
        r.read(s.profiles);
        s.unk_b0 = r.read<uint32_t>();
        s.unk_flag = r.read<uint8_t>();
        r.read(s.options);
        s.trailingGarbage = r.bytes(r.size() - r.tell());
        return s;
    }

    std::vector<uint8_t> serialize_system_slot(const SystemSlot& s) {
        SlotWriter w;
        w.write(s.version);
        w.write(s.menu);
        w.write(s.profiles);
        w.write(s.unk_b0);
        w.write(s.unk_flag);
        w.write(s.options);
        if (w.tell() + s.trailingGarbage.size() > SLOT_SIZE) {
            throw std::runtime_error("DSR system slot exceeds 0x60000 bytes");
        }
        w.bytes(s.trailingGarbage);
        w.zeros(SLOT_SIZE - w.tell());
        return w.data();
    }
}
