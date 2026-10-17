"""Generate the Elden Ring NPC table from the installed game data.

Usage:
    python gen_npcs.py "<...>/ELDEN RING/Game" <repo root>

Writes:
    src/parse/EldenRing/NpcProgress.h      table sizes
    src/parse/EldenRing/NpcProgress.cpp    NPCs

Every NPC owns a block of 20 global event flags in 3000-4999 (base % 20 == 0). The first
five (S = base .. base + 4) are its status: the scripts clear them (Batch Set Event Flags
S..S+3 or S..S+4 off) and set one, S friendly, S + 1 / S + 2 hostile, S + 3 dead, as in
Dark Souls III. The rest of the block holds the questline.

The scripts are the map event scripts (/event/<map>.emevd.dcx, EMEVD 0xCD) with the shared
events of common_func.emevd (Initialize Common Event 2000:06: slot, event id, args). The NPC
is the character the status events act on (IF Character Dead/Alive 4:00, IF Character HP
Ratio 4:02) -> map MSB enemy part (MSBE: type at +0xC, entity data at the offset at +0x60,
type data at the offset at +0x68; entity id at entity data +0, NpcParam id at type data
+0xC, talk id at +0x10) -> NpcParam +0xC name id -> NpcName.fmg (item, item_dlc01,
item_dlc02 msgbnd). Blocks whose events name no character are named by the talk script
that reads the block's flags most (t<talk id>.esd in /script/talk/<map>.talkesdbnd.dcx ->
the MSB parts with that talk id). Blocks named by neither are left out.

Requires pycryptodome and zstandard (see tools/er_items/erdata.py).
"""
import collections
import os
import re
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "er_items"))
import erdata  # noqa: E402

NPC_FLAG_MIN, NPC_FLAG_MAX = 3000, 5000
NPC_BLOCK = 20
NPC_STATUS = 5
ENEMY_PART = 2
ITEM_MSG = ("/msg/engus/item.msgbnd.dcx", "/msg/engus/item_dlc01.msgbnd.dcx", "/msg/engus/item_dlc02.msgbnd.dcx")
# Characters NpcName calls the same; numbered in flag order, e.g. "Nomadic Merchant (2)".
NPC_NAME_PLACEHOLDERS = {"DLC dummy"}


def map_names():
    """Candidate map ids: legacy dungeons mAA_BB_CC_00 and the overworld tiles of m60/m61."""
    names = [f"m{aa}_{bb:02d}_{cc:02d}_00" for aa in range(10, 46) for bb in range(100) for cc in (0, 1)]
    names += [f"m{aa}_{x:02d}_{y:02d}_{z:02d}" for aa in (60, 61) for x in range(70) for y in range(70)
              for z in (0, 1, 2, 10, 11, 12)]
    return names


def parse_emevd(data):
    """EMEVD 0xCD: {event id: [(bank, id, args, [(target, source, size)])]}."""
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


def event_instances(map_events, common_events):
    """Instructions of every initialized event with its arguments filled in, plus the map's
    events that run without initialization."""
    initialized = set()
    for instrs in map_events.values():
        for bank, index, args, _params in instrs:
            if (bank, index) == (2000, 0):      # Initialize Event: slot, event id, args
                events = map_events
            elif (bank, index) == (2000, 6):    # Initialize Common Event: slot, event id, args
                events = common_events
            else:
                continue
            event_id = struct.unpack_from("<I", args, 4)[0]
            if event_id in events:
                if events is map_events:
                    initialized.add(event_id)
                yield [(b, i, substitute(a, p, args[8:])) for b, i, a, p in events[event_id]]
    for event_id, instrs in map_events.items():
        if event_id not in initialized:
            yield [(b, i, a) for b, i, a, _p in instrs]


def status_blocks(instances):
    """{block base: Counter(character entity)} of the events that set a status flag."""
    blocks = collections.defaultdict(collections.Counter)
    for instrs in instances:
        characters = set()
        for bank, index, args in instrs:
            if bank == 4 and index in (0, 2):   # IF Character Dead/Alive, IF Character HP Ratio
                characters.add(struct.unpack_from("<I", args, 4)[0])
        cleared = []
        for bank, index, args in instrs:
            if bank == 2003 and index in (22, 63) and len(args) >= 9 and args[8] == 0:
                start, end = struct.unpack_from("<II", args, 0)
                if NPC_FLAG_MIN <= start < NPC_FLAG_MAX and start % NPC_BLOCK == 0 and end - start in (3, 4):
                    cleared.append(start)
            elif bank == 2003 and index in (66, 69) and len(args) >= 9 and args[8] == 1:
                flag = struct.unpack_from("<I", args, 4)[0]
                for start in cleared[-2:]:
                    if start <= flag < start + NPC_STATUS:
                        blocks[start].update(characters or {0})
    return blocks


