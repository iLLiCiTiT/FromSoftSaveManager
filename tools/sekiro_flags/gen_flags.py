"""Generate the Sekiro idol and boss tables from the installed game data.

Usage:
    python gen_flags.py "<...>/Sekiro" <sekiro exe> <repo root>

The exe only provides the archive keys (see tools/sekiro_items/sekirodata.py).

Writes:
    src/parse/Sekiro/WorldProgress.h      table sizes
    src/parse/Sekiro/WorldProgress.cpp    idols (BonfireWarpParam) and bosses

Idols come from BonfireWarpParam in /param/gameparam/gameparam.parambnd.dcx: +0x0 event
flag set when the idol is found, +0x8 name (NTC_メニューテキスト.fmg in
/msg/engus/menu.msgbnd.dcx), +0x10 travel menu sort id, +0x14 travel menu area (0 = rows
the menu doesn't list). Area names are the PlaceName (地名.fmg) of each area.

Bosses: every GameAreaParam row except 0 and 1 is a boss whose defeat flag is the row id
+ 10000000 (row 1000800 -> flag 11000800). The row id is also the boss's entity id; its
name is the text of the event scripts' DisplayBossHealthBar (2003:11) for that entity
(/event/mAA_BB_00_00.emevd.dcx, NPC名.fmg). A boss belongs to the travel menu area of most
idols in its map. Fights that show the same name in the same area are one entry, defeated
when any of their flags is set.

Requires pycryptodome.
"""
import collections
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "sekiro_items"))
import sekirodata  # noqa: E402

ARCHIVES = ("Data1", "Data2", "Data3", "Data4", "Data5")
HIDDEN_AREA = 0

# Travel menu area (BonfireWarpParam +0x14) -> PlaceName id.
AREA_PLACE_NAMES = {
    1: 1100,    # Ashina Outskirts
    2: 1110,    # Ashina Castle
    4: 1300,    # Abandoned Dungeon
    5: 2000,    # Senpou Temple, Mt. Kongo
    6: 1700,    # Sunken Valley
    7: 1510,    # Ashina Depths
    8: 2500,    # Fountainhead Palace
    9: 1000,    # Hirata Estate
}

# Bosses whose health bar repeats another fight's name: (name, travel menu area).
BOSS_OVERRIDES = {
    11700850: ("Headless Ape", 7),      # bar reads "Guardian Ape"; fought in Guardian Ape's Burrow
}

MAX_BOSS_FLAGS = 2
EMEVD_DISPLAY_BOSS_HEALTH_BAR = (2003, 11)


def open_archives(game_dir, exe_path):
    archives = []
    for name in ARCHIVES:
        try:
            archives.append(sekirodata.Archive(game_dir, exe_path, name))
        except (OSError, RuntimeError):
            pass    # not every archive is readable with the exe's keys; Data1 holds what we need
    if not archives:
        raise RuntimeError("no archive could be opened")
    return archives


def read_file(archives, path):
    h = sekirodata.path_hash(path)
    for archive in archives:
        if h in archive.entries:
            return archive.read_hash(h)
    return None


def read_bnd(archives, path):
    data = read_file(archives, path)
    if data is None:
        raise RuntimeError(f"{path} not found")
    return {os.path.basename(k.replace("\\", "/")): archives[0].undcx(v)
            for k, v in sekirodata.parse_bnd4(data).items()}


def boss_health_bars(emevd):
    """(entity id, name id) of every DisplayBossHealthBar in an EMEVD (64-bit, Sekiro)."""
    assert emevd[:4] == b"EVD\0"
    (event_count, events_offset, _instr_count, instr_offset, _u0, _u1, _layer_count, _layer_offset,
     _param_count, _param_offset, _linked_count, _linked_offset, _args_length, args_offset,
     _strings_length, _strings_offset) = struct.unpack_from("<16q", emevd, 0x10)
    out = []
    for e in range(event_count):
        _event_id, count, offset = struct.unpack_from("<qqq", emevd, events_offset + e * 0x30)
        for i in range(count):
            bank, index, args_length, args_off = struct.unpack_from("<iiqi", emevd, instr_offset + offset + i * 0x20)
            if (bank, index) != EMEVD_DISPLAY_BOSS_HEALTH_BAR or args_length < 16:
                continue
            _state, entity, _slot, name_id = struct.unpack_from("<iiii", emevd, args_offset + args_off)
            out.append((entity, name_id))
    return out


def idols(params, menu_text, place_names):
    rows = sekirodata.parse_param(params["BonfireWarpParam.param"])
    out = []
    for row in rows.values():
        flag, _entity, text_id, _unk, sort_id = struct.unpack_from("<iiiii", row, 0)
        area = row[0x14]
        if flag <= 0 or area == HIDDEN_AREA:
            continue
        if area not in AREA_PLACE_NAMES:
            raise RuntimeError(f"unknown travel menu area {area}")
        out.append((sort_id, flag, area, " ".join(menu_text[text_id].split())))
    out.sort()
    areas = sorted({area for _sort, _flag, area, _name in out})
    area_names = [" ".join(place_names[AREA_PLACE_NAMES[a]].split()) for a in areas]
    return [(flag, areas.index(area), name) for _sort, flag, area, name in out], areas, area_names


