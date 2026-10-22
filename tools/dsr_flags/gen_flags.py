"""Generate the Dark Souls Remastered bonfire, boss and NPC tables from the installed game data.

Usage:
    python gen_flags.py "<...>/DARK SOULS REMASTERED" <repo root>

Writes:
    src/parse/DSR/WorldProgress.h      table sizes
    src/parse/DSR/WorldProgress.cpp    bonfires, bosses, NPCs and area names

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

NPCs: every NPC owns a block of global event flags in 1000-1999. The map scripts instantiate
per-NPC events (InitializeEvent parameters: entity, block range, flag); their developer names
in the .emeld next to each script tell death events from hostile ones (npc_blocks). Names are
in NPCS below.
"""
import collections
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
EMEVD_INITIALIZE_EVENT = (2000, 0)
EMEVD_SET_EVENT_FLAG = (2003, 2)
EMEVD_BATCH_SET_EVENT_FLAGS = (2003, 22)

NPC_FLAGS = range(1000, 2000)
# PlaceName ids of areas (2000+ are the Lordvessel warp list's bonfire names).
AREA_PLACE_IDS = range(1000, 2000)
COMMON_CONSTRUCTOR = 50

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

# NPC flag block (first flag) -> name. The game has names for few NPCs (NPC_name_.fmg is
# mostly bosses), so each block was identified from its events' developer names (.emeld)
# and the dialogue of the NPC the events act on (MSB enemy part -> talk id -> the
# Conversation_.fmg lines its talk script uses, e.g. "I'm Andre, of Astora.").
NPCS = {
    1000: "Solaire of Astora",
    1030: "Darkmoon Knightess",
    1060: "Oscar, Knight of Astora",
    1090: "Big Hat Logan",
    1110: "Griggs of Vinheim",
    1120: "Dusk of Oolacile",
    1170: "Rhea of Thorolund",
    1190: "Petrus of Thorolund",
    1210: "Vince of Thorolund",
    1220: "Nico of Thorolund",
    1230: "Gwynevere",
    1250: "Laurentius of the Great Swamp",
    1270: "Fair Lady",
    1280: "Eingyi",
    1290: "Quelana of Izalith",
    1310: "Ingward",
    1320: "Andre of Astora",
    1340: "Vamos",
    1360: "Giant Blacksmith",
    1380: "Rickert of Vinheim",
    1400: "Undead Merchant (Male)",
    1410: "Undead Merchant (Female)",
    1420: "Crestfallen Merchant",
    1430: "Domhnall of Zena",
    1460: "Crestfallen Warrior",
    1490: "Siegmeyer of Catarina",
    1540: "Sieglinde of Catarina",
    1570: "Knight Lautrec of Carim",
    1600: "Shiva of the East",
    1620: "Patches",
    1640: "Kingseeker Frampt",
    1670: "Darkstalker Kaathe",
    1700: "Oswald of Carim",
    1760: "Shiva's Bodyguard",
    1820: "Hawkeye Gough",
    1840: "Marvellous Chester",
    1860: "Lord's Blade Ciaran",
    1870: "Elizabeth",
}
# Blocks left out: 1240 Gwyndolin and 1690 Priscilla are bosses (BOSSES); 1140 Anastacia's
# events all run when she dies and pick a state the talk script reads back (she can be
# revived), none of them is named a death.
NPC_BLOCKS_LEFT_OUT = {1240, 1690}
NPC_SLOTS = 3

