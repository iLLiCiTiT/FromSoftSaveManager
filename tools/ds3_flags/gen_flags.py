"""Generate the Dark Souls III bonfire, boss and NPC tables from the installed game data.

Usage:
    python gen_flags.py "<...>/DARK SOULS III/Game" <DecryptionKeys.cs> <repo root>

The DS3 executable is packed, so the archive RSA keys are not in it as PEM text. Pass any
file that contains them as "-----BEGIN RSA PUBLIC KEY-----" blocks, e.g. BinderTool's
BinderTool/DecryptionKeys.cs (github.com/Atvaark/BinderTool).

Writes:
    src/parse/DS3/WorldProgress.h      table sizes
    src/parse/DS3/WorldProgress.cpp    bonfires (BonfireWarpParam), bosses and NPCs

Bonfires come from BonfireWarpParam in the regulation (Data0.bdt, AES-256-CBC with the
key below, IV = first 16 bytes): +0x0 event flag set when the bonfire is lit, +0x8 name
(FDP_メニューテキスト[_dlc1|_dlc2].fmg in /msg/engus/menu*.msgbnd.dcx), +0x10 sort id,
+0x14 travel menu area (AREAS; 99 = rows the menu doesn't list). Rows that share a name
are one bonfire (e.g. High Wall of Lothric before and after its first visit), lit when
any of their flags is set.

Boss defeat flags are not in the game params; BOSSES was checked against saves (each
map's 8xx flags of the first thousand, matching the bonfire named after the boss).

NPCs come from the map event scripts (/event/mAA_BB_00_00.emevd.dcx). Each NPC owns a
block of 20 global flags; the last five (S = base + 15 .. base + 19) are its status and
exactly one is set: S friendly, S + 1 / S + 2 hostile, S + 3 dead or gone for good. The
scripts always clear the five and then set one (Batch Set Event Flags S..S+4 off, Set
Event Flag S+k), and the common events show which is which: 20006002 sets S + 3 when
the character dies, 20006000 sets S + 1 or S + 2 when it turns hostile (both count as
hostile there). The NPC is the character in the same event instance (IF Character
Dead/Alive, IF Character HP Ratio or Set Character Team Type 27) -> map MSB enemy part
-> NpcParam +0x8 name id -> NPC名[_dlc1|_dlc2].fmg in /msg/engus/item*.msgbnd.dcx.
Blocks whose characters have no name (DLC side characters) and bosses (name ids from
900000) are left out. The first 15 flags of a block are questline stages; the game data
doesn't name them.

Requires pycryptodome.
"""
import collections
import os
import struct
import sys
import zlib

from Crypto.Cipher import AES

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "sekiro_items"))
import sekirodata  # noqa: E402
import ds2data  # noqa: E402

REGULATION_KEY = b"ds3#jn/8_7(rsY9pg55GFN7VFL#+3n/)"
ARCHIVES = ("Data1", "Data2", "Data3", "Data4", "Data5", "DLC1", "DLC2")
# Later ones override earlier ones; not every install has all of them.
MENU_MSG = ("/msg/engus/menu.msgbnd.dcx", "/msg/engus/menu_dlc1.msgbnd.dcx", "/msg/engus/menu_dlc2.msgbnd.dcx")
MENU_FMG_PREFIX = "FDP_メニューテキスト"
ITEM_MSG = ("/msg/engus/item.msgbnd.dcx", "/msg/engus/item_dlc1.msgbnd.dcx", "/msg/engus/item_dlc2.msgbnd.dcx")
NPC_FMG_PREFIX = "NPC名"
HIDDEN_AREA = 99

# Map areas and blocks to look for event scripts and MSBs in.
MAP_AREAS = range(30, 56)
MAP_BLOCKS = range(0, 3)
COMMON_FUNC = "/event/common_func.emevd.dcx"
# NPC status flags are global flags base + 15 .. base + 19 of a 20 flag block.
NPC_FLAG_MIN, NPC_FLAG_MAX = 1000, 2000
NPC_TEAM_TYPE = 27              # Set Character Team Type: hostile NPC
BOSS_NAME_ID = 900000           # NPC name ids from here on are bosses

# Travel menu areas, by BonfireWarpParam +0x14.
AREAS = [
    "Firelink Shrine",
    "High Wall of Lothric",
    "Undead Settlement",
    "Road of Sacrifices",
    "Cathedral of the Deep",
    "Catacombs of Carthus",
    "Irithyll of the Boreal Valley",
    "Irithyll Dungeon",
    "Lothric Castle",
    "Archdragon Peak",
    "Kiln of the First Flame",
    "Painted World of Ariandel",
    "The Dreg Heap",
    "The Ringed City",
]

