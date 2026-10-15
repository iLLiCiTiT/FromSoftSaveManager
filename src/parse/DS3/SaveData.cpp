#include "SaveData.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fssm::parse::ds3 {
    namespace {
        void ensure(bool cond, const std::string& msg) {
            if (!cond) throw std::runtime_error("DS3 slot layout mismatch: " + msg);
        }

        class SlotReader {
        public:
            explicit SlotReader(const std::vector<uint8_t>& data) : m_data(data), m_pos(0) {}

            void raw(void* dest, size_t n) {
                ensure(m_pos + n <= m_data.size(), "read past end at " + std::to_string(m_pos));
                if (n) std::memcpy(dest, m_data.data() + m_pos, n);
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
            template <class T>
            std::vector<T> array(size_t n) {
                static_assert(std::is_trivially_copyable_v<T>);
                std::vector<T> v(n);
                if (n) raw(v.data(), n * sizeof(T));
                return v;
            }
            size_t tell() const { return m_pos; }
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
            template <class T>
            void array(const std::vector<T>& v) { raw(v.data(), v.size() * sizeof(T)); }
            size_t tell() const { return m_data.size(); }
            std::vector<uint8_t>& data() { return m_data; }

        private:
            std::vector<uint8_t> m_data;
        };

        // ---------------------------------------------------------------- Gaitem table

        GaitemEntry read_gaitem(SlotReader& r) {
            GaitemEntry g;
            g.handle = r.read<uint32_t>();
            g.itemId = r.read<uint32_t>();
            switch (gaitem_kind(g.itemId)) {
                case GaitemKind::Equip: r.read(g.equip); break;
                case GaitemKind::Gem: r.read(g.gem); break;
                case GaitemKind::None: break;
            }
            return g;
        }

        void write_gaitem(SlotWriter& w, const GaitemEntry& g) {
            w.write(g.handle);
            w.write(g.itemId);
            switch (gaitem_kind(g.itemId)) {
                case GaitemKind::Equip: w.write(g.equip); break;
                case GaitemKind::Gem: w.write(g.gem); break;
                case GaitemKind::None: break;
            }
        }

        // ---------------------------------------------------------------- Inventory

        EquipInventoryData read_inventory(SlotReader& r) {
            EquipInventoryData inv;
            auto& l = inv.list;
            l.unk_00 = r.read<uint32_t>();
            l.normal = r.array<InventoryEntry>(INVENTORY_NORMAL_CAPACITY);
            l.unk_20 = r.read<uint32_t>();
            l.key = r.array<InventoryEntry>(INVENTORY_KEY_CAPACITY);
            l.unk_30 = r.read<uint32_t>();
            l.unk = r.array<InventoryEntry>(INVENTORY_KEY_CAPACITY);
            inv.unk_88 = r.read<uint32_t>();
            return inv;
        }

        void write_inventory(SlotWriter& w, const EquipInventoryData& inv) {
            const auto& l = inv.list;
            ensure(l.normal.size() == INVENTORY_NORMAL_CAPACITY && l.key.size() == INVENTORY_KEY_CAPACITY
                   && l.unk.size() == INVENTORY_KEY_CAPACITY, "inventory list capacity");
            w.write(l.unk_00);
            w.array(l.normal);
            w.write(l.unk_20);
            w.array(l.key);
            w.write(l.unk_30);
            w.array(l.unk);
            w.write(inv.unk_88);
        }

        // ---------------------------------------------------------------- PlayerGameData

        EquipGameData read_equip_game_data(SlotReader& r) {
            EquipGameData e;
            r.read(e.equipInventoryIndices);
            r.read(e.chrAsm);
            e.inventory = read_inventory(r);
            r.read(e.magic);
            r.read(e.items);
            r.read(e.gestures);
            const auto count = r.read<uint32_t>();
            ensure(count <= 0x800, "EquipGameData+0x308 count " + std::to_string(count));
            e.unk_308 = r.array<EquipGameDataUnkEntry>(count);
            r.read(e.unk_32c);
            return e;
        }

        void write_equip_game_data(SlotWriter& w, const EquipGameData& e) {
            w.write(e.equipInventoryIndices);
            w.write(e.chrAsm);
            write_inventory(w, e.inventory);
            w.write(e.magic);
            w.write(e.items);
            w.write(e.gestures);
            w.write(static_cast<uint32_t>(e.unk_308.size()));
            w.array(e.unk_308);
            w.write(e.unk_32c);
        }

        PlayerGameData read_player_game_data(SlotReader& r) {
            PlayerGameData p;
            p.gaitems.reserve(GAITEM_COUNT);
            for (size_t i = 0; i < GAITEM_COUNT; ++i) p.gaitems.push_back(read_gaitem(r));
            r.read(p.stats);
            r.read(p.unk_150);
            p.equip = read_equip_game_data(r);
            r.read(p.face);
            p.storage = read_inventory(r);
            r.read(p.gestures);
            const auto count = r.read<uint32_t>();
            ensure(count <= 0x10000, "PlayerGameData vector count " + std::to_string(count));
            p.unk_810 = r.array<uint32_t>(count);
            return p;
        }

        void write_player_game_data(SlotWriter& w, const PlayerGameData& p) {
            ensure(p.gaitems.size() == GAITEM_COUNT, "gaitem count");
            for (const auto& g : p.gaitems) write_gaitem(w, g);
            w.write(p.stats);
            w.write(p.unk_150);
            write_equip_game_data(w, p.equip);
            w.write(p.face);
            write_inventory(w, p.storage);
            w.write(p.gestures);
            w.write(static_cast<uint32_t>(p.unk_810.size()));
            w.array(p.unk_810);
        }

        uint32_t adler32(const uint8_t* data, size_t n) {
            uint32_t a = 1, b = 0;
            while (n) {
                const size_t chunk = std::min<size_t>(n, 5552);
                for (size_t i = 0; i < chunk; ++i) {
                    a += data[i];
                    b += a;
                }
                a %= 65521;
                b %= 65521;
                data += chunk;
                n -= chunk;
            }
            return (b << 16) | a;
        }
    }

    GaitemKind gaitem_kind(uint32_t itemId) {
        switch (itemId >> 28) {
            case 0x0:       // weapon (SprjWepGaitemIns)
            case 0x1:       // protector (SprjProGaitemIns)
                return GaitemKind::Equip;
            case 0x8:       // gem (SprjGemGaitemIns)
                return GaitemKind::Gem;
            default:        // empty slot (0xffffffff)
                return GaitemKind::None;
        }
    }

    bool is_empty_slot(const std::vector<uint8_t>& data) {
        return std::all_of(data.begin(), data.end(), [](uint8_t b) { return b == 0; });
    }

    uint32_t compute_character_checksum(const std::vector<uint8_t>& data, const CharacterSlotHeader& header) {
        const size_t begin = header.checksumOffset;
        const size_t end = size_t(header.trailerOffset) + header.trailerSize + sizeof(CharacterSlotEnd);
        ensure(begin <= end && end <= data.size(), "checksum range");
        return adler32(data.data() + begin, end - begin) * 2;
    }

    // ---------------------------------------------------------------- Character slot

    CharacterSlot parse_character_slot(const std::vector<uint8_t>& data) {
        SlotReader r(data);
        CharacterSlot s;
        r.read(s.header);
        const auto& h = s.header;
        ensure(h.version == CHARACTER_SLOT_VERSION, "unsupported version " + std::to_string(h.version));
        r.read(s.random);

        // Section 1: GameDataMan
        ensure(r.tell() == h.gameDataOffset && r.tell() == h.playerGameDataOffset, "game data offset");
        s.player = read_player_game_data(r);
        ensure(r.tell() == h.playerGameDataOffset + h.playerGameDataSize, "PlayerGameData size");
        ensure(r.tell() == h.blobs44Offset, "blobs offset");
        s.hasBlob44a = r.read<uint8_t>();
        ensure(s.hasBlob44a == 1, "blob44a flag");
        r.read(s.blob44a);
        s.unk_48 = r.read<uint32_t>();
        s.hasBlob44b = r.read<uint8_t>();
        ensure(s.hasBlob44b == 1, "blob44b flag");
        r.read(s.blob44b);
        ensure(r.tell() == h.blobs44Offset + h.blobs44Size && r.tell() == h.unk2u32Offset, "blobs size");
        s.unk_78 = r.read<uint32_t>();
        s.unk_7c = r.read<uint32_t>();
        ensure(r.tell() == h.unk2u32Offset + h.unk2u32Size, "u32 pair size");
        r.read(s.trophy);
        r.read(s.scalars);
        ensure(r.tell() == h.gameDataOffset + h.gameDataSize, "GameDataMan size");

        // Section 2: event flags
        ensure(r.tell() == h.eventFlagsOffset && h.eventFlagsSize >= 1, "event flags offset");
        s.eventFlags = r.bytes(h.eventFlagsSize - 1);
        s.eventFlagsUnk = r.read<uint8_t>();

        // Section 3: map state
        ensure(r.tell() == h.mapStateOffset, "map state offset");
        for (auto& blob : s.mapState) {
            const auto size = r.read<uint32_t>();
            ensure(size <= h.mapStateSize, "map state blob size");
            blob = r.bytes(size);
        }
        ensure(r.tell() == h.mapStateOffset + h.mapStateSize, "map state size");

        // Section 4
        ensure(r.tell() == h.saveLoadOffset && h.saveLoadSize == sizeof(SaveLoadManBlock), "save load section");
        r.read(s.saveLoad);

        // Section 5
        ensure(r.tell() == h.netChunksOffset && h.netChunksSize == 4 + 0x20000, "net chunks section");
        s.netChunksVersion = r.read<uint32_t>();
        s.netChunks = r.bytes(0x20000);

        // Section 6
        ensure(r.tell() == h.trailerOffset && h.trailerSize == sizeof(CharacterSlotTrailer), "trailer section");
        r.read(s.trailer);
        r.read(s.end);

        s.trailingGarbage = r.bytes(r.size() - r.tell());
        return s;
    }

    std::vector<uint8_t> serialize_character_slot(const CharacterSlot& s) {
        SlotWriter w;
        w.write(s.header);
        w.write(s.random);
        write_player_game_data(w, s.player);
        w.write(s.hasBlob44a);
        w.write(s.blob44a);
        w.write(s.unk_48);
        w.write(s.hasBlob44b);
        w.write(s.blob44b);
        w.write(s.unk_78);
        w.write(s.unk_7c);
        w.write(s.trophy);
        w.write(s.scalars);
        w.bytes(s.eventFlags);
        w.write(s.eventFlagsUnk);
        for (const auto& blob : s.mapState) {
            w.write(static_cast<uint32_t>(blob.size()));
            w.bytes(blob);
        }
        w.write(s.saveLoad);
        w.write(s.netChunksVersion);
        w.bytes(s.netChunks);
        w.write(s.trailer);
        w.write(s.end);
        w.bytes(s.trailingGarbage);
        return std::move(w.data());
    }

    // ---------------------------------------------------------------- System slot

    SystemSlot parse_system_slot(const std::vector<uint8_t>& data) {
        SlotReader r(data);
        SystemSlot s;
        s.version = r.read<uint32_t>();
        s.steamId = r.read<uint64_t>();
        r.read(s.menu);
        s.menuChunksUnk0 = r.read<uint16_t>();
        s.menuChunksUnk1 = r.read<uint16_t>();
        const auto chunksSize = r.read<uint32_t>();
        ensure(chunksSize <= 0x10000, "menu chunks size " + std::to_string(chunksSize));
        s.menuChunks = r.bytes(chunksSize);
        r.read(s.profiles);
        s.unk_cc = r.read<uint32_t>();
        s.unk_flag = r.read<uint8_t>();
        r.read(s.options);
        r.read(s.unk_384);
        r.read(s.unk_710);
        s.trailingGarbage = r.bytes(r.size() - r.tell());
        return s;
    }

    std::vector<uint8_t> serialize_system_slot(const SystemSlot& s) {
        SlotWriter w;
        w.write(s.version);
        w.write(s.steamId);
        w.write(s.menu);
        w.write(s.menuChunksUnk0);
        w.write(s.menuChunksUnk1);
        w.write(static_cast<uint32_t>(s.menuChunks.size()));
        w.bytes(s.menuChunks);
        w.write(s.profiles);
        w.write(s.unk_cc);
        w.write(s.unk_flag);
        w.write(s.options);
        w.write(s.unk_384);
        w.write(s.unk_710);
        w.bytes(s.trailingGarbage);
        return std::move(w.data());
    }
}
