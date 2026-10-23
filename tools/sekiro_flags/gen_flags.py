"""Generate the Sekiro idol and boss tables from the installed game data.

Usage:
    python gen_flags.py "<...>/Sekiro" <sekiro exe> <repo root>

The exe only provides the archive keys (see tools/sekiro_items/sekirodata.py).

Writes:
    src/parse/Sekiro/WorldProgress.h      table sizes
    src/parse/Sekiro/WorldProgress.cpp    idols (BonfireWarpParam), bosses, NPCs, prosthetic tools

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

NPCs: every NPC owns a block of 20 global flags in 1000-1999. The map scripts clear base ..
base + 14 and set one of them (the questline stage); common_func 20006002 sets base + 19
when the character dies, 20006000 / 20006007 set base + 15 or + 16 when it turns hostile.
NPCS below lists the blocks with their names, NPC_STAGES what each stage flag means and
NPC_STEPS the progress steps shown under each NPC (docs/sekiro-npc-stages.md). The
generator fails when a listed flag is set by no map script (/event/*.emevd.dcx with
common_func) and no talk script (/script/talk/*.talkesdbnd.dcx), or when a script sets a
stage flag of a listed NPC that has no text.

Dragonrot: each sickened NPC has 5 flags; common event 50 sets group, then group + 1 and
group + 2 as the sickness spreads (one per death of the player), and a cure event per NPC
(common 9050 .. 9066) clears them (Batch Set Event Flags off). The cure event checks the
player's "Rot Essence: <name>" key item (IfPlayerHasItem goods 9500 ..) and the NPC's flag
blocks, which tie each group to its NPC. An NPC is sick while any flag of its group is set.

Prosthetic tools: the ten base tools are the weapons 70000, 71000, ... 79000 (names from
武器名.fmg); a tool counts as found when the inventory holds it or one of its upgrades
(weapon ids base .. base + 999, upgrades of the tool keep its first three digits).

Requires pycryptodome.
"""
import collections
import os
import re
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

MAPS = ("m10_00", "m11_00", "m11_01", "m11_02", "m13_00", "m15_00", "m17_00", "m20_00", "m25_00")
NPC_FLAG_MIN, NPC_FLAG_MAX = 1000, 2000
NPC_STAGE_COUNT = 15            # base .. base + 14
PROSTHETIC_TOOL_IDS = range(70000, 80000, 1000)

# Flags used below that are not NPC flags (event scripts, see docs/sekiro-npc-stages.md):
#   9302 Lady Butterfly, 9303 Genichiro, Way of Tomoe, 9308 Great Shinobi Owl, 9312 Isshin,
#   the Sword Saint, 9315 Emma, the Gentle Blade, 9316 Isshin Ashina, 9317 Owl (Father):
#   defeat flags set by the boss events (11005800 ... next to HandleBossDefeat).
#   8301 set by common event 910 when the player has the Mortal Blade, the Lotus of the
#   Palace and the Shelter Stone; 8302 by event 911 when the Divine Dragon (9310) falls.
#   6830 .. 6833 the endings, kept in New Game+ (20 .. 23 for the current cycle): m11_02
#   event 11120110 after Isshin, the Sword Saint picks the ending from Kuro's talk menu in
#   t112111 ("Give Divine Dragon's Tears" -> 11125707 -> 6830 Immortal Severance, "Give
#   Dragon's Tears and Everblossom" -> 11125708 -> 6831 Purification, "Give Dragon Tears and
#   Frozen Tears" -> 11125709 -> 6832 Return); m11_01 event 11110100 sets 6833 (Shura) after
#   Isshin Ashina.