def msb_enemies(data):
    """MSBE enemy parts: {entity id: (NpcParam id, talk id)}."""
    lists, offset = {}, 0x10
    while offset:
        _version, count, name_offset = struct.unpack_from("<iiq", data, offset)
        entries = struct.unpack_from(f"<{count - 1}q", data, offset + 16)
        end = name_offset
        while data[end:end + 2] != b"\0\0":
            end += 2
        lists[data[name_offset:end].decode("utf-16le", "ignore")] = entries
        offset = struct.unpack_from("<q", data, offset + 16 + (count - 1) * 8)[0]
    out = {}
    for part in lists["PARTS_PARAM_ST"]:
        if struct.unpack_from("<i", data, part + 0xC)[0] != ENEMY_PART:
            continue
        entity_offset, type_offset = struct.unpack_from("<qq", data, part + 0x60)
        entity = struct.unpack_from("<I", data, part + entity_offset)[0]
        npc, talk = struct.unpack_from("<ii", data, part + type_offset + 0xC)
        out[entity] = (npc, talk)
    return out


def npcs(archives):
    """[(status base, name)] of the named NPCs, by flag."""
    maps = [m for m in map_names() if archives.has(f"/event/{m}.emevd.dcx")]
    common = parse_emevd(archives.read("/event/common_func.emevd.dcx"))
    blocks = collections.defaultdict(collections.Counter)
    for script in ["common"] + maps:
        for base, chars in status_blocks(event_instances(parse_emevd(archives.read(f"/event/{script}.emevd.dcx")),
                                                         common)).items():
            blocks[base].update(chars)

    npc_rows = erdata.parse_param(archives.regulation()["NpcParam.param"])
    names = {}
    for path in ITEM_MSG:
        if archives.has(path):
            for name, fmg in archives.bnd(path).items():
                if os.path.basename(name.replace("\\", "/")).startswith("NpcName"):
                    names.update(erdata.parse_fmg2(fmg))
    entity_names, talk_names = {}, collections.defaultdict(collections.Counter)
    for m in maps:
        path = f"/map/mapstudio/{m}.msb.dcx"
        if not archives.has(path):
            continue
        for entity, (npc, talk) in msb_enemies(archives.read(path)).items():
            row = npc_rows.get(npc)
            name = names.get(struct.unpack_from("<i", row, 0xC)[0]) if row else None
            if name and name not in NPC_NAME_PLACEHOLDERS:
                entity_names[entity] = name
                if talk > 0:
                    talk_names[talk][name] += 1

    # Talk scripts reading each block's flags.
    talk_reads = collections.defaultdict(collections.Counter)
    for m in maps:
        path = f"/script/talk/{m}.talkesdbnd.dcx"
        if not archives.has(path):
            continue
        for fname, esd in archives.bnd(path).items():
            match = re.search(r"t(\d+)\.esd$", fname)
            if not match:
                continue
            for literal in re.finditer(rb"\x82(....)", esd, re.S):    # ESD int32 literal
                value = struct.unpack("<i", literal.group(1))[0]
                if NPC_FLAG_MIN <= value < NPC_FLAG_MAX:
                    talk_reads[value - value % NPC_BLOCK][int(match.group(1))] += 1

    out = []
    for base in sorted(blocks):
        votes = collections.Counter()
        for entity, n in blocks[base].items():
            if entity in entity_names:
                votes[entity_names[entity]] += n
        if not votes:
            for talk, _n in talk_reads[base].most_common(1):
                votes = talk_names.get(talk, collections.Counter())
        if votes:
            out.append((base, votes.most_common(1)[0][0]))
    seen = collections.Counter(name for _, name in out)
    numbered, count = [], collections.Counter()
    for base, name in out:
        count[name] += 1
        numbered.append((base, f"{name} ({count[name]})" if seen[name] > 1 else name))
    return numbered


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_crlf(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    game_dir, repo = sys.argv[1:]
    archives = erdata.Archives(game_dir)
    table = npcs(archives)
    out_dir = os.path.join(repo, "src", "parse", "EldenRing")
    write_crlf(os.path.join(out_dir, "NpcProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/er_flags/gen_npcs.py, do not edit.
namespace fssm::parse::er {{
    // Status flags of an NPC, exactly one set once its map has loaded:
    // statusFlag friendly, + 1 or + 2 hostile, + 3 dead.
    struct Npc {{
        uint32_t statusFlag;
        std::string_view name;
    }};

    // NPCs with a status flag block, by flag.
    extern const std::array<Npc, {len(table)}> NPCS;
}}
""")
    lines = ['#include "NpcProgress.h"', "", "// Generated by tools/er_flags/gen_npcs.py, do not edit.",
             "namespace fssm::parse::er {", f"    const std::array<Npc, {len(table)}> NPCS = {{"]
    lines += [f"        Npc{{{base}, {cpp_string(name)}}}," for base, name in table]
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "NpcProgress.cpp"), "\n".join(lines))
    print(f"{len(table)} NPCs")


if __name__ == "__main__":
    main()
