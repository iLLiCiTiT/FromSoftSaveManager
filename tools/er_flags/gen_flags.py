"""Generate the Elden Ring event flag block table, graces and bosses.

Usage:
    python gen_flags.py <ER-Save-Editor>/src/db <repo root>

Source: ER-Save-Editor by ClayAmore (github.com/ClayAmore/ER-Save-Editor), Apache License
2.0: src/db/event_flags.rs (flag id -> byte and bit in the save's event flag region),
src/db/graces.rs and src/db/bosses.rs (base game graces and bosses with their flags).
DLC graces and bosses are not in those lists.

Writes:
    src/parse/EldenRing/WorldProgress.h      table sizes
    src/parse/EldenRing/WorldProgress.cpp    flag blocks, graces and bosses

The region stores flags in blocks of 1000 (125 bytes, bit 7 of a byte first); a block's
position is its rank in the game's block table, so each known flag gives its block's
position. Only blocks of known flags can be read.
"""
import os
import re
import sys

# graces.rs MapName -> area name, in the order the list first uses them.
GRACE_AREA_NAMES = {
    "RoundtableHold": "Roundtable Hold",
    "Limgrave": "Limgrave",
    "StrandedGraveyard": "Stranded Graveyard",
    "Stormhill": "Stormhill",
    "WeepingPeninsula": "Weeping Peninsula",
    "StormveilCastle": "Stormveil Castle",
    "LiurniaOfTheLakes": "Liurnia of the Lakes",
    "BellumHighway": "Bellum Highway",
    "RuinStrewnPrecipice": "Ruin-Strewn Precipice",
    "MoonlightAltar": "Moonlight Altar",
    "AcademyOfRayaLucaria": "Academy of Raya Lucaria",
    "AltusPlateau": "Altus Plateau",
    "MtGelmir": "Mt. Gelmir",
    "CapitalOutskirts": "Capital Outskirts",
    "VolcanoManor": "Volcano Manor",
    "LeyndellRoyalCapital": "Leyndell, Royal Capital",
    "SubterraneanShunningGrounds": "Subterranean Shunning-Grounds",
    "LeyndellAshenCapital": "Leyndell, Ashen Capital",
    "StonePlatform": "Stone Platform",
    "Caelid": "Caelid",
    "SwampOfAeonia": "Swamp of Aeonia",
    "GreyollsDragonbarrow": "Greyoll's Dragonbarrow",
    "ForbiddenLands": "Forbidden Lands",
    "MountaintopsOfTheGiants": "Mountaintops of the Giants",
    "FlamePeak": "Flame Peak",
    "ConsecratedSnowfield": "Consecrated Snowfield",
    "MiquellasHaligtree": "Miquella's Haligtree",
    "ElphaelBraceOfTheHaligtree": "Elphael, Brace of the Haligtree",
    "CrumblingFarumAzula": "Crumbling Farum Azula",
    "AinselRiver": "Ainsel River",
    "AinselRiverMain": "Ainsel River Main",
    "LakeOfRot": "Lake of Rot",
    "NokronEternalCity": "Nokron, Eternal City",
    "SiofraRiver": "Siofra River",
    "MohgwynPalace": "Mohgwyn Palace",
    "DeeprootDepths": "Deeproot Depths",
}

# bosses.rs region comments -> area name (fixing the comments' typos).
BOSS_AREA_FIXES = {
    "Weeping Weeping Peninsula": "Weeping Peninsula",
    "Academya of Raya Lucaria": "Academy of Raya Lucaria",
    "Leyendell, Royal Capital": "Leyndell, Royal Capital",
    "Leyendell, Ashen Capital": "Leyndell, Ashen Capital",
    "Consecrated Snowfields": "Consecrated Snowfield",
    "Dragonbarrow": "Greyoll's Dragonbarrow",
    "Nokron Eternal City": "Nokron, Eternal City",
}

BLOCK_SIZE = 125    # bytes per 1000 flags