# (first flag, name, dead flags, hostile flags). Names: NpcParam +0x8 (NPC名.fmg) of the
# character the block's events act on; where that is 0, the character's own dialogue
# (会話.fmg) and the MSB talk event name (Talk_<name>).
NPCS = [
    (1000, "Sculptor", [1019], []),
    (1020, "Kuro, the Divine Heir", [], []),                # NpcParam "Kuro the Divine Heir"
    (1040, "Divine Child of Rejuvenation", [], []),
    (1060, "Hanbei the Undying", [], []),
    (1120, "Inosuke's Mother", [], []),                     # Talk_鈴ばばあ, "Oh... is that you, Inosuke?"
    (1200, "Anayama the Peddler", [1219], []),
    (1240, "Owl (Father)", [9317], []),                     # Hirata Estate, Talk_忍軍の長
    (1260, "Emma, the Gentle Blade", [9315], [1276]),       # NpcParam "Emma the Physician"
    (1280, "Isshin Ashina", [1299], [1296]),
    (1340, "Great Shinobi Owl", [1359], [1356]),            # Ashina Castle, Talk_忍軍の長
    (1380, "Blackhat Badger", [1399], [1395, 1396]),
    (1420, "Jinzaemon Kumano", [1439], [1435, 1436]),       # Talk_魅了された侍, "My name is Jinzaemon Kumano."
    (1440, "Kotaro", [1459], []),                           # Talk_さらわれ下男, Anayama: "Take a look at Kotaro here."
    (1460, "Doujun", [1479], [1475, 1476]),                 # Talk_不死実験の医僧, "I am Doujun."
    (1580, "Basket Wearer Shosuke", [1599], [1596]),
    (1660, "Tengu of Ashina", [], []),                      # Talk_スキル爺(剣聖), "I am the Tengu of Ashina!"
    (1920, "Fujioka the Info Broker", [1939], [1935, 1936]),
]

# Questline stage of every stage flag the scripts set in the blocks above. Triggers are in
# docs/sekiro-npc-stages.md; where the trigger is only a talk flag, the text quotes the line.
NPC_STAGES = {
    1000: "Dilapidated Temple",
    1001: "After the Divine Dragon",
    1020: "Ashina Castle, his room",
    1021: "Prologue: escaping the castle with you",
    1022: "His room, Genichiro defeated",
    1023: "After the Divine Dragon",
    1025: "After the Great Shinobi Owl",
    1026: "After the Mortal Blade, Lotus, Shelter Stone",
    1027: "Prologue: taken by Genichiro",
    1040: "Senpou Temple, inner sanctum",
    1041: "Senpou Temple, after \"This is for you.\"",
    1042: "Has the Holy Chapter: Dragon's Return",
    1043: "After \"Yes, I've found them.\"",
    1044: "Writhing in pain beyond the door",
    1060: "Ashina Outskirts",
    1061: "Ashina Outskirts, after \"I will... of course.\"",
    1062: "Ashina Outskirts, later",
    1120: "Ashina Outskirts",
    1121: "Ashina Outskirts, after Lady Butterfly",
    1122: "Ashina Outskirts, later",
    1200: "Ashina Outskirts",
    1201: "With Kotaro, after the Divine Dragon",
    1202: "Gone, killed in the Hirata Estate",
    1214: "Gone, after the Divine Dragon",
    1240: "Hirata Estate",
    1241: "Hirata Estate, after \"Father...\"",
    1242: "After Lady Butterfly and the Owl",
    1260: "Dilapidated Temple, later",
    1261: "Ashina Castle",
    1262: "Dilapidated Temple",
    1263: "After Isshin asked about the Mortal Blade",
    1265: "Ashina Castle, after \"Purification?\"",
    1268: "Ashina Castle, after Purification",
    1270: "Shura: after Isshin's death",
    1272: "After the Divine Dragon",
    1280: "Ashina Castle watchtower",
    1281: "Ashina Castle watchtower, you know where he is",
    1282: "After the Divine Dragon",
    1290: "Shura: fought you",
    1340: "Ashina Castle",
    1341: "After the Mortal Blade, Lotus, Shelter Stone",
    1342: "Fought you",
    1354: "Defeated",
    1380: "Ashina Castle",
    1381: "Senpou Temple",
    1382: "Senpou Temple, at the cliff",
    1383: "After the Divine Dragon",
    1384: "Gone",
    1420: "Ashina Reservoir",
    1421: "Mibu Village, following the melody",
    1422: "Mibu Village, later",
    1424: "Abandoned Dungeon, sent to Doujun",
    1425: "Changed by Doujun's treatment",
    1426: "Abandoned Dungeon, following the melody",
    1427: "Ashina Reservoir (prologue)",
    1440: "Senpou Temple, Halls of Illusion",
    1441: "Ashina Outskirts, with Anayama",
    1442: "Killed at Senpou Temple",
    1443: "Abandoned Dungeon, sent to Doujun",
    1444: "Changed by Doujun's treatment",
    1445: "With Anayama, after the Divine Dragon",
    1446: "Killed at Senpou Temple (white flower)",
    1447: "Gone with Anayama",
    1460: "Abandoned Dungeon",
    1461: "Abandoned Dungeon, a patient arrived",
    1462: "After \"It's about time I embark...\"",
    1463: "After \"H-How is this, Master Dousaku?\"",
    1464: "Treated the patient",
    1465: "After \"Unh... Aahhh...\"",
    1466: "After \"Answer me, Dousaku...\"",
    1580: "Mibu Village",
    1581: "Mibu Village, talked",
    1660: "Ashina Castle",
    1661: "After Isshin asked about the Mortal Blade",
    1674: "After the Mortal Blade, Lotus, Shelter Stone",
    1920: "Ashina Castle",
    1921: "Dilapidated Temple",
}