# Questline stage of each flag the scripts set in an NPC block (one is set at a time): the
# developers' event names translated, see docs/dsr-npc-stages.md. Every stage flag needs a
# text; dead flags may have one when the death is a particular one.
NPC_STAGES = {
    # Solaire of Astora
    1000: "Not met yet",
    1001: "Undead Burg: met, offers his help",
    1007: "Sunlight Altar: offers his covenant",
    1008: "Anor Londo: met again",
    1009: "Lost Izalith: looking for his sun",
    1002: "Lost Izalith: found his sun",
    1003: "Lost Izalith: did not find his sun",
    1012: "Ready to fight alongside you",
    1011: "Died after finding his sun",
    # Darkmoon Knightess
    1030: "Not met yet",
    1031: "Anor Londo: wants to talk",
    1036: "Anor Londo: wants to talk (after the Lordvessel)",
    # Oscar, Knight of Astora
    1060: "Undead Asylum",
    1061: "Undead Asylum: hollowed",
    1062: "Killed as a Hollow",
    # Big Hat Logan
    1090: "Not met yet",
    1091: "Sen's Fortress: freed, thanked you",
    1092: "Firelink Shrine: teaches sorceries",
    1093: "Duke's Archives: locked in a cell",
    1094: "Duke's Archives: freed, thanked you",
    1095: "Duke's Archives: teaches sorceries",
    # Griggs of Vinheim
    1110: "Not met yet",
    1111: "Undead Burg: freed, thanked you",
    1112: "Firelink Shrine: teaches sorceries",
    1113: "Firelink Shrine: teaches advanced sorceries",
    # Dusk of Oolacile
    1120: "Not met yet",
    1121: "Darkroot Basin: freed, thanked you",
    1122: "Summon sign available",
    1123: "Teaches sorceries",
    1126: "Gone",
    1127: "Taken to Oolacile, needs help",
    1128: "Oolacile: thanked you",
    1129: "Back from Oolacile, summon sign available",
    1130: "Back from Oolacile, teaches sorceries",
    # Rhea of Thorolund, Petrus, Vince, Nico
    1170: "Not met yet",
    1171: "Firelink Shrine: arrived",
    1173: "Left for the Catacombs",
    1174: "Tomb of the Giants: rescued, thanked you",
    1175: "Undead Parish: teaches miracles",
    1180: "Killed without resisting",
    1202: "Not met yet",
    1192: "Left with Rhea",
    1193: "Firelink Shrine: came back alone",
    1194: "Firelink Shrine: showed his true nature",
    1196: "Killed after betraying Rhea",
    1210: "Not met yet",
    1211: "Firelink Shrine: arrived",
    1216: "Left with Rhea",
    1220: "Not met yet",
    1221: "Firelink Shrine: arrived",
    1226: "Left with Rhea",
    # Gwynevere, Fair Lady
    1230: "Anor Londo",
    1270: "Quelaag's Domain",
    # Laurentius of the Great Swamp
    1250: "Not met yet",
    1251: "Depths: freed, thanked you",
    1252: "Firelink Shrine: teaches pyromancy",
    1256: "Went to Blighttown after Chaos",
    # Eingyi
    1280: "Not met yet",
    1281: "Turns you away",
    1286: "Friendly, you are an egg-bearer",
    # Quelana of Izalith
    1290: "Not met yet",
    1291: "Teaches pyromancy",
    1292: "Left",
    1293: "Thanked you for the Bed of Chaos",
    1296: "Gone",
    # Ingward
    1310: "Not met yet",
    1311: "New Londo: will give you the key",
    1312: "New Londo: asks you to defeat the Darkwraiths",
    1313: "Heals curses",
    # Merchants and smiths
    1320: "Undead Parish",
    1340: "The Catacombs",
    1360: "Anor Londo",
    1380: "New Londo Ruins",
    1400: "Undead Burg",
    1410: "Lower Undead Burg",
    1420: "Sen's Fortress",
    1700: "Undead Parish",
    # Domhnall of Zena
    1430: "Depths",
    1431: "Firelink Shrine: sells his wares",
    # Crestfallen Warrior
    1460: "Firelink Shrine",
    1464: "Hollowed in New Londo",
    # Siegmeyer of Catarina
    1490: "Not met yet",
    1491: "Sen's Fortress: waiting on the balcony",
    1492: "Sen's Fortress: waiting on the balcony",
    1493: "Left Sen's Fortress",
    1494: "Anor Londo: thanked you",
    1497: "Firelink Shrine: resting",
    1501: "Blighttown: asleep, needs help",
    1502: "Blighttown: thanked you",
    1503: "Lost Izalith: wants to help",
    1504: "Lost Izalith: charging the Chaos Bugs",
    1505: "Lost Izalith: you killed the bugs",
    1506: "Lost Izalith: thanked you, near death",
    1507: "Lost Izalith: thanked you, survived",
    1511: "Hollowed, killed at Ash Lake",
    1514: "Gone",
    # Sieglinde of Catarina
    1540: "Not met yet",
    1541: "Duke's Archives: locked in a cell",
    1542: "Duke's Archives: freed, thanked you",
    1543: "Can't find her father",
    1544: "Firelink Shrine: looking for her father",
    1545: "Found her father",
    1546: "Killed her hollowed father",
    1549: "Gone",
    # Knight Lautrec of Carim
    1570: "Undead Parish: locked in a cell",
    1571: "Freed, thanked you",
    1572: "Firelink Shrine: looking for his target",
    1573: "Gone after the Fire Keeper",
    1577: "Not freed, gone",
    # Shiva of the East and his bodyguard
    1600: "Not met yet",
    1601: "Blighttown: sells his wares",
    1606: "Painted World: invading",
    1607: "Painted World: invading",
    1608: "Killed in the Painted World",
    1609: "Killed in the Painted World",
    1760: "Not met yet",
    1766: "Darkroot Garden",
    # Patches
    1620: "Not met yet",
    1621: "Catacombs: kicked you down",
    1623: "Tomb of the Giants: waiting to kick you down",
    1624: "Tomb of the Giants: kicked you down",
    1626: "Firelink Shrine: sells his wares",
    # Kingseeker Frampt, Darkstalker Kaathe
    1640: "Asleep",
    1641: "About to wake",
    1642: "Gives you the first task",
    1643: "Wants the Lordvessel placed",
    1644: "Gives you the second task",
    1645: "Pleased, task done",
    1647: "Gone",
    1649: "Lamenting your choice",
    1670: "Not met yet",
    1671: "Gives you the first task",
    1672: "Wants the Lordvessel placed",
    1673: "Gives you the second task",
    1674: "Pleased, task done",
    1677: "Gone",
    1678: "Lamenting your choice",
    # Oolacile
    1820: "Not met yet",
    1821: "Thanked you",
    1840: "Not met yet",
    1860: "Not met yet",
    1861: "At Artorias' grave",
    1862: "Thanked you",
    1865: "Gone",
    1870: "Not met yet",
}

