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
doesn't name them; NPC_STAGES below describes each from what its event waits for.

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
NPC_STAGE_COUNT = 15            # questline stages base .. base + 14 before the status flags

# Questline stage of each flag the event scripts set in an NPC block (exactly one of the
# 15 is set at a time). The game data has no names for them (DS3 ships no .emeld): each
# text was written from what the setting event waits for (a boss or NPC defeated, a line
# of dialogue whose talk flag it reads, an item, another NPC's stage), see
# docs/ds3-npc-stages.md.
NPC_STAGES = {
    # Fire Keeper
    1000: "Firelink Shrine",
    1001: "Lords of Cinder on their thrones",
    1002: "Lords of Cinder on their thrones, later",
    # Ludleth of Courland
    1020: "Firelink Shrine",
    1021: "After the Kiln warp",
    # Yuria of Londor
    1040: "Not met yet",
    1041: "Firelink Shrine: came after Yoel's death",
    1042: "Firelink Shrine: met",
    1043: "Firelink Shrine: later",
    1044: "Left",
    1045: "Kiln: after the Soul of Cinder",
    # Yoel of Londor
    1060: "Not met yet",
    1061: "Firelink Shrine: serving you",
    1062: "Drew out all your strength",
    1063: "Not recruited, gone",
    1064: "Died at Firelink Shrine",
    # Company Captain Yorshka
    1080: "Anor Londo",
    # Hawkwood the Deserter
    1100: "Firelink Shrine",
    1101: "Firelink Shrine: after the Greatwood or Crystal Sage",
    1102: "Firelink Shrine: after the Abyss Watchers",
    1103: "Left Firelink Shrine",
    1104: "Farron Keep: waiting for a duel",
    # Sirris of the Sunless Realms
    1120: "Not met yet",
    1121: "Firelink Shrine",
    1122: "Firelink Shrine: met",
    1123: "Firelink Shrine: talked again",
    1124: "Irithyll: asks for help against Creighton",
    1125: "Creighton defeated",
    1126: "Thanked you for the rescue",
    1127: "Holy Knight Hodrick defeated",
    1128: "Thanked you again",
    1129: "After the Twin Princes",
    1130: "Undead Settlement: her last duel",
    1133: "You joined Rosaria's Fingers",
    1134: "Turned against you, a Finger of Rosaria",
    # Ringfinger Leonhard
    1140: "Firelink Shrine",
    1141: "Firelink Shrine: after the High Wall",
    1142: "Firelink Shrine: gave you red eye orbs",
    1143: "Firelink Shrine: you have a Pale Tongue",
    1144: "Firelink Shrine: talked about invading",
    1145: "Firelink Shrine: later",
    1146: "Cathedral of the Deep: you joined Rosaria's Fingers",
    1151: "Cathedral of the Deep: asked about Horace",
    1152: "Took Rosaria's soul",
    1153: "Defeated in Anor Londo",
    # Blacksmith Andre, Shrine Handmaid
    1160: "Firelink Shrine",
    1180: "Firelink Shrine",
    1600: "Firelink Shrine",
    # Greirat of the Undead Settlement
    1200: "High Wall: in his cell",
    1201: "Freed",
    1202: "Firelink Shrine: sells his wares",
    1203: "Out looting the Undead Settlement",
    1204: "Back from looting",
    1205: "Out looting Irithyll alone",
    1206: "Back from Irithyll",
    1207: "Out looting Lothric Castle",
    # Orbeck of Vinheim
    1220: "Not met yet",
    1221: "Road of Sacrifices: met",
    1222: "Firelink Shrine: teaches sorceries",
    1223: "Firelink Shrine: you brought him scrolls",
    1224: "After the Twin Princes",
    # Cornyx, Karla
    1240: "Undead Settlement: in a cage",
    1241: "Firelink Shrine: freed",
    1260: "Irithyll Dungeon: in a cell",
    1261: "Firelink Shrine: freed",
    # Irina of Carim
    1280: "Undead Settlement: in a cell",
    1281: "Firelink Shrine: freed",
    1282: "Firelink Shrine: given a dark tome",
    1283: "Firelink Shrine: given every dark tome",
    1284: "Firelink Shrine: gone dark",
    1285: "Firelink Shrine: gone dark",
    1286: "Firelink Shrine: given no dark tome",
    # Eygon of Carim
    1300: "Undead Settlement",
    1301: "Undead Settlement: met",
    1302: "Firelink Shrine: after the Abyss Watchers",
    1303: "Firelink Shrine: talked",
    1304: "Firelink Shrine: Irina gone dark",
    1305: "After the Dragonslayer Armour",
    # Anri of Astora
    1340: "Not met yet",
    1341: "Road of Sacrifices: met",
    1342: "Firelink Shrine: after the Deacons",
    1344: "Catacombs of Carthus",
    1345: "Catacombs of Carthus: met again",
    1346: "After High Lord Wolnir, Horace hollowed",
    1347: "After High Lord Wolnir",
    1348: "Irithyll: met again",
    1349: "Irithyll: wedding",
    1350: "Irithyll: the Old Woman is dead",
    1351: "After Aldrich",
    1352: "After Aldrich: left",
    1353: "After Aldrich: left (Smouldering Lake)",
    # Unbreakable Patches
    1360: "Not met yet",
    1361: "Cathedral of the Deep: met",
    1362: "Cathedral of the Deep: trapped you",
    1363: "Cathedral of the Deep: trap avoided",
    1364: "Firelink Shrine: arrived",
    1365: "Firelink Shrine: sells his wares",
    1366: "Firelink Shrine: Greirat went to Irithyll",
    1367: "Firelink Shrine: Greirat went to Lothric",
    1370: "Greirat is dead",
    # Siegward of Catarina
    1380: "Not met yet",
    1382: "Undead Settlement: on the tower",
    1383: "Cathedral of the Deep: met again",
    1384: "Firelink Shrine",
    1385: "Cathedral of the Deep: lost his armour",
    1386: "Irithyll: dinner by the fire",
    1388: "Irithyll Dungeon: freed",
    1389: "Fought Yhorm with you",
    1390: "Yhorm defeated",
    1394: "Undead Settlement: Fire Demon defeated",
    # High Priestess Emma
    1420: "High Wall",
    1421: "After the Dancer",
    # Horace the Hushed
    1480: "Not met yet",
    1481: "Road of Sacrifices: met",
    1482: "Firelink Shrine: after the Deacons",
    1484: "Catacombs of Carthus",
    1485: "Smouldering Lake: hollowed",
    # Rosaria, Mother of Rebirth
    1500: "Cathedral of the Deep",
    1501: "Soul taken by Leonhard",
    1502: "Restored",
    # Old Woman of Londor
    1560: "Firelink Shrine",
    1561: "After High Lord Wolnir",
    1562: "Talked",
    1563: "Anri in Irithyll",
    1564: "Told you of the Lord of Hollows",
    1565: "Wedding with Anri",
    1566: "Anri in Irithyll",
    1567: "Anri is dead",
    # Holy Knight Hodrick
    1580: "Undead Settlement",
    # Slave Knight Gael
    1620: "Cathedral of the Deep",
    1621: "Cathedral of the Deep: met",
    1622: "Cathedral of the Deep: talked",
    1623: "Later in the game",
    1624: "Later in the game, talked",
    1625: "The Painted World",
    # Sir Vilhelm
    1640: "Painted World of Ariandel",
    1641: "Painted World of Ariandel: met",
    # Amnesiac Lapp
    1800: "Not met yet",
    1801: "Dreg Heap: met",
    1802: "Dreg Heap: remembered",
    1803: "Dreg Heap: summon sign",
    1804: "Dreg Heap: summon sign",
    1805: "After the Demon Prince",
    1806: "After the Demon Prince",
    1807: "Feast with you",
    1808: "Off to the Purging Monument",
    1809: "Asked for help",
    1810: "Thanked you",
    1811: "Purging Monument",
    # Shira, Knight of Filianore
    1820: "Ringed City",
    1821: "Ringed City: gave you a token",
    1822: "After Halflight",
    1823: "Ringed City: later",
    # Stone-humped Hag
    1860: "Ringed City",
    1861: "Ringed City: dead",
    1862: "Ringed City: later",
}