# Progress steps under an NPC: (text, flags), done when any flag is set. The block keeps
# only the current stage, so a questline step lists its own stage and every stage the
# scripts can only reach after it; boss, ending and talk flags stay set.
NPC_STEPS = {
    1020: [
        ("Genichiro defeated at the castle keep", [1022, 1026, 1025, 1023]),
        ("Mortal Blade, Lotus, Shelter Stone", [1026, 1025, 1023]),
        ("Great Shinobi Owl defeated", [1025, 1023]),
        ("Divine Dragon defeated", [1023]),
        ("Ending: Immortal Severance", [6830]),
        ("Ending: Purification", [6831]),
        ("Ending: Return", [6832]),
        ("Ending: Shura", [6833]),
    ],
    1040: [
        ("Holy Chapter: Dragon's Return", [1042, 1043, 1044]),
        ("Told her \"Yes, I've found them.\"", [1043, 1044]),
        ("In pain beyond the door", [1044]),
    ],
    1120: [
        ("Lady Butterfly defeated", [1121]),
    ],
    1200: [
        ("Kotaro sent to him", [71100283]),
    ],
    1240: [
        ("Owl (Father) defeated", [9317]),
    ],
    1260: [
        ("Moved to Ashina Castle", [1261, 1263, 1265, 1268, 1270, 1272]),
        ("Told you about Purification", [71110165]),
        ("Defeated (Shura)", [9315]),
    ],
    1280: [
        ("You know where he recuperates", [1281, 1282]),
        ("Isshin, the Sword Saint defeated", [9312]),
        ("Isshin Ashina defeated (Shura)", [9316]),
    ],
    1340: [
        ("Great Shinobi Owl defeated", [9308]),
    ],
    1380: [
        ("Moved on to Senpou Temple", [1381, 1382, 1383]),
        ("Waiting to cross the cliff", [1382]),
        ("Met again after the Divine Dragon", [1383]),
    ],
    1420: [
        ("Followed the melody", [1426, 1421, 1422]),
        ("Reached Mibu Village", [1421, 1422]),
        ("Sent to Doujun", [1424, 1425]),
        ("Changed by Doujun's treatment", [1425]),
    ],
    1440: [
        ("Sent to Anayama", [1441, 1445, 1447]),
        ("Sent to Doujun", [1443, 1444]),
        ("Changed by Doujun's treatment", [1444]),
    ],
    1460: [
        ("A patient you sent arrived", [1461, 1462, 1463, 1464, 1465, 1466]),
        ("Treated the patient", [1464, 1465, 1466]),
    ],
    1580: [
        ("Drank it all", [1596]),
    ],
    1920: [
        ("Moved to the Dilapidated Temple", [1921]),
    ],
}
MAX_NPC_FLAGS = 2
MAX_STEP_FLAGS = 6