# Block suffix -> position in its map's run of blocks (see flag_blocks).
MAP_RUN_OVERWORLD = {0: 0, 2: 1, 7: 4, 8: 5, 9: 6}
MAP_RUN_DUNGEON = {0: 0, 1: 1, 2: 2, 7: 6, 8: 7, 9: 8}
# Blocks no ER-Save-Editor flag lies in, located on the 5 test characters: the item lot
# flags (ItemLotParam getItemFlagId) of the block were tried at every rank between the
# neighbouring known blocks (82 at 32, 580 at 98) against the characters' inventories and
# storage; one rank matched (320 owned items with their flag set, 1 without). The same
# test finds the known ranks of blocks 65 and 11007. Block 400 holds the flags of the
# items NPCs give: letters, quest rewards, the gear they leave behind.
EXTRA_BLOCKS = {400: 80}


def read(db_dir, name):
    with open(os.path.join(db_dir, name), encoding="utf-8") as f:
        return f.read()


def flag_blocks(db_dir):
    """Block id (flag / 1000) -> rank, checked against every known flag."""
    blocks = {}
    for flag, byte, bit in re.findall(r"\((\d+)\s*,\s*\(\s*0x([0-9a-fA-F]+)\s*,\s*(\d+)\s*\)\s*\)",
                                      read(db_dir, "event_flags.rs")):
        flag, byte, bit = int(flag), int(byte, 16), int(bit)
        position = byte * 8 + 7 - bit
        if position % 1000 != flag % 1000:
            raise RuntimeError(f"flag {flag} is not at its block offset")
        rank = position // 1000
        if blocks.setdefault(flag // 1000, rank) != rank:
            raise RuntimeError(f"block {flag // 1000} has two positions")
    # The game's table is sorted by block id, so where two known blocks are as many ranks
    # apart as ids apart, every id between is in the table (e.g. block 3, the NPC flags
    # 3000-3999, between blocks 2 and 4).
    known = sorted(blocks.items())
    for (id_a, rank_a), (id_b, rank_b) in zip(known, known[1:]):
        if id_b - id_a == rank_b - rank_a:
            for k in range(1, id_b - id_a):
                blocks[id_a + k] = rank_a + k
    # A map owns a run of consecutive blocks, map id * 10 + suffix, with a fixed set of
    # suffixes: overworld tiles (1AAXXYY, 7 blocks) 0, 2, two unknown, 7, 8, 9; legacy
    # dungeons and the like (AABB, 9 blocks) 0, 1, 2, three unknown, 7, 8, 9. Every known
    # block fits these runs, so one known block of a map gives the map's other suffixes
    # (e.g. Ranni's Rise 1034509xxx from block 1034500).
    runs = {}
    for block, rank in list(blocks.items()):
        offsets = (MAP_RUN_OVERWORLD if block >= 1000000 else MAP_RUN_DUNGEON if block >= 10000 else None)
        if offsets is None or block % 10 not in offsets:
            continue
        start = rank - offsets[block % 10]
        if runs.setdefault(block // 10, (offsets, start)) != (offsets, start):
            raise RuntimeError(f"block {block} breaks its map's run")
    for map_id, (offsets, start) in runs.items():
        for suffix, offset in offsets.items():
            blocks.setdefault(map_id * 10 + suffix, start + offset)
    for block, rank in EXTRA_BLOCKS.items():
        if blocks.setdefault(block, rank) != rank:
            raise RuntimeError(f"block {block} has two positions")
    return blocks


def map_section(text):
    """The HashMap::from([...]) body, where each entry carries its flag."""
    return text[text.index("HashMap::from"):]


def graces(db_dir):
    text = map_section(read(db_dir, "graces.rs"))
    out = []
    for map_name, flag, name in re.findall(r"\(\s*Grace::\w+\s*,\s*\(\s*MapName::(\w+)\s*,\s*(\d+)\s*,\s*\"([^\"]*)\"",
                                           text):
        out.append((GRACE_AREA_NAMES[map_name], int(flag), name))
    return out


def bosses(db_dir):
    text = map_section(read(db_dir, "bosses.rs"))
    out = []
    area = None
    for line in text.splitlines():
        comment = re.match(r"\s*//\s*(.+?)\s*$", line)
        if comment:
            area = BOSS_AREA_FIXES.get(comment.group(1), comment.group(1))
            continue
        for flag, name in re.findall(r"\(\s*Boss::\w+\s*,\s*\(\s*(\d+)\s*,\s*\"([^\"]*)\"", line):
            out.append((area, int(flag), name))
    return out


def areas_in_order(entries):
    areas = []
    for area, _flag, _name in entries:
        if area not in areas:
            areas.append(area)
    return areas


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_crlf(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


def table(lines, struct_name, var, entries, areas):
    lines += ["", f"    const std::array<{struct_name}, {len(entries)}> {var} = {{"]
    for area, flag, name in entries:
        lines.append(f"        {struct_name}{{{flag}, {areas.index(area)}, {cpp_string(name)}}},")
    lines.append("    };")


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    db_dir, repo = sys.argv[1:]
    blocks = flag_blocks(db_dir)
    grace_table = graces(db_dir)
    boss_table = bosses(db_dir)
    for _area, flag, name in grace_table + boss_table:
        if flag // 1000 not in blocks:
            raise RuntimeError(f"{name}: flag {flag} is in no known block")
    grace_areas = areas_in_order(grace_table)
    boss_areas = areas_in_order(boss_table)
    max_rank = max(blocks.values())

    out_dir = os.path.join(repo, "src", "parse", "EldenRing")
    write_crlf(os.path.join(out_dir, "WorldProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/er_flags/gen_flags.py from ER-Save-Editor's tables
// (github.com/ClayAmore/ER-Save-Editor, Apache License 2.0); do not edit.
namespace fssm::parse::er {{
    struct FlagBlock {{
        uint32_t blockId;               // flag id / 1000
        uint32_t rank;                  // position in the event flag region, in {BLOCK_SIZE} byte blocks
    }};

    struct Grace {{
        uint32_t flagId;                // set when the grace is discovered
        uint8_t area;                   // index into GRACE_AREAS
        std::string_view name;
    }};

    struct Boss {{
        uint32_t flagId;                // set when defeated
        uint8_t area;                   // index into BOSS_AREAS
        std::string_view name;
    }};

    constexpr size_t FLAG_BLOCK_SIZE = {BLOCK_SIZE};
    // Known flag blocks, sorted by blockId.
    extern const std::array<FlagBlock, {len(blocks)}> FLAG_BLOCKS;

    extern const std::array<std::string_view, {len(grace_areas)}> GRACE_AREAS;
    // Sites of Grace of the base game, by area.
    extern const std::array<Grace, {len(grace_table)}> GRACES;
    extern const std::array<std::string_view, {len(boss_areas)}> BOSS_AREAS;
    // Bosses of the base game, by area.
    extern const std::array<Boss, {len(boss_table)}> BOSSES;
}}
""")

    lines = [
        '#include "WorldProgress.h"',
        "",
        "// Generated by tools/er_flags/gen_flags.py from ER-Save-Editor's tables",
        "// (github.com/ClayAmore/ER-Save-Editor, Apache License 2.0); do not edit.",
        "namespace fssm::parse::er {",
        f"    const std::array<FlagBlock, {len(blocks)}> FLAG_BLOCKS = {{{{",
    ]
    items = [f"{{{block}, {rank}}}" for block, rank in sorted(blocks.items())]
    for i in range(0, len(items), 6):
        lines.append("        " + ", ".join(items[i:i + 6]) + ",")
    lines.append("    }};")
    for var, areas in (("GRACE_AREAS", grace_areas), ("BOSS_AREAS", boss_areas)):
        lines += ["", f"    const std::array<std::string_view, {len(areas)}> {var} = {{"]
        lines += [f"        {cpp_string(a)}," for a in areas]
        lines.append("    };")
    table(lines, "Grace", "GRACES", grace_table, grace_areas)
    table(lines, "Boss", "BOSSES", boss_table, boss_areas)
    lines += ["}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(blocks)} flag blocks (region needs {(max_rank + 1) * BLOCK_SIZE:#x} bytes), "
          f"{len(grace_table)} graces, {len(boss_table)} bosses")


if __name__ == "__main__":
    main()
