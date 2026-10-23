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

Bosses: every GameAreaParam row except 0 and 1 is a boss; the row id is also the boss's
entity id. Its name is the text of the event scripts' DisplayBossHealthBar (2003:11) for
that entity (/event/mAA_BB_00_00.emevd.dcx, NPC名.fmg). Its defeat flag is the global
flag 9300-9399 its defeat event sets: the event (with the arguments of each Initialize
[Common] Event filled in) that sets one of those flags and calls HandleBossDefeat (2003:12)
or DisplayBossHealthBar for the entity, or, where no event does, mentions the entity and
no other boss. The row id + 10000000 is not reliable: the scripts never set 12500800
(Divine Dragon, the event sets 12500880) or 11110801 (Genichiro, Way of Tomoe, 11110800).
A boss belongs to the travel menu area of most idols in its map. Fights that show the
same name in the same area are one entry, defeated when any of their flags is set. Bosses
are listed by boss flag (the game's order).

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
    3: 1120,    # Ashina Reservoir: no idols (the menu lists them under Ashina Castle), bosses only
    4: 1300,    # Abandoned Dungeon
    5: 2000,    # Senpou Temple, Mt. Kongo
    6: 1700,    # Sunken Valley
    7: 1510,    # Ashina Depths
    8: 2500,    # Fountainhead Palace
    9: 1000,    # Hirata Estate
}

# Bosses whose health bar repeats another fight's name: (name, travel menu area).
BOSS_OVERRIDES = {
    1700850: ("Headless Ape", 7),       # bar reads "Guardian Ape"; fought in Guardian Ape's Burrow
    2500850: ("True Corrupted Monk", 8),    # bar reads "Corrupted Monk", like the illusion in Ashina Depths
}
# Bosses listed in an area other than their map's: travel menu area.
BOSS_AREAS = {
    1120860: 3,                         # Isshin, the Sword Saint, at Ashina Reservoir (the final boss)
}

MAX_BOSS_FLAGS = 2
BOSS_FLAG_MIN, BOSS_FLAG_MAX = 9300, 9400
EMEVD_DISPLAY_BOSS_HEALTH_BAR = (2003, 11)
EMEVD_HANDLE_BOSS_DEFEAT = (2003, 12)
EMEVD_SET_EVENT_FLAGS = ((2003, 2), (2003, 62))     # Set [Network-Connected] Event Flag


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
    areas += sorted(set(BOSS_AREAS.values()) - set(areas))     # boss-only areas last
    area_names = [" ".join(place_names[AREA_PLACE_NAMES[a]].split()) for a in areas]
    return [(flag, areas.index(area), name) for _sort, flag, area, name in out], areas, area_names


def map_block(flag):
    return flag // 10000 % 1000     # AA B of the flag id


def parse_emevd(data):
    """{event id: [(bank, id, args, [(target, source, size)])]} of an EMEVD (64-bit)."""
    assert data[:4] == b"EVD\0"
    (event_count, events_offset, _ic, instrs_offset, _uc, _uo, _lc, _lo,
     _pc, params_offset, _fc, _fo, _al, args_offset) = struct.unpack_from("<14q", data, 0x10)
    events = {}
    for i in range(event_count):
        event_id, count, offset, param_count, param_offset = struct.unpack_from(
            "<qqqqq", data, events_offset + i * 0x30)
        instrs = []
        for k in range(count):
            bank, index, size, arg_offset, _layer = struct.unpack_from(
                "<IIqqq", data, instrs_offset + offset + k * 0x20)
            start = args_offset + arg_offset
            instrs.append((bank, index, data[start:start + size], []))
        for k in range(param_count):
            instr, target, source, size = struct.unpack_from("<qqqi", data, params_offset + param_offset + k * 0x20)
            instrs[instr][3].append((target, source, size))
        events[event_id] = instrs
    return events


def substitute(args, params, values):
    out = bytearray(args)
    for target, source, size in params:
        if source + size <= len(values):
            out[target:target + size] = values[source:source + size]
    return bytes(out)