def stage_range(first, last, *more):
    """The questline stages first .. last (the ones the scripts set) and more."""
    return [flag for flag in range(first, last + 1) if flag in NPC_STAGES] + list(more)


# Progress steps of an NPC, by its status flag, in the order they usually happen: quest
# milestones and things handed over or received. A step is done when any of its flags is
# set. Every flag comes from the game data (main() checks where):
# - questline stages (NPC_STAGES). Only the current stage is set, so a "reached" step
#   lists its stage and the ones after it;
# - ItemLotParam +0x80, the flag set when the lot is given or picked up ("Received");
# - ShopLineupParam +0x10, the flag that releases a shop row ("Given"): set when an item
#   is handed to a teacher or the Shrine Handmaid. The NPC's talk script lists the items
#   it takes and the shop pages they open in the same order (t400210 Shrine Handmaid,
#   t400230 Orbeck, t400240 Cornyx, t400250 Karla, t400260 Irina);
# - flags set by the map scripts: 8200 / 8220 / 8240 are set by common event 20005061
#   when the invader Creighton / Hodrick / Leonhard dies.
# See docs/ds3-npc-stages.md.
NPC_STEPS = {
    1055: [  # Yuria of Londor
        ("Met at Firelink Shrine", stage_range(1042, 1045)),
        ("Received the Morion Blade", [50006040]),
        ("Left Firelink Shrine", [1044]),
        ("Kiln: after the Soul of Cinder", [1045]),
        ("Received Darkdrift", [50006041]),
        ("Received Yuria's attire", [50006042]),
    ],
    1075: [  # Yoel of Londor
        ("Serving you at Firelink Shrine", [1061, 1062, 1064]),
        ("Received the Dark Sigil", [50006193]),
        ("Drew out all your strength", [1062]),
    ],
    1115: [  # Hawkwood the Deserter
        ("Received a Heavy Gem", [50006070]),
        ("Left Firelink Shrine", [1103, 1104]),
        ("Received Hawkwood's Shield", [50006074]),
        ("Received the Twinkling Dragon Torso Stone", [50006071]),
        ("Waited for a duel in Farron Keep", [1104]),
        ("Received the Twinkling Dragon Head Stone", [50006072]),
    ],
    1135: [  # Sirris of the Sunless Realms
        ("Met at Firelink Shrine", stage_range(1122, 1130, 1133)),
        ("Creighton the Wanderer defeated", [8200]),
        ("Received the Silvercat Ring", [50006080]),
        ("Holy Knight Hodrick defeated", [8220]),
        ("Received the Sunset Shield", [50006081]),
        ("Received the Sunset armour", [50006083]),
        ("Received the Sunless Talisman", [50006082]),
        ("Her last duel in the Undead Settlement", [1130]),
    ],
    1155: [  # Ringfinger Leonhard
        ("Received a Cracked Red Eye Orb", [50006090]),
        ("Received the Lift Chamber Key", [50006091]),
        ("Met in the Cathedral of the Deep", stage_range(1146, 1153)),
        ("Took Rosaria's soul", [1152, 1153]),
        ("Received the Black Eye Orb", [50006092]),
        ("Defeated in Anor Londo", [8240]),
        ("Received the Crescent Moon Sword", [50006093]),
    ],
    1195: [  # Shrine Handmaid: umbral ashes handed over, in her ash menu's order
        ("Given Mortician's Ashes", [70000100]),
        ("Given Dreamchaser's Ashes", [70000101]),
        ("Given Paladin's Ashes", [70000102]),
        ("Given Grave Warden's Ashes", [70000103]),
        ("Given Greirat's Ashes", [70000110]),
        ("Given Orbeck's Ashes", [70000111]),
        ("Given Cornyx's Ashes", [70000112]),
        ("Given Karla's Ashes", [70000113]),
        ("Given Irina's Ashes", [70000114]),
        ("Given Patches' Ashes", [70000115]),
        ("Given Excrement-covered Ashes", [70000104]),
        ("Given Prisoner Chief's Ashes", [70000105]),
        ("Given Xanthous Ashes", [70000106]),
        ("Given Hollow's Ashes", [70000107]),
        ("Given Dragon Chaser's Ashes", [70000108]),
        ("Given Yuria's Ashes", [70000116]),
        ("Given Easterner's Ashes", [70000109]),
        ("Given Captain's Ashes", [70000120]),
        ("Given Old Woman's Ashes", [70000121]),
    ],
    1215: [  # Greirat of the Undead Settlement
        ("Freed from his cell", stage_range(1201, 1207)),
        ("Received the Blue Tearstone Ring", [50006120]),
        ("Sent to loot the Undead Settlement", stage_range(1203, 1207)),
        ("Back with new wares", [70000151]),
        ("Sent to loot Irithyll", stage_range(1205, 1207)),
        ("Back from Irithyll", [1206, 1207]),
        ("Sent to loot Lothric Castle", [1207]),
        ("Lothric Castle loot", [70000152]),
    ],
    1235: [  # Orbeck of Vinheim
        ("Met on the Road of Sacrifices", stage_range(1221, 1224)),
        ("Given the Sage's Scroll", [70000201]),
        ("Given Logan's Scroll", [70000202]),
        ("Given the Crystal Scroll", [70000203]),
        ("Given the Golden Scroll", [70000204]),
        ("Received the Young Dragon Ring", [50006130]),
        ("Received the Slumbering Dragoncrest Ring", [50006131]),
    ],
    1255: [  # Cornyx of the Great Swamp
        ("Freed", [1241]),
        ("Received a Pyromancy Flame", [50006140]),
        ("Given the Great Swamp Pyromancy Tome", [70000250]),
        ("Given the Carthus Pyromancy Tome", [70000251]),
        ("Given the Izalith Pyromancy Tome", [70000252]),
    ],
    1275: [  # Karla
        ("Freed", [1261]),
        ("Given the Quelana Pyromancy Tome", [70000300]),
        ("Given the Grave Warden Pyromancy Tome", [70000301]),
        ("Given the Deep Braille Divine Tome", [70000302]),
        ("Given the Londor Braille Divine Tome", [70000303]),
    ],
    1295: [  # Irina of Carim
        ("Freed", stage_range(1281, 1286)),
        ("Given the Braille Divine Tome of Carim", [70000350]),
        ("Given the Braille Divine Tome of Lothric", [70000351]),
        ("Given the Deep Braille Divine Tome", [70000352]),
        ("Given the Londor Braille Divine Tome", [70000353]),
        ("Gone dark", [1284, 1285]),
    ],
    1315: [  # Eygon of Carim
        ("Met in the Undead Settlement", stage_range(1301, 1305)),
        ("At Firelink Shrine", stage_range(1302, 1305)),
        ("Received Morne's Great Hammer", [50006170]),
    ],
    1355: [  # Anri of Astora
        ("Met on the Road of Sacrifices", stage_range(1341, 1353)),
        ("Received the Ring of the Evil Eye", [50006190]),
        ("Met in the Catacombs of Carthus", stage_range(1344, 1353)),
        ("High Lord Wolnir defeated", stage_range(1346, 1353)),
        ("Met in Irithyll", stage_range(1348, 1353)),
        ("Received the Sword of Avowal", [50006301]),
        ("Wedding", [1349]),
        ("Received Anri's Straight Sword", [50006030]),
    ],
    1375: [  # Unbreakable Patches
        ("Met in the Cathedral of the Deep", stage_range(1361, 1367, 1370)),
        ("Arrived at Firelink Shrine", stage_range(1364, 1367)),
        ("Received a Rusted Coin", [50006200]),
        ("Received the Horsehoof Ring", [50006202]),
    ],
    1395: [  # Siegward of Catarina
        ("Met on the Undead Settlement tower", stage_range(1382, 1390, 1394)),
        ("Received Siegbräu", [50006210]),
        ("Met in the Cathedral of the Deep", stage_range(1383, 1390)),
        ("Dinner by the fire in Irithyll", stage_range(1386, 1390)),
        ("Freed from the Irithyll Dungeon well", [1388, 1389]),
        ("Received a Titanite Slab", [50006215]),
        ("Received the Catarina armour", [50006217]),
    ],
    1495: [  # Horace the Hushed
        ("Met on the Road of Sacrifices", stage_range(1481, 1485)),
        ("Met in the Catacombs of Carthus", [1484, 1485]),
        ("Hollowed at Smouldering Lake", [1485]),
        ("Received the Llewellyn Shield", [50006260]),
    ],
    1815: [  # Amnesiac Lapp
        ("Met in the Dreg Heap", stage_range(1801, 1811)),
        ("Feast with you", stage_range(1807, 1811)),
        ("Received Siegbräu", [50006620]),
        ("Off to the Purging Monument", stage_range(1808, 1811)),
        ("Received Lapp's armour", [50006624]),
    ],
    1835: [  # Shira, Knight of Filianore
        ("Received the Sacred Chime of Filianore", [50006630]),
        ("Received the Crucifix of the Mad King", [50006632]),
    ],
}

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