# (defeat flag, travel menu area, name), in the order they are usually met.
BOSSES = [
    (14000800, 0, "Iudex Gundyr"),
    (13000800, 1, "Vordt of the Boreal Valley"),
    (13100800, 2, "Curse-Rotted Greatwood"),
    (13300850, 3, "Crystal Sage"),
    (13500800, 4, "Deacons of the Deep"),
    (13300800, 3, "Abyss Watchers"),
    (13800800, 5, "High Lord Wolnir"),
    (13800830, 5, "Old Demon King"),
    (13700850, 6, "Pontiff Sulyvahn"),
    (13900800, 7, "Yhorm the Giant"),
    (13700800, 6, "Aldrich, Devourer of Gods"),
    (13000890, 1, "Dancer of the Boreal Valley"),
    (13010800, 8, "Dragonslayer Armour"),
    (13000830, 1, "Oceiros, the Consumed King"),
    (14000830, 0, "Champion Gundyr"),
    (13200800, 9, "Ancient Wyvern"),
    (13200850, 9, "Nameless King"),
    (13410830, 8, "Lothric, Younger Prince"),
    (14100800, 10, "Soul of Cinder"),
    (14500860, 11, "Champion's Gravetender"),
    (14500800, 11, "Sister Friede"),
    (15000800, 12, "Demon Prince"),
    (15100800, 13, "Halflight, Spear of the Church"),
    (15100850, 13, "Darkeater Midir"),
    (15110800, 13, "Slave Knight Gael"),
]

MAX_BONFIRE_FLAGS = 4


class Archive(sekirodata.Archive):
    """DS3 BHD5 archive: like Sekiro's, but DCX files are DFLT only (no Oodle)."""

    def __init__(self, game_dir, keys_path, name):
        with open(os.path.join(game_dir, f"{name}.bhd"), "rb") as f:
            raw = f.read()
        self.bhd = None
        for n, e in sekirodata.embedded_rsa_keys(keys_path):
            data = ds2data._rsa_decrypt(raw, n, e)
            if data[:4] == b"BHD5":
                self.bhd = data
                break
        if self.bhd is None:
            raise RuntimeError(f"no key decrypts {name}.bhd")
        self.bdt_path = os.path.join(game_dir, f"{name}.bdt")
        self.oodle = None
        bucket_count, buckets_offset = struct.unpack_from("<II", self.bhd, 16)
        self.entries = {}
        for i in range(bucket_count):
            count, offset = struct.unpack_from("<II", self.bhd, buckets_offset + i * 8)
            for j in range(count):
                h, size, data_offset, _sha, aes_offset, unpadded = struct.unpack_from(
                    "<IIqqqq", self.bhd, offset + j * 0x28)
                self.entries[h] = (size, data_offset, aes_offset, unpadded)


def read_file(archives, path):
    h = sekirodata.path_hash(path)
    for archive in archives:
        if h in archive.entries:
            return archive.read_hash(h)
    return None


def fmg_text(archives, paths, prefix):
    text = {}
    for path in paths:
        data = read_file(archives, path)
        if data is None:
            continue
        for name, fmg in sekirodata.parse_bnd4(data).items():
            if os.path.basename(name.replace("\\", "/")).startswith(prefix):
                text.update(sekirodata.parse_fmg2(fmg))
    if not text:
        raise RuntimeError(f"no {prefix} text found")
    return text


def menu_text(archives):
    return fmg_text(archives, MENU_MSG, MENU_FMG_PREFIX)


def regulation_params(game_dir):
    with open(os.path.join(game_dir, "Data0.bdt"), "rb") as f:
        data = f.read()
    dcx = AES.new(REGULATION_KEY, AES.MODE_CBC, data[:16]).decrypt(data[16:])
    assert dcx[:4] == b"DCX\0" and dcx[0x28:0x2C] == b"DFLT"
    compressed = struct.unpack_from(">I", dcx, 0x20)[0]
    start = dcx.index(b"DCA\0") + 8
    bnd = zlib.decompress(dcx[start:start + compressed])
    return {os.path.basename(k.replace("\\", "/")): v for k, v in sekirodata.parse_bnd4(bnd).items()}