# Progress steps shown under an NPC (docs/dsr-npc-stages.md, "Progress steps"): a step is
# done when any of its flags is set. The block keeps only the current stage, so a questline
# step lists its own stage and every stage the scripts can only reach after it; talk script
# flags (set by SetEventState in the NPC's talk script, kept after the block is cleared)
# are added where a stage event waits for them. ("give", item) is the flag the NPC's talk
# script sets when you give it that item (its "Give <item>?" prompt). Every flag must be
# set by a map script in an NPC block or by a talk script.
NPC_STEPS = {
    1000: [     # Solaire of Astora
        ("Met in the Undead Burg", [11010591, 1001, 1008, 1007, 1009, 1002, 1003, 1012, 1011]),
        ("Met again in Anor Londo", [1008, 1007, 1009, 1002, 1003, 1012, 1011]),
        ("Offered his covenant at the Sunlight Altar", [1007, 1009, 1002, 1003, 1012, 1011]),
        ("Came looking for his sun in Lost Izalith", [1009, 1002, 1003, 1012, 1011]),
        ("Found the Sunlight Maggot", [1002, 1011]),
        ("Ready to fight alongside you", [1012]),
    ],
    1090: [     # Big Hat Logan
        ("Freed in Sen's Fortress", [1091, 1092, 1093, 1094, 1095]),
        ("Firelink Shrine: taught sorceries", [1092, 1093, 1094, 1095]),
        ("Freed again in the Duke's Archives", [1094, 1095]),
        ("Duke's Archives: teaches sorceries", [1095]),
    ],
    1110: [     # Griggs of Vinheim
        ("Freed in the Undead Burg", [1111, 1112, 1113]),
        ("Firelink Shrine: teaches sorceries", [1112, 1113]),
        ("Teaches advanced sorceries", [1113]),
    ],
    1120: [     # Dusk of Oolacile
        ("Freed from the Golden Crystal Golem", [1121, 1122, 1123, 1126, 1127, 1128, 1129, 1130]),
        ("Taken to Oolacile", [1127, 1128, 1129, 1130]),
        ("Rescued in Oolacile", [1128, 1129, 1130]),
    ],
    1170: [     # Rhea of Thorolund
        ("Arrived at Firelink Shrine", [1171, 1173, 1174, 1175]),
        ("Left for the Catacombs", [1173, 1174, 1175]),
        ("Rescued in the Tomb of the Giants", [1174, 1175]),
        ("Undead Parish: teaches miracles", [1175]),
        ("Hollowed in the Duke's Archives", [1181]),
    ],
    1190: [     # Petrus of Thorolund
        ("Left with Rhea", [1192, 1193, 1194, 1196]),
        ("Came back alone", [1193, 1194, 1196]),
        ("Showed his true nature", [1194, 1196]),
        ("Killed after betraying Rhea", [1196]),
    ],
    1250: [     # Laurentius of the Great Swamp
        ("Freed in the Depths", [1251, 1252, 1256, 1257]),
        ("Firelink Shrine: teaches pyromancy", [1252, 1256, 1257]),
        ("Went to Blighttown after Chaos", [1256, 1257]),
    ],
    1280: [     # Eingyi
        ("Met in Quelaag's Domain", [1281, 1286]),
        ("Friendly, you are an egg-bearer", [1286]),
    ],
    1290: [     # Quelana of Izalith
        ("Teaches pyromancy", [1291, 1292, 1293]),
        ("Thanked you for the Bed of Chaos", [1292, 1293]),
    ],
    1310: [     # Ingward
        ("Asked you to defeat the Darkwraiths", [1312, 1313]),
        ("Firelink Shrine: heals curses", [1313]),
    ],
    1320: [     # Andre of Astora
        ("give", 800), ("give", 801), ("give", 808), ("give", 809), ("give", 810),
    ],
    1340: [     # Vamos
        ("give", 812), ("give", 813),
    ],
    1360: [     # Giant Blacksmith
        ("give", 802),
    ],
    1380: [     # Rickert of Vinheim
        ("give", 806), ("give", 807),
    ],
    1430: [     # Domhnall of Zena
        ("Moved to Firelink Shrine", [1431]),
    ],
    1460: [     # Crestfallen Warrior
        ("Went hollow in New Londo", [1464]),
    ],
    1490: [     # Siegmeyer of Catarina
        ("Anor Londo: helped him", [1494, 1497, 1501, 1502, 1503, 1504, 1505, 1506, 1507, 1511, 1514]),
        ("Firelink Shrine: rested", [1497, 1501, 1502, 1503, 1504, 1505, 1506, 1507, 1511, 1514]),
        ("Blighttown: woke him up", [1502, 1503, 1504, 1505, 1506, 1507, 1511, 1514]),
        ("Lost Izalith: the Chaos Bugs are dead", [1505, 1506, 1507, 1511, 1514]),
        ("Hollowed, killed at Ash Lake", [1511, 1546, 1549]),
    ],
    1540: [     # Sieglinde of Catarina
        ("Freed in the Duke's Archives", [1542, 1543, 1544, 1545, 1546, 1549]),
        ("Firelink Shrine: looking for her father", [1544, 1545, 1546, 1549]),
        ("Found her father", [1545, 1546, 1549]),
        ("Ended her father's suffering at Ash Lake", [1546, 1549]),
    ],
    1570: [     # Knight Lautrec of Carim
        ("Freed in the Undead Parish", [1571, 1572, 1573, 1578]),
        ("Went after the Fire Keeper", [1573, 1578]),
    ],
    1620: [     # Patches
        ("Tomb of the Giants: kicked you down", [1624, 1626]),
        ("Firelink Shrine: sells his wares", [1626]),
    ],
    1640: [     # Kingseeker Frampt
        ("Gave you the first task", [1642, 1643, 1644, 1645, 1649, 1647]),
        ("Wants the Lordvessel placed", [1643, 1644, 1645]),
        ("Gave you the second task", [1644, 1645]),
        ("Pleased, task done", [1645]),
        ("You followed Kaathe instead", [1649, 1647]),
    ],
    1670: [     # Darkstalker Kaathe
        ("Gave you the first task", [1671, 1672, 1673, 1674, 1678, 1677]),
        ("Gave you the second task", [1673, 1674]),
        ("Pleased, task done", [1674]),
        ("You followed Frampt instead", [1678]),
    ],
    1820: [     # Hawkeye Gough
        ("Freed, thanked you", [1821]),
    ],
    1860: [     # Lord's Blade Ciaran
        ("At Artorias' grave", [1861, 1862, 1865]),
        ("Thanked you", [1862]),
    ],
}

