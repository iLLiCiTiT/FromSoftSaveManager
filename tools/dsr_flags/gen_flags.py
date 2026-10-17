"""Generate the Dark Souls Remastered bonfire and boss tables from the installed game data.

Usage:
    python gen_flags.py "<...>/DARK SOULS REMASTERED" <repo root>

Writes:
    src/parse/DSR/WorldProgress.h      table sizes
    src/parse/DSR/WorldProgress.cpp    bonfires and bosses

The game has no bonfire param. Bonfires are the RegisterBonfire (2009:03) calls of the map
event scripts (/event/mAA_BB_00_00.emevd.dcx); the bonfire's entity id is the key of its
FrpgNetBonfireDbItem in the save (src/parse/DSR/BonfireDb.h).

Names: the Lordvessel warp list in DarkSoulsRemastered.exe (21 rows of event flag 200-220,
bonfire entity, PlaceName id 2000-2020; FUN_1406ed740) names the bonfires it lists, with
the text from /msg/ENGLISH/item.msgbnd.dcx (Place_name_.fmg). Bonfires the warp list doesn't
have use community names (BONFIRES below, after DSR-Gadget's Resources/Bonfires.txt, whose
ids are the bonfires' warp points, entity + 1000). Area names are PlaceName texts.

Bosses: each map script event that calls HandleBossDefeat (2003:12) on a boss; the event's
own id is the defeat flag (an event flag is set when its event completes; the global flags
2-16 those events also set agree on every checked save). The name is the boss health bar
(DisplayBossHealthBar 2003:11, NPC_name_.fmg) of the killed entity or of an entity the
event waits for (Bed of Chaos). Events without a named health bar are skipped (the unused
fight 1600810 in New Londo, health bar name 2390 has no text).
"""
import glob
import os
import re
import struct
import sys
import zlib

EMEVD_REGISTER_BONFIRE = (2009, 3)
EMEVD_DISPLAY_BOSS_HEALTH_BAR = (2003, 11)
EMEVD_HANDLE_BOSS_DEFEAT = (2003, 12)
EMEVD_IF_CHARACTER_DEAD = (4, 0)

# Areas (PlaceName ids) in the order they are usually reached.
AREAS = [
    1810,   # Northern Undead Asylum
    1020,   # Firelink Shrine
    1010,   # Undead Burg
    1011,   # Undead Parish
    1000,   # Depths
    1400,   # Blighttown
    1200,   # Darkroot Garden
    1500,   # Sen's Fortress
    1510,   # Anor Londo
    1100,   # Painted World of Ariamis
    1600,   # New Londo Ruins
    1700,   # The Duke's Archives
    1300,   # The Catacombs
    1310,   # Tomb of the Giants
    1320,   # The Great Hollow
    1410,   # Demon Ruins
    1411,   # Lost Izalith
    1210,   # Sanctuary Garden
    1213,   # Oolacile Township
    1214,   # Chasm of the Abyss
    1800,   # Kiln of the First Flame
]