def bonfires(params, text):
    rows = sekirodata.parse_param(params["BonfireWarpParam.param"])
    by_text = {}
    for row in rows.values():
        flag, _entity, text_id, _unk, sort_id = struct.unpack_from("<iiiii", row, 0)
        area = row[0x14]
        if flag <= 0:
            continue
        entry = by_text.setdefault(text_id, {"flags": [], "area": None, "sort": None})
        if flag not in entry["flags"]:
            entry["flags"].append(flag)
        if area != HIDDEN_AREA and (entry["sort"] is None or sort_id < entry["sort"]):
            entry["area"], entry["sort"] = area, sort_id
    out = []
    for text_id, entry in by_text.items():
        if entry["area"] is None:
            continue
        if entry["area"] >= len(AREAS):
            raise RuntimeError(f"unknown travel menu area {entry['area']}")
        if len(entry["flags"]) > MAX_BONFIRE_FLAGS:
            raise RuntimeError(f"bonfire {text_id} has {len(entry['flags'])} flags")
        # The flag the menu row uses first, then the hidden rows' flags.
        out.append((entry["area"], entry["sort"], entry["flags"], text[text_id]))
    out.sort()
    return out


def parse_emevd(data):
    """EMEVD version 0xCD (64-bit): {event id: [(bank, id, args, [(target, source, size)])]}."""
    assert data[:4] == b"EVD\0" and struct.unpack_from("<I", data, 8)[0] == 0xCD
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


def event_instances(map_events, common_events):
    """Instructions of every initialized event with its arguments filled in, plus the events
    that run without initialization (e.g. event 0)."""
    initialized = set()
    for instrs in map_events.values():
        for bank, index, args, _params in instrs:
            if (bank, index) == (2000, 0):      # Initialize Event: slot, event id, args
                event_id, values, events = struct.unpack_from("<I", args, 4)[0], args[8:], map_events
            elif (bank, index) == (2000, 6):    # Initialize Common Event: event id, args
                event_id, values, events = struct.unpack_from("<I", args, 0)[0], args[4:], common_events
            else:
                continue
            if event_id in events:
                initialized.add(event_id)
                yield [(b, i, substitute(a, p, values)) for b, i, a, p in events[event_id]]
    for event_id, instrs in map_events.items():
        if event_id not in initialized:
            yield [(b, i, a) for b, i, a, _p in instrs]


def substitute(args, params, values):
    out = bytearray(args)
    for target, source, size in params:
        if source + size <= len(values):
            out[target:target + size] = values[source:source + size]
    return bytes(out)


def npc_status_blocks(instances):
    """{status base: {offset set: set of characters in the same event}}."""
    blocks = collections.defaultdict(lambda: collections.defaultdict(set))
    for instrs in instances:
        dying, hostile = set(), set()
        for bank, index, args in instrs:
            if (bank, index) == (4, 0) and args[8] == 1:            # IF Character Dead/Alive: dead
                dying.add(struct.unpack_from("<i", args, 4)[0])
            elif (bank, index) == (4, 2):                           # IF Character HP Ratio
                dying.add(struct.unpack_from("<i", args, 4)[0])
            elif (bank, index) == (2004, 2) and args[4] == NPC_TEAM_TYPE:
                hostile.add(struct.unpack_from("<i", args, 0)[0])
        for (bank, index, args), (bank2, index2, args2) in zip(instrs, instrs[1:]):
            # Batch Set (Network-Connected) Event Flags start, end, off; then Set flag on.
            if bank != 2003 or index not in (22, 63) or bank2 != 2003 or index2 not in (2, 62):
                continue
            start, end = struct.unpack_from("<ii", args, 0)
            flag = struct.unpack_from("<i", args2, 0)[0]
            if end - start != 4 or args[8] != 0 or args2[4] != 1 or not start <= flag <= end:
                continue
            if not NPC_FLAG_MIN <= start < NPC_FLAG_MAX:
                continue
            offset = flag - start
            owners = dying if offset == 3 else hostile if offset in (1, 2) else set()
            blocks[start][offset] |= owners
    return blocks


def msb_npc_params(data):
    """MSB3 enemy parts: {entity id: NpcParam id}."""
    lists, offset = {}, 0x10
    while offset:
        _version, count, name_offset = struct.unpack_from("<iiq", data, offset)
        entries = struct.unpack_from(f"<{count - 1}q", data, offset + 16)
        name = data[name_offset:data.index(b"\0\0", name_offset)].decode("ascii", "ignore")
        lists[name.replace("\0", "")] = entries
        offset = struct.unpack_from("<q", data, offset + 16 + (count - 1) * 8)[0]
    out = {}
    for part in lists["PARTS_PARAM_ST"]:
        part_type = struct.unpack_from("<i", data, part + 8)[0]
        if part_type != 2:      # enemy
            continue
        entity_offset, type_offset = struct.unpack_from("<qq", data, part + 0xB0)
        entity = struct.unpack_from("<i", data, part + entity_offset)[0]
        if entity > 0:
            out[entity] = struct.unpack_from("<i", data, part + type_offset + 0xC)[0]
    return out