DRAGONROT_CURE_EVENTS = range(9050, 9070)    # common.emevd, one per NPC that can sicken
DRAGONROT_FLAG_MIN, DRAGONROT_FLAG_MAX = 70002000, 70003000
DRAGONROT_GROUP = 5
ROT_ESSENCE_PREFIX = "Rot Essence:"

TALK_SET_EVENT_FLAG = (1, 11)       # SetEventFlag(flag, state)
TALK_CALL_MACHINE_BANK = 6          # call to a state machine of the same script


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


def event_set_flags(archives):
    """Flags the map scripts and common.emevd set on (Set [Network-Connected] Event Flag)."""
    common_func = parse_emevd(read_file(archives, "/event/common_func.emevd.dcx"))
    scripts = [f"/event/{m}_00_00.emevd.dcx" for m in MAPS] + ["/event/common.emevd.dcx"]
    out = set()
    for path in scripts:
        for instrs in event_instances(parse_emevd(read_file(archives, path)), common_func):
            for bank, index, args in instrs:
                if bank == 2003 and index in (2, 62) and len(args) >= 5 and args[4] == 1:
                    out.add(struct.unpack_from("<i", args, 0)[0])
    return out


def esd_constant(b):
    """Value of an ESD expression that is a single int constant, else None."""
    if len(b) == 2 and b[0] < 0x80 and b[1] == 0xA1:
        return b[0] - 64
    if len(b) == 6 and b[0] == 0x82 and b[5] == 0xA1:
        return struct.unpack_from("<i", b, 1)[0]
    return None


def esd_commands(d):
    """(bank, id, [constant args or None]) of every entry and exit command of a talk script
    (fsSL, 64-bit: state machines at +0x48 of the data block that starts at 0x6C)."""
    assert d[:4] == b"fsSL", "expected 64-bit ESD"
    base = 0x6C
    group_count = struct.unpack_from("<i", d, 0x28)[0]

    def commands(offset, count):
        for k in range(count):
            bank, command, args_offset, args_count = struct.unpack_from("<iiqq", d, base + offset + k * 0x18)
            args = []
            for a in range(args_count):
                start, length = struct.unpack_from("<qq", d, base + args_offset + a * 0x10)
                args.append(esd_constant(d[base + start:base + start + length]))
            yield bank, command, args

    for group in range(group_count):
        _group_id, states_offset, state_count, _ = struct.unpack_from("<4q", d, base + 0x48 + group * 0x20)
        for s in range(state_count):
            (_state_id, _co, _cc, entry_offset, entry_count, exit_offset, exit_count) = \
                struct.unpack_from("<7q", d, base + states_offset + s * 0x48)
            yield from commands(entry_offset, entry_count)
            yield from commands(exit_offset, exit_count)


def talk_set_flags(archives):
    """Flags the talk scripts set on: SetEventFlag(flag, 1), or passed to one of the script's
    own talk state machines (which take the line and the flag it sets, e.g. x26(23100300,
    71110150, 0, 1))."""
    out = set()
    for m in MAPS:
        path = f"/script/talk/{m}_00_00.talkesdbnd.dcx"
        if read_file(archives, path) is None:
            continue
        for name, esd in read_bnd(archives, path).items():
            if not re.search(r"t\d+\.esd$", name):
                continue
            for bank, command, args in esd_commands(esd):
                if (bank, command) == TALK_SET_EVENT_FLAG and len(args) == 2 and args[1] == 1 and args[0] is not None:
                    out.add(args[0])
                elif bank == TALK_CALL_MACHINE_BANK:
                    out.update(a for a in args if a is not None and a >= NPC_FLAG_MIN)
    return out