# Bonfire entity -> (area, community name or None for the warp list's name), in the
# order shown within each area.
BONFIRES = {
    1811960: (1810, "Undead Asylum (Bonfire #1)"),
    1811961: (1810, "Undead Asylum (Bonfire #2)"),
    1021960: (1020, None),                              # Firelink Shrine
    1011962: (1010, "Undead Burg"),
    1011961: (1010, None),                              # Sunlight Altar
    1011964: (1011, None),                              # Undead Parish
    1001960: (1000, None),                              # Depths
    1401961: (1400, "Blighttown (Swamp)"),
    1401962: (1400, "Blighttown (Bridge)"),
    1401960: (1400, None),                              # Daughter of Chaos
    1201961: (1200, "Darkroot Garden"),
    1601961: (1200, "Darkroot Basin"),
    1501961: (1500, "Sen's Fortress"),
    1511960: (1510, None),                              # Anor Londo
    1511961: (1510, "Anor Londo (Interior)"),
    1511950: (1510, None),                              # Chamber of the Princess
    1511962: (1510, None),                              # Darkmoon Tomb
    1101960: (1100, None),                              # Painted World of Ariamis
    1601950: (1600, None),                              # The Abyss
    1701960: (1700, None),                              # The Duke's Archives
    1701961: (1700, "The Duke's Archives (Cell)"),
    1701962: (1700, "The Duke's Archives (Elevator)"),
    1701950: (1700, None),                              # Crystal Cave
    1301962: (1300, None),                              # Catacombs
    1301960: (1300, "Catacombs (Bonfire #1)"),
    1301961: (1300, "Catacombs (Bonfire #2)"),
    1311960: (1310, "Tomb of the Giants (Bonfire #1)"),
    1311961: (1310, None),                              # Tomb of the Giants
    1311950: (1310, None),                              # Altar of the Gravelord
    1321962: (1320, "The Great Hollow"),
    1321961: (1320, "Ash Lake"),
    1321960: (1320, None),                              # Stone Dragon
    1411961: (1410, "Demon Ruins (Bonfire #1)"),
    1411962: (1410, "Demon Ruins (Bonfire #2)"),
    1411963: (1410, "Demon Ruins (Bonfire #3)"),
    1411964: (1411, "Lost Izalith (Bonfire #1)"),
    1411960: (1411, "Lost Izalith (Bonfire #2)"),
    1411950: (1411, "Lost Izalith (Bed of Chaos)"),
    1211963: (1210, None),                              # Sanctuary Garden
    1211961: (1210, None),                              # Oolacile Sanctuary
    1211962: (1213, None),                              # Oolacile Township
    1211964: (1213, None),                              # Oolacile Township Dungeon
    1211950: (1214, None),                              # Chasm of the Abyss
    1801960: (1800, "Firelink Altar"),
}

# Boss entity -> area, in the order bosses are usually met.
BOSSES = {
    1810800: 1810,  # Asylum Demon
    1010700: 1010,  # Taurus Demon
    1010800: 1011,  # Bell Gargoyle
    1010750: 1010,  # Capra Demon
    1000800: 1000,  # Gaping Dragon
    1200801: 1200,  # Moonlight Butterfly
    1400800: 1400,  # Chaos Witch Quelaag
    1810810: 1810,  # Stray Demon
    1500800: 1500,  # Iron Golem
    1510801: 1510,  # Ornstein and Smough
    1510650: 1510,  # Dark Sun Gwyndolin
    1100160: 1100,  # Crossbreed Priscilla
    1200800: 1200,  # Great Grey Wolf Sif
    1300800: 1300,  # Pinwheel
    1310800: 1310,  # Gravelord Nito
    1410600: 1410,  # Ceaseless Discharge
    1410400: 1410,  # Demon Firesage
    1410700: 1410,  # Centipede Demon
    1410800: 1411,  # Bed of Chaos
    1600800: 1600,  # Four Kings
    1700800: 1700,  # Seath the Scaleless
    1210800: 1210,  # Sanctuary Guardian
    1210820: 1213,  # Knight Artorias
    1210401: 1213,  # Black Dragon Kalameet
    1210840: 1214,  # Manus, Father of the Abyss
    1800800: 1800,  # Gwyn, Lord of Cinder
}

# Fights whose health bars show one name of a group.
BOSS_NAMES = {
    1010800: "Bell Gargoyles",
    1510801: "Ornstein and Smough",
}

WARP_FLAGS = range(200, 300)
WARP_TEXTS = range(2000, 2100)


def undcx(b):
    if b[:4] != b"DCX\0":
        return b
    fmt = b[0x28:0x2C]
    if fmt != b"DFLT":
        raise RuntimeError(f"unsupported DCX format {fmt}")
    compressed = struct.unpack_from(">I", b, 0x20)[0]
    i = b.find(b"DCA\0")
    header = struct.unpack_from(">I", b, i + 4)[0]
    return zlib.decompress(b[i + header:i + header + compressed])