TALK_SET_EVENT_STATE = (1, 11)
TALK_SHOW_YES_NO = (1, 17)          # (window, Event_text id, ...)
TALK_DEBUG_NAME = (1, 0)

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


def parse_emevd_params(d):
    """{event id: ([(bank, index, raw args)], [(instruction, target byte, source byte, length)])}
    of a Dark Souls EMEVD, keeping the parameter entries InitializeEvent fills in."""
    assert d[:4] == b"EVD\0", "expected EMEVD"
    header = struct.unpack_from("<16I", d, 0x10)
    event_count, events_offset, instr_offset = header[0], header[1], header[3]
    param_offset, args_offset = header[9], header[13]
    events = {}
    for e in range(event_count):
        event_id, count, offset, param_count, param_off = struct.unpack_from("<IIIII", d, events_offset + e * 0x1C)
        instructions = []
        for i in range(count):
            bank, index, args_length, args_off = struct.unpack_from("<IIIi", d, instr_offset + offset + i * 0x18)
            instructions.append((bank, index, d[args_offset + args_off:args_offset + args_off + args_length]))
        params = [struct.unpack_from("<IIII", d, param_offset + param_off + p * 0x14) for p in range(param_count)]
        events[event_id] = (instructions, params)
    return events


def instantiate(event, init_args):
    """The event's instructions as int32 args with InitializeEvent's parameter bytes applied."""
    instructions, params = event
    out = []
    for n, (bank, index, raw) in enumerate(instructions):
        raw = bytearray(raw)
        for instruction, target, source, length in params:
            if instruction == n:
                raw[target:target + length] = init_args[source:source + length]
        out.append((bank, index, struct.unpack_from(f"<{len(raw) // 4}i", raw)))
    return out