def npcs(archives, params):
    """[(status base, name)] for the named NPCs, by status base."""
    common = parse_emevd(read_file(archives, COMMON_FUNC))
    npc_params = {}
    blocks = collections.defaultdict(lambda: collections.defaultdict(set))
    for area in MAP_AREAS:
        for block in MAP_BLOCKS:
            map_name = f"m{area:02d}_{block:02d}_00_00"
            msb = read_file(archives, f"/map/mapstudio/{map_name}.msb.dcx")
            if msb is not None:
                npc_params.update(msb_npc_params(msb))
            script = read_file(archives, f"/event/{map_name}.emevd.dcx")
            if script is None:
                continue
            for base, offsets in npc_status_blocks(event_instances(parse_emevd(script), common)).items():
                for offset, owners in offsets.items():
                    blocks[base][offset] |= owners
    npc_rows = sekirodata.parse_param(params["NpcParam.param"])
    names = fmg_text(archives, ITEM_MSG, NPC_FMG_PREFIX)
    out = []
    for base, offsets in sorted(blocks.items()):
        votes = collections.Counter()
        for owners in offsets.values():
            for entity in owners:
                row = npc_rows.get(npc_params.get(entity))
                name_id = struct.unpack_from("<i", row, 8)[0] if row else 0
                if name_id in names and name_id < BOSS_NAME_ID:
                    votes[names[name_id]] += 1
        if votes:
            name = votes.most_common(1)[0][0]
            # The Shrine Handmaid has two blocks (two characters at the same spot).
            seen = sum(1 for _, n in out if n == name or n.startswith(name + " ("))
            out.append((base, f"{name} ({seen + 1})" if seen else name))
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
    game_dir, keys_path, repo = sys.argv[1:]
    archives = [Archive(game_dir, keys_path, name) for name in ARCHIVES]
    params = regulation_params(game_dir)
    table = bonfires(params, menu_text(archives))
    npc_table = npcs(archives, params)

    out_dir = os.path.join(repo, "src", "parse", "DS3")
    write_crlf(os.path.join(out_dir, "WorldProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/ds3_flags/gen_flags.py, do not edit.
namespace fssm::parse::ds3 {{
    struct Bonfire {{
        // Event flags set when the bonfire is lit (any of them); 0 when unused.
        std::array<uint32_t, {MAX_BONFIRE_FLAGS}> flagIds;
        uint8_t area;                   // index into AREAS
        std::string_view name;
    }};

    struct Boss {{
        uint32_t flagId;                // set when defeated
        uint8_t area;                   // index into AREAS
        std::string_view name;
    }};

    // Status flags of an NPC, exactly one set once its map has loaded:
    // statusFlag friendly, + 1 or + 2 hostile, + 3 dead (or gone for good).
    struct Npc {{
        uint32_t statusFlag;
        std::string_view name;
    }};

    // Areas of the bonfire travel menu, in menu order.
    extern const std::array<std::string_view, {len(AREAS)}> AREAS;
    // Bonfires in travel menu order.
    extern const std::array<Bonfire, {len(table)}> BONFIRES;
    // Bosses in the order they are usually met.
    extern const std::array<Boss, {len(BOSSES)}> BOSSES;
    // NPCs with a status flag block, by flag.
    extern const std::array<Npc, {len(npc_table)}> NPCS;
}}
""")

    lines = [
        '#include "WorldProgress.h"',
        "",
        "// Generated by tools/ds3_flags/gen_flags.py, do not edit.",
        "namespace fssm::parse::ds3 {",
        f"    const std::array<std::string_view, {len(AREAS)}> AREAS = {{",
    ]
    lines += [f"        {cpp_string(a)}," for a in AREAS]
    lines += ["    };", "", f"    const std::array<Bonfire, {len(table)}> BONFIRES = {{"]
    for area, _sort, flags, name in table:
        ids = ", ".join(str(f) for f in flags + [0] * (MAX_BONFIRE_FLAGS - len(flags)))
        lines.append(f"        Bonfire{{{{{ids}}}, {area}, {cpp_string(name)}}},")
    lines += ["    };", "", f"    const std::array<Boss, {len(BOSSES)}> BOSSES = {{"]
    for flag, area, name in BOSSES:
        lines.append(f"        Boss{{{flag}, {area}, {cpp_string(name)}}},")
    lines += ["    };", "", f"    const std::array<Npc, {len(npc_table)}> NPCS = {{"]
    for flag, name in npc_table:
        lines.append(f"        Npc{{{flag}, {cpp_string(name)}}},")
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(table)} bonfires, {len(BOSSES)} bosses, {len(npc_table)} NPCs")


if __name__ == "__main__":
    main()