def read(game_dir, path):
    with open(os.path.join(game_dir, path), "rb") as f:
        return undcx(f.read())


def parse_bnd3(b):
    assert b[:4] == b"BND3", "expected BND3"
    count = struct.unpack_from("<I", b, 0x10)[0]
    entry_size = 0x18 if b[0x0C] & 0x04 else 0x14
    out = {}
    for i in range(count):
        size, offset, _id, name_offset = struct.unpack_from("<IIiI", b, 0x20 + i * entry_size + 4)
        name = b[name_offset:b.index(b"\0", name_offset)].decode("cp932")
        out[name.replace("\\", "/").split("/")[-1]] = undcx(b[offset:offset + size])
    return out


def parse_fmg1(d):
    """FMG version 1 (Dark Souls): 32-bit offsets."""
    assert d[2] == 1, "expected FMG version 1"
    group_count = struct.unpack_from("<I", d, 0x0C)[0]
    strings_offset = struct.unpack_from("<I", d, 0x14)[0]
    out = {}
    for g in range(group_count):
        index, first, last = struct.unpack_from("<iii", d, 0x1C + g * 12)
        for k in range(last - first + 1):
            so = struct.unpack_from("<I", d, strings_offset + (index + k) * 4)[0]
            if so == 0:
                continue
            end = so
            while d[end:end + 2] != b"\0\0":
                end += 2
            out[first + k] = d[so:end].decode("utf-16le")
    return out


def parse_emevd(d):
    """[(event id, [(bank, index, args)])] of a Dark Souls EMEVD (32-bit, version 0xCC)."""
    assert d[:4] == b"EVD\0", "expected EMEVD"
    header = struct.unpack_from("<16I", d, 0x10)
    event_count, events_offset, instr_offset, args_offset = header[0], header[1], header[3], header[13]
    events = []
    for e in range(event_count):
        event_id, count, offset = struct.unpack_from("<III", d, events_offset + e * 0x1C)
        instructions = []
        for i in range(count):
            bank, index, args_length, args_off = struct.unpack_from("<IIIi", d, instr_offset + offset + i * 0x18)
            args = d[args_offset + args_off:args_offset + args_off + args_length] if args_length else b""
            instructions.append((bank, index, struct.unpack_from(f"<{len(args) // 4}i", args)))
        events.append((event_id, instructions))
    return events


def warp_list(exe):
    """[(event flag, bonfire entity, PlaceName id)] of the Lordvessel warp menu."""
    for m in re.finditer(re.escape(struct.pack("<i", WARP_TEXTS[0])), exe):
        start = m.start() - 8
        rows = []
        while start + 12 <= len(exe):
            flag, entity, text = struct.unpack_from("<iii", exe, start)
            if flag not in WARP_FLAGS or text not in WARP_TEXTS:
                break
            rows.append((flag, entity, text))
            start += 12
        if len(rows) > 10:
            return rows
    raise RuntimeError("Lordvessel warp list not found")