def event_instances(events, common_func):
    """Instructions of every event of a script with the arguments of each Initialize Event
    (2000:00 slot, event id, args) / Initialize Common Event (2000:06 event id, args)
    filled in, plus the events nothing initializes."""
    initialized = set()
    for instrs in events.values():
        for bank, index, args, _params in instrs:
            if (bank, index) == (2000, 0):
                event_id, values, source = struct.unpack_from("<I", args, 4)[0], args[8:], events
            elif (bank, index) == (2000, 6):
                event_id, values, source = struct.unpack_from("<I", args, 0)[0], args[4:], common_func
            else:
                continue
            if event_id in source:
                if source is events:
                    initialized.add(event_id)
                yield [(b, i, substitute(a, p, values)) for b, i, a, p in source[event_id]]
    for event_id, instrs in events.items():
        if event_id not in initialized:
            yield [(b, i, a) for b, i, a, _p in instrs]


def boss_defeat_events(instances, entities):
    """(boss flags, entities named by HandleBossDefeat / DisplayBossHealthBar, boss entities
    in any argument) of every event instance that sets a global boss flag on."""
    out = []
    for instrs in instances:
        flags, named, mentioned = set(), set(), set()
        for bank, index, args in instrs:
            if (bank, index) in EMEVD_SET_EVENT_FLAGS and len(args) >= 5 and args[4] == 1:
                flag = struct.unpack_from("<i", args, 0)[0]
                if BOSS_FLAG_MIN <= flag < BOSS_FLAG_MAX:
                    flags.add(flag)
            elif (bank, index) == EMEVD_HANDLE_BOSS_DEFEAT and len(args) >= 4:
                named.add(struct.unpack_from("<i", args, 0)[0])
            elif (bank, index) == EMEVD_DISPLAY_BOSS_HEALTH_BAR and len(args) >= 8:
                named.add(struct.unpack_from("<i", args, 4)[0])
            mentioned.update(v for v in struct.unpack_from(f"<{len(args) // 4}i", args) if v in entities)
        if flags:
            out.append((flags, named, mentioned))
    return out


def boss_flags(entity, defeat_events, entities):
    """Boss flags set by the defeat events of an entity (see the module doc)."""
    others = entities - {entity}
    named = [flags for flags, names, _mentioned in defeat_events if entity in names]
    if not named:
        named = [flags for flags, names, mentioned in defeat_events
                 if entity in mentioned and not (names | mentioned) & others]
    return set().union(*named)


def bosses(archives, params, npc_names, idol_table, areas):
    # Travel menu area of each map block: the area most of its idols are listed in.
    votes = collections.defaultdict(collections.Counter)
    for flag, area, _name in idol_table:
        votes[map_block(flag)][area] += 1
    block_area = {block: counter.most_common(1)[0][0] for block, counter in votes.items()}

    entities = sorted(k for k in sekirodata.parse_param(params["GameAreaParam.param"]) if k > 1)
    common_func = parse_emevd(read_file(archives, "/event/common_func.emevd.dcx"))
    names = {}
    defeat_events = []
    for block in sorted({e // 10000 for e in entities}):
        path = f"/event/m{block // 10:02d}_{block % 10:02d}_00_00.emevd.dcx"
        data = read_file(archives, path)
        if data is None:
            raise RuntimeError(f"{path} not found")
        for entity, name_id in boss_health_bars(data):
            if name_id in npc_names:
                names.setdefault(entity, npc_names[name_id])
        defeat_events += boss_defeat_events(event_instances(parse_emevd(data), common_func), set(entities))

    merged = {}
    for entity in entities:
        if entity in BOSS_OVERRIDES:
            name, menu_area = BOSS_OVERRIDES[entity]
            area = areas.index(menu_area)
        elif entity in names:
            name, area = names[entity], block_area[map_block(10000000 + entity)]
        else:
            print(f"boss {entity}: no health bar, skipped")
            continue
        if entity in BOSS_AREAS:
            area = areas.index(BOSS_AREAS[entity])
        flags = boss_flags(entity, defeat_events, set(entities))
        if not flags:
            raise RuntimeError(f"{name} ({entity}): no defeat event sets a boss flag")
        merged.setdefault((area, name), set()).update(flags)
    out = []
    for (area, name), flags in merged.items():
        if len(flags) > MAX_BOSS_FLAGS:
            raise RuntimeError(f"{name} has {len(flags)} flags")
        out.append((area, sorted(flags), name))
    out.sort()  # boss flags follow the game's order
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