def script_set_flags(archives):
    """Flags the map scripts set on (Set Event Flag with its arguments filled in)."""
    common = parse_emevd(read_file(archives, COMMON_FUNC))
    flags = set()
    for area in MAP_AREAS:
        for block in MAP_BLOCKS:
            script = read_file(archives, f"/event/m{area:02d}_{block:02d}_00_00.emevd.dcx")
            if script is None:
                continue
            for instrs in event_instances(parse_emevd(script), common):
                for bank, index, args in instrs:
                    if bank == 2003 and index in (2, 62):       # Set (Network-Connected) Event Flag
                        flag, on = struct.unpack_from("<iB", args, 0)
                        if on == 1:
                            flags.add(flag)
    return flags


def npc_stage_flags(set_flags, bases):
    """Flags of the NPC blocks' stage range (base .. base + 14) the map scripts set."""
    return {flag for flag in set_flags if any(base <= flag < base + NPC_STAGE_COUNT for base in bases)}


def param_flags(params):
    """Flags set by picking up an item lot (ItemLotParam +0x80) and flags that release a
    shop row (ShopLineupParam +0x10)."""
    lots = {struct.unpack_from("<i", row, 0x80)[0]
            for row in sekirodata.parse_param(params["ItemLotParam.param"]).values()}
    shops = {struct.unpack_from("<i", row, 0x10)[0]
             for row in sekirodata.parse_param(params["ShopLineupParam.param"]).values()}
    return lots, shops