def map_events(game_dir):
    for path in sorted(glob.glob(os.path.join(game_dir, "event", "m1*_00_00.emevd.dcx"))):
        with open(path, "rb") as f:
            yield parse_emevd(undcx(f.read()))


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    game_dir, repo = sys.argv[1:]
    item_msg = parse_bnd3(read(game_dir, "msg/ENGLISH/item.msgbnd.dcx"))
    place_names = parse_fmg1(item_msg["Place_name_.fmg"])
    npc_names = parse_fmg1(item_msg["NPC_name_.fmg"])
    with open(os.path.join(game_dir, "DarkSoulsRemastered.exe"), "rb") as f:
        warp = {entity: text for _flag, entity, text in warp_list(f.read())}

    registered = set()
    bosses = {}
    for events in map_events(game_dir):
        bars = {}
        for _event_id, instructions in events:
            for bank, index, args in instructions:
                if (bank, index) == EMEVD_REGISTER_BONFIRE:
                    registered.add(args[1])
                elif (bank, index) == EMEVD_DISPLAY_BOSS_HEALTH_BAR:
                    bars[args[1]] = args[2] >> 16      # s16 slot, s16 name id
        for event_id, instructions in events:
            killed = [args[0] for bank, index, args in instructions if (bank, index) == EMEVD_HANDLE_BOSS_DEFEAT]
            if not killed:
                continue
            waited = [args[1] for bank, index, args in instructions if (bank, index) == EMEVD_IF_CHARACTER_DEAD]
            # One event per fight, keyed by its first boss (Ornstein and Smough are two).
            name_ids = [bars[e] for e in killed + waited if e in bars and bars[e] in npc_names]
            if name_ids:
                bosses[killed[0]] = (event_id, npc_names[name_ids[0]])

    if registered != set(BONFIRES):
        raise RuntimeError(f"bonfires changed: missing {sorted(registered - set(BONFIRES))}, "
                           f"gone {sorted(set(BONFIRES) - registered)}")
    if not set(warp) <= registered:
        raise RuntimeError(f"warp list has unknown bonfires {sorted(set(warp) - registered)}")
    if set(bosses) != set(BOSSES):
        raise RuntimeError(f"bosses changed: missing {sorted(set(bosses) - set(BOSSES))}, "
                           f"gone {sorted(set(BOSSES) - set(bosses))}")

    def area_name(place):
        return " ".join(place_names[place].split())

    bonfire_table = []
    for area_index, place in enumerate(AREAS):
        for entity, (area, name) in BONFIRES.items():
            if area != place:
                continue
            if (entity in warp) != (name is None):
                raise RuntimeError(f"bonfire {entity}: name and warp list disagree")
            bonfire_table.append((entity, area_index, name or " ".join(place_names[warp[entity]].split())))
    boss_table = [(bosses[entity][0], AREAS.index(area), BOSS_NAMES.get(entity, bosses[entity][1]))
                  for entity, area in BOSSES.items()]

    out_dir = os.path.join(repo, "src", "parse", "DSR")
    write_crlf(os.path.join(out_dir, "WorldProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/dsr_flags/gen_flags.py, do not edit.
namespace fssm::parse::dsr {{
    struct Bonfire {{
        uint32_t entityId;              // key of the bonfire's FrpgNetBonfireDbItem
        uint8_t area;                   // index into AREAS
        std::string_view name;          // Lordvessel warp menu text, else community name
    }};

    struct Boss {{
        uint32_t flagId;                // set when defeated
        uint8_t area;                   // index into AREAS
        std::string_view name;
    }};

    // Areas in the order they are usually reached.
    extern const std::array<std::string_view, {len(AREAS)}> AREAS;
    // Bonfires by area.
    extern const std::array<Bonfire, {len(bonfire_table)}> BONFIRES;
    // Bosses in the order they are usually met.
    extern const std::array<Boss, {len(boss_table)}> BOSSES;
}}
""")

    lines = [
        '#include "WorldProgress.h"',
        "",
        "// Generated by tools/dsr_flags/gen_flags.py, do not edit.",
        "namespace fssm::parse::dsr {",
        f"    const std::array<std::string_view, {len(AREAS)}> AREAS = {{",
    ]
    lines += [f"        {cpp_string(area_name(a))}," for a in AREAS]
    lines += ["    };", "", f"    const std::array<Bonfire, {len(bonfire_table)}> BONFIRES = {{"]
    for entity, area, name in bonfire_table:
        lines.append(f"        Bonfire{{{entity}, {area}, {cpp_string(name)}}},")
    lines += ["    };", "", f"    const std::array<Boss, {len(boss_table)}> BOSSES = {{"]
    for flag, area, name in boss_table:
        lines.append(f"        Boss{{{flag}, {area}, {cpp_string(name)}}},")
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(bonfire_table)} bonfires ({len(warp)} named by the warp list), {len(boss_table)} bosses")


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_crlf(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


if __name__ == "__main__":
    main()