def parse_emeld(d):
    """{event id: developer's event name} of a Dark Souls EMELD."""
    assert d[:4] == b"ELD\0", "expected EMELD"
    count, offset = struct.unpack_from("<II", d, 0x10)
    strings_offset = struct.unpack_from("<I", d, 0x2C)[0]
    out = {}
    for i in range(count):
        event_id, name_offset = struct.unpack_from("<II", d, offset + i * 12)
        start = end = strings_offset + name_offset
        while d[end:end + 2] != b"\0\0":
            end += 2
        out[event_id] = d[start:end].decode("utf-16le")
    return out


def esd_value(b):
    """Value of an ESD expression that is a single constant (int or UTF-16 string), else None."""
    if len(b) == 2 and b[0] < 0x80 and b[1] == 0xA1:
        return b[0] - 64
    if len(b) == 6 and b[0] == 0x82 and b[5] == 0xA1:
        return struct.unpack_from("<i", b, 1)[0]
    if b[:1] == b"\xA5" and b[-3:] == b"\0\0\xA1":
        return b[1:-3].decode("utf-16le")
    return None


def parse_esd(d):
    """{state id: (entry commands, [next state ids])} of every state of a DSR talk script
    (fsSL, 64-bit layout); commands are (bank, id, [constant args or None])."""
    assert d[:4] == b"fsSL" and struct.unpack_from("<i", d, 0x1C)[0] == 0x48, "expected 64-bit fsSL ESD"
    group_count = struct.unpack_from("<i", d, 0x28)[0]
    base = 0x6C

    def q(*offsets):
        return struct.unpack_from(f"<{len(offsets)}q", d, base + offsets[0])

    def commands(offset, count):
        out = []
        for k in range(count):
            bank, command, args_offset, args_count = struct.unpack_from("<iiqq", d, base + offset + k * 0x18)
            args = []
            for a in range(args_count):
                start, length = struct.unpack_from("<qq", d, base + args_offset + a * 0x10)
                args.append(esd_value(d[base + start:base + start + length]))
            out.append((bank, command, args))
        return out

    states = {}
    for group in range(group_count):
        _group_id, states_offset, state_count, _ = struct.unpack_from("<4q", d, base + 0x48 + group * 0x20)
        for s in range(state_count):
            state_id, conditions_offset, condition_count, entry_offset, entry_count = \
                struct.unpack_from("<5q", d, base + states_offset + s * 0x48)
            following = []
            for c in range(condition_count):
                condition = struct.unpack_from("<q", d, base + conditions_offset + c * 8)[0]
                target = struct.unpack_from("<q", d, base + condition)[0]
                if target >= 0:
                    following.append(struct.unpack_from("<q", d, base + target)[0])
            states[state_id] = (commands(entry_offset, entry_count), following)
    return states