def dragonrot_groups(archives, goods_names):
    """{NPC block base: first Dragonrot flag} from the cure events in common.emevd: each
    clears one group of 5 flags, checks one "Rot Essence" key item and the blocks of the NPC
    (and its other self, e.g. Anayama in the Hirata Estate)."""
    common = parse_emevd(read_file(archives, "/event/common.emevd.dcx"))
    out = {}
    for event_id in DRAGONROT_CURE_EVENTS:
        instrs = common.get(event_id)
        if instrs is None:
            continue
        groups = {struct.unpack_from("<i", a, 0)[0] for b, i, a, _p in instrs
                  if (b, i) == (2003, 22) and a[8] == 0
                  and struct.unpack_from("<i", a, 4)[0] - struct.unpack_from("<i", a, 0)[0] == DRAGONROT_GROUP - 1}
        groups = {g for g in groups if DRAGONROT_FLAG_MIN <= g < DRAGONROT_FLAG_MAX}
        items = {struct.unpack_from("<i", a, 4)[0] for b, i, a, _p in instrs if (b, i) == (3, 4)}
        blocks = {struct.unpack_from("<i", a, 4)[0] // 20 * 20 for b, i, a, _p in instrs
                  if b in (3, 1003) and len(a) >= 8 and NPC_FLAG_MIN <= struct.unpack_from("<i", a, 4)[0] < NPC_FLAG_MAX}
        if len(groups) != 1 or len(items) != 1 or not goods_names.get(next(iter(items)), "").startswith(ROT_ESSENCE_PREFIX):
            raise RuntimeError(f"common event {event_id} is not a Dragonrot cure event")
        for base in blocks:
            if out.setdefault(base, next(iter(groups))) != next(iter(groups)):
                raise RuntimeError(f"NPC block {base}: two Dragonrot groups")
    return out


def npc_tables(archives):
    """(npcs, stages, steps) checked against the scripts."""
    event_set = event_set_flags(archives)
    script_set = event_set | talk_set_flags(archives)
    bases = [base for base, _name, _dead, _hostile in NPCS]
    for base, name, dead, hostile in NPCS:
        for flag in dead + hostile:
            if flag not in script_set:
                raise RuntimeError(f"{name}: no script sets status flag {flag}")
        if len(dead) > MAX_NPC_FLAGS or len(hostile) > MAX_NPC_FLAGS:
            raise RuntimeError(f"{name}: too many status flags")
    # Stages are set by the map scripts (the talk script numbers are not all flags).
    stage_set = {f for f in event_set if any(b <= f < b + NPC_STAGE_COUNT for b in bases)}
    missing = sorted(stage_set - set(NPC_STAGES))
    if missing:
        raise RuntimeError(f"stage flags without a text: {missing}")
    unset = sorted(set(NPC_STAGES) - stage_set)
    if unset:
        raise RuntimeError(f"stage texts for flags no script sets: {unset}")
    steps = []
    for base, name, _dead, _hostile in NPCS:
        for text, flags in NPC_STEPS.get(base, []):
            unknown = [f for f in flags if f not in script_set]
            if unknown:
                raise RuntimeError(f"{name} step {text!r}: no script sets {unknown}")
            if len(flags) > MAX_STEP_FLAGS:
                raise RuntimeError(f"{name} step {text!r}: too many flags")
            steps.append((base, flags, text))
    if set(NPC_STEPS) - set(bases):
        raise RuntimeError(f"steps for unknown NPC blocks: {sorted(set(NPC_STEPS) - set(bases))}")
    return NPCS, sorted(NPC_STAGES.items()), steps


def cpp_npc_rows(npc_table, rot_groups):
    out = []
    for base, name, dead, hostile in npc_table:
        out.append((base, name, dead, hostile, rot_groups.get(base, 0)))
    return out