def map_block(flag):
    return flag // 10000 % 1000     # AA B of the flag id


def bosses(archives, params, npc_names, idol_table, areas):
    # Travel menu area of each map block: the area most of its idols are listed in.
    votes = collections.defaultdict(collections.Counter)
    for flag, area, _name in idol_table:
        votes[map_block(flag)][area] += 1
    block_area = {block: counter.most_common(1)[0][0] for block, counter in votes.items()}

    entities = sorted(k for k in sekirodata.parse_param(params["GameAreaParam.param"]) if k > 1)
    names = {}
    for block in sorted({e // 10000 for e in entities}):
        path = f"/event/m{block // 10:02d}_{block % 10:02d}_00_00.emevd.dcx"
        data = read_file(archives, path)
        if data is None:
            raise RuntimeError(f"{path} not found")
        for entity, name_id in boss_health_bars(data):
            if name_id in npc_names:
                names.setdefault(entity, npc_names[name_id])

    merged = {}
    for entity in entities:
        flag = 10000000 + entity
        if flag in BOSS_OVERRIDES:
            name, menu_area = BOSS_OVERRIDES[flag]
            area = areas.index(menu_area)
        elif entity in names:
            name, area = names[entity], block_area[map_block(flag)]
        else:
            print(f"boss flag {flag}: no health bar, skipped")
            continue
        merged.setdefault((area, name), []).append(flag)
    out = []
    for (area, name), flags in merged.items():
        if len(flags) > MAX_BOSS_FLAGS:
            raise RuntimeError(f"{name} has {len(flags)} flags")
        out.append((area, flags, name))
    out.sort(key=lambda b: (b[0], b[1][0]))
    return out


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_crlf(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)
    game_dir, exe_path, repo = sys.argv[1:]
    archives = open_archives(game_dir, exe_path)
    params = read_bnd(archives, "/param/gameparam/gameparam.parambnd.dcx")
    menu = read_bnd(archives, "/msg/engus/menu.msgbnd.dcx")
    item = read_bnd(archives, "/msg/engus/item.msgbnd.dcx")
    menu_text = sekirodata.parse_fmg2(menu["NTC_メニューテキスト.fmg"])
    place_names = sekirodata.parse_fmg2(item["地名.fmg"])
    npc_names = sekirodata.parse_fmg2(item["NPC名.fmg"])

    idol_table, areas, area_names = idols(params, menu_text, place_names)
    boss_table = bosses(archives, params, npc_names, idol_table, areas)

    out_dir = os.path.join(repo, "src", "parse", "Sekiro")
    write_crlf(os.path.join(out_dir, "WorldProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/sekiro_flags/gen_flags.py from the game's BonfireWarpParam,
// GameAreaParam, event scripts and menu text; do not edit.
namespace fssm::parse::sekiro {{
    struct Idol {{
        uint32_t flagId;                // set when the idol is found
        uint8_t area;                   // index into AREAS
        std::string_view name;
    }};

    struct Boss {{
        // Event flags set when defeated (any of them); 0 when unused.
        std::array<uint32_t, {MAX_BOSS_FLAGS}> flagIds;
        uint8_t area;                   // index into AREAS
        std::string_view name;
    }};

    // Areas of the idol travel menu, in menu order.
    extern const std::array<std::string_view, {len(area_names)}> AREAS;
    // Sculptor's Idols in travel menu order.
    extern const std::array<Idol, {len(idol_table)}> IDOLS;
    // Bosses by area.
    extern const std::array<Boss, {len(boss_table)}> BOSSES;
}}
""")

    lines = [
        '#include "WorldProgress.h"',
        "",
        "// Generated by tools/sekiro_flags/gen_flags.py, do not edit.",
        "namespace fssm::parse::sekiro {",
        f"    const std::array<std::string_view, {len(area_names)}> AREAS = {{",
    ]
    lines += [f"        {cpp_string(a)}," for a in area_names]
    lines += ["    };", "", f"    const std::array<Idol, {len(idol_table)}> IDOLS = {{"]
    for flag, area, name in idol_table:
        lines.append(f"        Idol{{{flag}, {area}, {cpp_string(name)}}},")
    lines += ["    };", "", f"    const std::array<Boss, {len(boss_table)}> BOSSES = {{"]
    for area, flags, name in boss_table:
        ids = ", ".join(str(f) for f in flags + [0] * (MAX_BOSS_FLAGS - len(flags)))
        lines.append(f"        Boss{{{{{ids}}}, {area}, {cpp_string(name)}}},")
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(idol_table)} idols, {len(boss_table)} bosses")


if __name__ == "__main__":
    main()