def talk_flags(game_dir):
    """(flags the talk scripts set on, {item id: flag}) where the flag is set by the state the
    "Give <item>?" prompt (Event_text `Give <?gdsparam@ID?>?`) moves to when you accept: its
    debug name is アイテムを渡すN (give item N), the refusal is アイテムを渡さないN."""
    event_text = parse_fmg1(parse_bnd3(read(game_dir, "msg/ENGLISH/menu.msgbnd.dcx"))["Event_text_.fmg"])
    set_on, give = set(), {}
    for path in sorted(glob.glob(os.path.join(game_dir, "script", "talk", "*.talkesdbnd.dcx"))):
        for d in parse_bnd3(read(game_dir, path)).values():
            states = parse_esd(d)
            for entry, _following in states.values():
                set_on |= {args[0] for (bank, command, args) in entry
                           if (bank, command) == TALK_SET_EVENT_STATE and args[1] == 1 and isinstance(args[0], int)}
            for entry, following in states.values():
                for bank, command, args in entry:
                    m = re.fullmatch(r"Give <\?gdsparam@(\d+)\?>\?", event_text.get(args[1], "") if
                                     (bank, command) == TALK_SHOW_YES_NO and isinstance(args[1], int) else "")
                    if not m:
                        continue
                    for target in following:
                        target_entry = states.get(target, ([], []))[0]
                        names = [a[0] for b, c, a in target_entry if (b, c) == TALK_DEBUG_NAME]
                        flags = [a[0] for b, c, a in target_entry if (b, c) == TALK_SET_EVENT_STATE and a[1] == 1]
                        if names and re.fullmatch(r"アイテムを渡す\d", names[0]) and len(flags) == 1:
                            if give.setdefault(int(m.group(1)), flags[0]) != flags[0]:
                                raise RuntimeError(f"item {m.group(1)}: two give flags")
    return set_on, give