def prosthetic_tools(weapon_names):
    out = []
    for weapon_id in PROSTHETIC_TOOL_IDS:
        name = " ".join(weapon_names.get(weapon_id, "").split())
        if not name:
            raise RuntimeError(f"no name for prosthetic tool {weapon_id}")
        out.append((weapon_id, name))
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
    npc_table, stage_table, step_table = npc_tables(archives)
    npc_rows = cpp_npc_rows(npc_table, dragonrot_groups(archives, sekirodata.parse_fmg2(item["アイテム名.fmg"])))
    tool_table = prosthetic_tools(sekirodata.parse_fmg2(item["武器名.fmg"]))

    out_dir = os.path.join(repo, "src", "parse", "Sekiro")
    write_crlf(os.path.join(out_dir, "WorldProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/sekiro_flags/gen_flags.py from the game's BonfireWarpParam,
// GameAreaParam, event and talk scripts and menu text; do not edit.
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

    // An NPC's block of 20 event flags (docs/sekiro-npc-stages.md): firstFlag .. + 14 are
    // questline stages (one set at a time). Dead or gone when a dead flag is set, else
    // hostile when a hostile flag is set; 0 when unused.
    struct Npc {{
        uint32_t firstFlag;
        std::string_view name;
        std::array<uint32_t, {MAX_NPC_FLAGS}> deadFlags;
        std::array<uint32_t, {MAX_NPC_FLAGS}> hostileFlags;
        // First of the 5 Dragonrot flags (sick while any is set), 0 when it can't sicken.
        uint32_t dragonrotFlag;
    }};

    struct NpcStage {{
        uint32_t flagId;
        std::string_view text;
    }};

    // A progress step of an NPC: done when any of flags is set. Unused entries are 0.
    struct NpcStep {{
        uint32_t npcFlag;               // firstFlag of the NPCS entry
        std::array<uint32_t, {MAX_STEP_FLAGS}> flags;
        std::string_view text;
    }};

    // A prosthetic tool: found when the inventory holds weapon weaponId .. weaponId + 999
    // (the tool and its upgrades).
    struct ProstheticTool {{
        uint32_t weaponId;
        std::string_view name;
    }};

    // Areas of the idol travel menu, in menu order.
    extern const std::array<std::string_view, {len(area_names)}> AREAS;
    // Sculptor's Idols in travel menu order.
    extern const std::array<Idol, {len(idol_table)}> IDOLS;
    // Bosses by area.
    extern const std::array<Boss, {len(boss_table)}> BOSSES;
    // NPCs with a flag block, by flag.
    extern const std::array<Npc, {len(npc_table)}> NPCS;
    // Questline stage of the flags the scripts set in the NPC blocks, by flag.
    extern const std::array<NpcStage, {len(stage_table)}> NPC_STAGES;
    // Progress steps of the NPCs, in NPCS order, then in the order they happen.
    extern const std::array<NpcStep, {len(step_table)}> NPC_STEPS;
    // The ten prosthetic tools, in menu order.
    extern const std::array<ProstheticTool, {len(tool_table)}> PROSTHETIC_TOOLS;
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

    def padded(flags, size):
        return ", ".join(str(f) for f in flags + [0] * (size - len(flags)))

    lines += ["    };", "", f"    const std::array<Npc, {len(npc_table)}> NPCS = {{"]
    for base, name, dead, hostile, rot in npc_rows:
        lines.append(f"        Npc{{{base}, {cpp_string(name)}, {{{padded(dead, MAX_NPC_FLAGS)}}}, "
                     f"{{{padded(hostile, MAX_NPC_FLAGS)}}}, {rot}}},")
    lines += ["    };", "", f"    const std::array<NpcStage, {len(stage_table)}> NPC_STAGES = {{"]
    for flag, text in stage_table:
        lines.append(f"        NpcStage{{{flag}, {cpp_string(text)}}},")
    lines += ["    };", "", f"    const std::array<NpcStep, {len(step_table)}> NPC_STEPS = {{"]
    for base, flags, text in step_table:
        lines.append(f"        NpcStep{{{base}, {{{padded(flags, MAX_STEP_FLAGS)}}}, {cpp_string(text)}}},")
    lines += ["    };", "", f"    const std::array<ProstheticTool, {len(tool_table)}> PROSTHETIC_TOOLS = {{"]
    for weapon_id, name in tool_table:
        lines.append(f"        ProstheticTool{{{weapon_id}, {cpp_string(name)}}},")
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{sum(1 for row in npc_rows if row[4])} NPCs can get Dragonrot")
    print(f"{len(idol_table)} idols, {len(boss_table)} bosses, {len(npc_table)} NPCs, "
          f"{len(stage_table)} stages, {len(step_table)} steps, {len(tool_table)} prosthetic tools")


if __name__ == "__main__":
    main()
