"""Generate the Dark Souls III bonfire and boss tables from the installed game data.

Usage:
    python gen_flags.py "<...>/DARK SOULS III/Game" <DecryptionKeys.cs> <repo root>

The DS3 executable is packed, so the archive RSA keys are not in it as PEM text. Pass any
file that contains them as "-----BEGIN RSA PUBLIC KEY-----" blocks, e.g. BinderTool's
BinderTool/DecryptionKeys.cs (github.com/Atvaark/BinderTool).

Writes:
    src/parse/DS3/WorldProgress.h      table sizes
    src/parse/DS3/WorldProgress.cpp    bonfires (BonfireWarpParam) and bosses

Bonfires come from BonfireWarpParam in the regulation (Data0.bdt, AES-256-CBC with the
key below, IV = first 16 bytes): +0x0 event flag set when the bonfire is lit, +0x8 name
(FDP_メニューテキスト[_dlc1|_dlc2].fmg in /msg/engus/menu*.msgbnd.dcx), +0x10 sort id,
+0x14 travel menu area (AREAS; 99 = rows the menu doesn't list). Rows that share a name
are one bonfire (e.g. High Wall of Lothric before and after its first visit), lit when
any of their flags is set.

Boss defeat flags are not in the game params; BOSSES was checked against saves (each
map's 8xx flags of the first thousand, matching the bonfire named after the boss).

Requires pycryptodome.
"""
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
HIDDEN_AREA = 99

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


def menu_text(archives):
    text = {}
    for path in MENU_MSG:
        data = read_file(archives, path)
        if data is None:
            continue
        for name, fmg in sekirodata.parse_bnd4(data).items():
            if os.path.basename(name.replace("\\", "/")).startswith(MENU_FMG_PREFIX):
                text.update(sekirodata.parse_fmg2(fmg))
    if not text:
        raise RuntimeError("no menu text found")
    return text


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
    table = bonfires(regulation_params(game_dir), menu_text(archives))

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

    // Areas of the bonfire travel menu, in menu order.
    extern const std::array<std::string_view, {len(AREAS)}> AREAS;
    // Bonfires in travel menu order.
    extern const std::array<Bonfire, {len(table)}> BONFIRES;
    // Bosses in the order they are usually met.
    extern const std::array<Boss, {len(BOSSES)}> BOSSES;
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
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "WorldProgress.cpp"), "\n".join(lines))
    print(f"{len(table)} bonfires, {len(BOSSES)} bosses")


if __name__ == "__main__":
    main()