def npc_blocks(game_dir):
    """{first flag: (last flag, dead flags, hostile flags, entities, stage flags)} of the NPC
    flag blocks; stage flags are the other flags of the block the scripts set.

    Every instance of an event (InitializeEvent with its parameters) is read with the event's
    developer name from the matching .emeld: a 死亡 (death) event clears the block with Batch
    Set Event Flags and sets the dead flag; 敵対 (hostile) events set a hostile flag, except
    敵対でいなくなる (leaves after turning hostile). The common absolution event (免罪, Oswald)
    clears the hostile flags; those no other named event sets are hostile flags too (set by the
    talk scripts)."""
    scripts = {}
    for path in sorted(glob.glob(os.path.join(game_dir, "event", "*.emevd.dcx"))):
        name = os.path.basename(path).split(".")[0]
        if name != "common" and not re.fullmatch(r"m1\d_\d\d_00_00", name):
            continue
        with open(path, "rb") as f:
            events = parse_emevd_params(undcx(f.read()))
        with open(path.replace(".emevd.", ".emeld."), "rb") as f:
            names = parse_emeld(undcx(f.read()))
        scripts[name] = (events, names)
    common = scripts["common"][0]

    instances = []      # (event name, instructions)
    for events, names in scripts.values():
        for instructions, _params in events.values():
            for bank, index, raw in instructions:
                if (bank, index) != EMEVD_INITIALIZE_EVENT:
                    continue
                target = struct.unpack_from("<I", raw, 4)[0]
                if target in events:
                    instances.append((names.get(target, ""), instantiate(events[target], raw[8:])))
                elif target in common:
                    instances.append((scripts["common"][1].get(target, ""), instantiate(common[target], raw[8:])))

    ranges = {}
    for _name, instructions in instances:
        for bank, index, args in instructions:
            if (bank, index) == EMEVD_BATCH_SET_EVENT_FLAGS and args[0] in NPC_FLAGS and args[2] == 0:
                ranges[args[0]] = max(ranges.get(args[0], args[1]), args[1])

    def block_of(flag):
        starts = [s for s, e in ranges.items() if s <= flag <= e]
        return max(starts) if starts else None

    dead, hostile, stage, named, absolved = (collections.defaultdict(set) for _ in range(5))
    entities = collections.defaultdict(set)
    for name, instructions in instances:
        sets = [args[0] for bank, index, args in instructions
                if (bank, index) == EMEVD_SET_EVENT_FLAG and args[0] in NPC_FLAGS and args[1] == 1]
        if "免罪" in name:
            for bank, index, args in instructions:
                if (bank, index) == EMEVD_SET_EVENT_FLAG and args[0] in NPC_FLAGS and args[1] == 0:
                    absolved[block_of(args[0])].add(args[0])
            continue
        chars = {args[1] for bank, index, args in instructions if bank == 4 and index in (0, 2) and len(args) > 1}
        for flag in sets:
            block = block_of(flag)
            if block is None:
                continue
            named[block].add(flag)
            if "死亡" in name:
                dead[block].add(flag)
                entities[block] |= chars
            elif "敵対" in name and "いなくなる" not in name:
                hostile[block].add(flag)
                entities[block] |= chars
            elif name:
                stage[block].add(flag)
    for block, flags in absolved.items():
        hostile[block] |= {f for f in flags if f not in named[block] or f in hostile[block]}
    # A flag other named events set as well is a questline stage the hostile event also
    # moves to (1766: Shiva and his bodyguard are in Darkroot Garden).
    for block in hostile:
        hostile[block] -= stage[block]
    # Starting stages: common.emevd event 50 (run by the game itself) sets them.
    for bank, index, args in instantiate(common[COMMON_CONSTRUCTOR], b""):
        if (bank, index) == EMEVD_SET_EVENT_FLAG and args[0] in NPC_FLAGS and args[1] == 1:
            block = block_of(args[0])
            if block is not None:
                named[block].add(args[0])
    return {start: (ranges[start], sorted(dead[start]), sorted(hostile[start]), entities[start],
                    sorted(named[start] - hostile[start] - dead[start]))
            for start in sorted(ranges) if dead[start] or hostile[start]}


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

    place_table = [(place, area_name(place)) for place in sorted(place_names)
                   if place in AREA_PLACE_IDS and area_name(place)]

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

    blocks = npc_blocks(game_dir)
    for start in NPC_BLOCKS_LEFT_OUT:
        if not blocks.pop(start)[3] & set(BOSSES):
            raise RuntimeError(f"NPC block {start} is no longer a boss")
    if set(blocks) != set(NPCS):
        raise RuntimeError(f"NPC blocks changed: unnamed {sorted(set(blocks) - set(NPCS))}, "
                           f"gone {sorted(set(NPCS) - set(blocks))}")
    npc_table = []
    for start, name in NPCS.items():
        end, dead, hostile, _entities, stages = blocks[start]
        if len(dead) > NPC_SLOTS or len(hostile) > NPC_SLOTS:
            raise RuntimeError(f"NPC block {start}: more than {NPC_SLOTS} dead or hostile flags")
        missing = [f for f in stages if f not in NPC_STAGES]
        if missing:
            raise RuntimeError(f"NPC block {start} ({name}): no stage text for {missing}")
        npc_table.append((start, end, dead, hostile, name))
    known = {f for start in NPCS for f in blocks[start][1] + blocks[start][4]}
    if set(NPC_STAGES) - known:
        raise RuntimeError(f"stage texts for flags no script sets: {sorted(set(NPC_STAGES) - known)}")
    stage_table = sorted(NPC_STAGES.items())

    goods_names = parse_fmg1(item_msg["Item_name_.fmg"])
    talk_set, give_flags = talk_flags(game_dir)
    block_set = {f for b in blocks.values() for f in b[1] + b[2] + b[4]}
    step_table = []
    for start in NPCS:
        for text, flags in NPC_STEPS.get(start, []):
            if text == "give":
                if flags not in give_flags:
                    raise RuntimeError(f"NPC block {start}: no talk script takes item {flags}")
                text, flags = f"Given the {goods_names[flags]}", [give_flags[flags]]
            unknown = [f for f in flags if f not in block_set and f not in talk_set]
            if unknown:
                raise RuntimeError(f"NPC block {start} step {text!r}: no script sets {unknown}")
            step_table.append((start, flags, text))
    if set(NPC_STEPS) - set(NPCS):
        raise RuntimeError(f"steps for unknown NPC blocks {sorted(set(NPC_STEPS) - set(NPCS))}")
    step_slots = max(len(flags) for _start, flags, _text in step_table)

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

    // An NPC's event flag block. Some flag in it is set once the NPC's map has loaded; its
    // death events clear the block and set one of deadFlags, its hostile events set one of
    // hostileFlags (cleared again by absolution). Unused entries are 0.
    struct Npc {{
        uint32_t firstFlag;
        uint32_t lastFlag;
        std::array<uint32_t, {NPC_SLOTS}> deadFlags;
        std::array<uint32_t, {NPC_SLOTS}> hostileFlags;
        std::string_view name;
    }};

    // A questline stage: the NPC block's one set flag (docs/dsr-npc-stages.md).
    struct NpcStage {{
        uint32_t flag;
        std::string_view text;
    }};

    // Area name (PlaceName text) the Data List screen shows for a profile's areaId.
    // A progress step of an NPC (docs/dsr-npc-stages.md): done when any of flags is set.
    // Unused entries are 0.
    struct NpcStep {{
        uint32_t npcFlag;               // firstFlag of the NPCS entry
        std::array<uint32_t, {step_slots}> flags;
        std::string_view text;
    }};

    struct PlaceName {{
        uint32_t id;
        std::string_view name;
    }};

    // Areas in the order they are usually reached.
    extern const std::array<std::string_view, {len(AREAS)}> AREAS;
    // Bonfires by area.
    extern const std::array<Bonfire, {len(bonfire_table)}> BONFIRES;
    // Bosses in the order they are usually met.
    extern const std::array<Boss, {len(boss_table)}> BOSSES;
    // NPCs with an event flag block, by flag.
    extern const std::array<Npc, {len(npc_table)}> NPCS;
    // Questline stage of the flags the scripts set in the NPC blocks, by flag.
    extern const std::array<NpcStage, {len(stage_table)}> NPC_STAGES;
    // Questline steps and events of the NPCs, in NPCS order, then in the order they happen.
    extern const std::array<NpcStep, {len(step_table)}> NPC_STEPS;
    // Area PlaceNames (ids 1000-1999), by id.
    extern const std::array<PlaceName, {len(place_table)}> PLACE_NAMES;
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
    lines += ["    };", "", f"    const std::array<Npc, {len(npc_table)}> NPCS = {{"]

    def flag_array(flags):
        return "{" + ", ".join(str(f) for f in flags + [0] * (NPC_SLOTS - len(flags))) + "}"

    for start, end, dead, hostile, name in npc_table:
        lines.append(f"        Npc{{{start}, {end}, {flag_array(dead)}, {flag_array(hostile)}, {cpp_string(name)}}},")
    lines += ["    };", "", f"    const std::array<NpcStage, {len(stage_table)}> NPC_STAGES = {{"]
    for flag, text in stage_table:
        lines.append(f"        NpcStage{{{flag}, {cpp_string(text)}}},")
    lines += ["    };", "", f"    const std::array<NpcStep, {len(step_table)}> NPC_STEPS = {{"]
    for start, flags, text in step_table:
        padded = ", ".join(str(f) for f in flags + [0] * (step_slots - len(flags)))
        lines.append(f"        NpcStep{{{start}, {{{padded}}}, {cpp_string(text)}}},")
    lines += ["    };", "", f"    const std::array<PlaceName, {len(place_table)}> PLACE_NAMES = {{"]
    for place, name in place_table:
        lines.append(f"        PlaceName{{{place}, {cpp_string(name)}}},")
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(bonfire_table)} bonfires ({len(warp)} named by the warp list), {len(boss_table)} bosses, "
          f"{len(npc_table)} NPCs")


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_crlf(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


if __name__ == "__main__":
    main()