def check_npc_steps(npc_table, stages, set_flags, lots, shops):
    """Every step flag must come from the game data (see NPC_STEPS)."""
    status_flags = {flag for flag, _ in npc_table}
    for status, steps in NPC_STEPS.items():
        if status not in status_flags:
            raise RuntimeError(f"NPC_STEPS: no NPC with status flag {status}")
        for text, flags in steps:
            for flag in flags:
                if status - NPC_STAGE_COUNT <= flag < status:
                    ok = flag in stages
                else:
                    ok = flag in lots or flag in shops or flag in set_flags
                if not ok:
                    raise RuntimeError(f"NPC_STEPS: {text!r}: no game data sets {flag}")


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
    set_flags = script_set_flags(archives)
    stages = npc_stage_flags(set_flags, [flag - NPC_STAGE_COUNT for flag, _ in npc_table])
    if stages != set(NPC_STAGES):
        raise RuntimeError(f"NPC stages changed: no text for {sorted(stages - set(NPC_STAGES))}, "
                           f"no script sets {sorted(set(NPC_STAGES) - stages)}")
    stage_table = sorted(NPC_STAGES.items())
    check_npc_steps(npc_table, stages, set_flags, *param_flags(params))
    # Steps in NPCS order, then in the order they are listed.
    step_table = [(status, flags, text) for status, _ in npc_table for text, flags in NPC_STEPS.get(status, [])]
    max_step_flags = max(len(flags) for _, flags, _ in step_table)

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

    // A questline stage: the one set flag of statusFlag - 15 .. statusFlag - 1
    // (docs/ds3-npc-stages.md).
    struct NpcStage {{
        uint32_t flag;
        std::string_view text;
    }};

    // A progress step of an NPC: a quest milestone, or an item given or received. Done
    // when any of its flags is set; 0 when unused (docs/ds3-npc-stages.md).
    struct NpcStep {{
        uint32_t statusFlag;            // the NPC's (Npc::statusFlag)
        std::array<uint32_t, {max_step_flags}> flags;
        std::string_view text;
    }};

    // Areas of the bonfire travel menu, in menu order.
    extern const std::array<std::string_view, {len(AREAS)}> AREAS;
    // Bonfires in travel menu order.
    extern const std::array<Bonfire, {len(table)}> BONFIRES;
    // Bosses in the order they are usually met.
    extern const std::array<Boss, {len(BOSSES)}> BOSSES;
    // NPCs with a status flag block, by flag.
    extern const std::array<Npc, {len(npc_table)}> NPCS;
    // Questline stage of the flags the scripts set in the NPC blocks, by flag.
    extern const std::array<NpcStage, {len(stage_table)}> NPC_STAGES;
    // Progress steps, by NPC in NPCS order, then in the order they usually happen.
    extern const std::array<NpcStep, {len(step_table)}> NPC_STEPS;
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
    lines += ["    };", "", f"    const std::array<NpcStage, {len(stage_table)}> NPC_STAGES = {{"]
    for flag, text in stage_table:
        lines.append(f"        NpcStage{{{flag}, {cpp_string(text)}}},")
    lines += ["    };", "", f"    const std::array<NpcStep, {len(step_table)}> NPC_STEPS = {{"]
    for status, flags, text in step_table:
        ids = ", ".join(str(f) for f in flags + [0] * (max_step_flags - len(flags)))
        lines.append(f"        NpcStep{{{status}, {{{ids}}}, {cpp_string(text)}}},")
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(table)} bonfires, {len(BOSSES)} bosses, {len(npc_table)} NPCs, {len(stage_table)} NPC stages, "
          f"{len(step_table)} NPC steps")


if __name__ == "__main__":
    main()
