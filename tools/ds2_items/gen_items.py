"""Generate the DS2 item database and inventory icons from the installed game data.

Usage:
    python gen_items.py "<...>/Dark Souls II Scholar of the First Sin/Game" <repo root> [--no-icons]

Writes:
    src/parse/DS2/Items.cpp                       item database
    src/resources/images/ds2/item_icons/<id>.png  item icons (128x256, game framing)
    src/resources/ds2_inv_images.qrc              Qt resource list for the icons
    src/resources/images/ds2/menu_icons/inv_*.png inventory category icons (64x64)
    src/resources/images/ds2/ui/ui_*.png          inventory and character list pieces (windows, tiles, pedestal)
    src/parse/DS2/Areas.cpp                       map id -> area name

Item names come from /menu/text/english/itemname.fmg, item kind and stack size from
ItemParam in enc_regulation.bnd.dcx, icons from the "*_IC_<icon id>" texture packs in
GameDataEbl. Rows without an English name are internal placeholders and are skipped.
With --no-icons the existing icon files are kept and referenced.

Requires pycryptodome and Pillow.
"""
import io
import os
import struct
import sys

import ds2data

ICON_SIZE = (128, 256)
ICON_DIR = os.path.join("src", "resources", "images", "ds2", "item_icons")

# Top byte of ItemParam field 19 (ITEM_PARAM_ST +0x4c).
KIND_WEAPON = 0
KIND_SHIELD = 1
KIND_HEAD = 2
KIND_CHEST = 3
KIND_HANDS = 4
KIND_LEGS = 5
KIND_AMMO = 6
KIND_RING = 7
KIND_GOODS = 8
KIND_SPELL = 9
KIND_GESTURE = 10


def category(item_id, kind):
    if kind == KIND_WEAPON:
        if 3_800_000 <= item_id < 4_200_000 or 5_400_000 <= item_id < 5_500_000:
            return "Catalysts"      # staves, chimes, pyromancy flames
        if 4_200_000 <= item_id < 4_700_000:
            return "RangedWeapons"  # bows, greatbows, crossbows
        return "Weapons"
    if kind == KIND_GOODS:
        if 50_000_000 <= item_id < 54_000_000:
            return "KeyItems"
        if 60_970_000 <= item_id < 62_000_000:
            return "Materials"      # titanite, infusion stones
        return "Consumables"
    return {
        KIND_SHIELD: "Shields",
        KIND_HEAD: "HeadArmor",
        KIND_CHEST: "ChestArmor",
        KIND_HANDS: "HandsArmor",
        KIND_LEGS: "LegsArmor",
        KIND_AMMO: "Ammunition",
        KIND_RING: "Rings",
        KIND_SPELL: "Spells",
        KIND_GESTURE: "Gestures",
    }.get(kind, "Unknown")


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def pick_icon(icons, icon_id):
    """Prefer the high quality texture; both variants share the same framing."""
    for prefix in ("HQ_", ""):
        for kind in ("WP", "SD", "HD", "BD", "AM", "LG", "AS", "IT", "MA", "GE", "SI"):
            dds = icons.get(f"{prefix}{kind}_IC_{icon_id:010d}")
            if dds is not None:
                return dds
    return None


def write_icons(repo_root, icons, icon_ids):
    from PIL import Image

    out_dir = os.path.join(repo_root, ICON_DIR)
    os.makedirs(out_dir, exist_ok=True)
    written = set()
    for icon_id in sorted(icon_ids):
        dds = pick_icon(icons, icon_id)
        if dds is None:
            continue
        image = Image.open(io.BytesIO(dds))
        image.load()
        image = image.convert("RGBA")
        if image.size[0] * 2 == image.size[1] and image.size != ICON_SIZE:
            image = image.resize(ICON_SIZE, Image.LANCZOS)
        image.save(os.path.join(out_dir, f"{icon_id}.png"), optimize=True)
        written.add(icon_id)
    return written


# Inventory category icons ("IC_CA_<n>" textures) used for each ItemCategory.
CATEGORY_ICONS = {
    "weapons": "IC_CA_00010",   # crossed axe and halberd, as in the in-game inventory tabs
    "catalysts": "IC_CA_00140",
    "ranged_weapons": "IC_CA_00130",
    "shields": "IC_CA_00150",
    "head": "IC_CA_00400",
    "chest": "IC_CA_00410",
    "hands": "IC_CA_00420",
    "legs": "IC_CA_00430",
    "rings": "IC_CA_00500",
    "ammunition": "IC_CA_00600",
    "consumables": "IC_CA_00700",
    "key_items": "IC_CA_00710",
    "materials": "IC_CA_00720",
    "spells": "IC_CA_00800",
    "gestures": "IC_CA_02000",
}


def write_category_icons(repo_root, archive):
    from PIL import Image

    wanted = set(CATEGORY_ICONS.values())
    textures = {}
    for h, (size, offset, aes_offset) in archive.entries.items():
        if size > 1_000_000 or len(textures) == len(wanted):
            continue
        if not aes_offset:
            with open(archive.bdt_path, "rb") as f:
                f.seek(offset)
                if f.read(4) not in (b"TPF\0", b"DCX\0"):
                    continue
        try:
            d = archive.read_hash(h)
            if d[:4] != b"TPF\0":
                continue
            for name, dds in ds2data.tpf_textures(d):
                if name in wanted:
                    textures[name] = dds
        except Exception:
            continue
    out_dir = os.path.join(repo_root, "src", "resources", "images", "ds2", "menu_icons")
    os.makedirs(out_dir, exist_ok=True)
    for alias, texture in CATEGORY_ICONS.items():
        image = Image.open(io.BytesIO(textures[texture]))
        image.load()
        image.convert("RGBA").save(os.path.join(out_dir, f"inv_{alias}.png"), optimize=True)
    print(f"wrote {len(CATEGORY_ICONS)} category icons")


# Pieces of the in-game inventory screen: (menu bundle, atlas texture, rect in atlas).
UI_SPRITES = {
    "ui_window": ("/menu/40.febnd.dcx", "In-game_01", (15, 6, 895, 549)),      # frame + title bar
    "ui_category_tile": ("/menu/40.febnd.dcx", "In-game_01", (469, 643, 517, 691)),
    "ui_category_selected": ("/menu/40.febnd.dcx", "In-game_01", (941, 374, 1022, 494)),
    "ui_divider": ("/menu/40.febnd.dcx", "In-game_01", (563, 640, 1012, 677)),
    "ui_menu_tabs": ("/menu/40.febnd.dcx", "In-game_01", (7, 783, 294, 843)),
    "ui_item_tile": ("/menu/99.febnd.dcx", "IconBase", (3, 11, 78, 94)),
    "ui_item_selected": ("/menu/99.febnd.dcx", "IconBase", (100, 5, 181, 93)),
    "ui_item_border": ("/menu/99.febnd.dcx", "IconBase", (205, 4, 253, 71)),
    "ui_item_shadow": ("/menu/99.febnd.dcx", "IconBase", (2, 160, 79, 254)),
    "ui_pedestal": ("/menu/99.febnd.dcx", "IconBase", (80, 224, 150, 254)),
    # Character select ("Data List") window and its selected row band.
    "ui_data_list": ("/menu/17.febnd.dcx", "Data_list", (0, 0, 910, 640)),
    "ui_row_selected": ("/menu/17.febnd.dcx", "Data_list", (7, 657, 776, 731)),
}
# Sprites drawn on opaque black in their atlas; the outer black is made transparent.
CLEAR_OUTER_BLACK = {"ui_data_list"}


def write_ui_sprites(repo_root, archive):
    from PIL import Image

    out_dir = os.path.join(repo_root, "src", "resources", "images", "ds2", "ui")
    os.makedirs(out_dir, exist_ok=True)
    atlases = {}
    for alias, (bundle, texture, rect) in UI_SPRITES.items():
        key = (bundle, texture)
        if key not in atlases:
            for _, data in ds2data.parse_bnd4(archive.read(bundle)).items():
                data = ds2data.undcx(data)
                if data[:4] != b"TPF\0":
                    continue
                for name, dds in ds2data.tpf_textures(data):
                    if name == texture:
                        image = Image.open(io.BytesIO(dds))
                        image.load()
                        atlases[key] = image.convert("RGBA")
        sprite = atlases[key].crop(rect)
        if alias in CLEAR_OUTER_BLACK:
            from PIL import ImageDraw
            w, h = sprite.size
            for corner in ((0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)):
                ImageDraw.floodfill(sprite, corner, (0, 0, 0, 0), thresh=24)
        sprite.save(os.path.join(out_dir, f"{alias}.png"), optimize=True)
    print(f"wrote {len(UI_SPRITES)} ui sprites")


def write_areas(repo_root, archive):
    names = ds2data.parse_fmg(archive.read("/menu/text/english/mapname.fmg"))
    lines = [
        "// Generated by tools/ds2_items/gen_items.py from /menu/text/english/mapname.fmg.",
        "// Do not edit by hand.",
        '#include "Areas.h"',
        "",
        "namespace fssm::parse::ds2 {",
        f"    const std::array<Area, {len(names)}> ALL_AREAS = {{",
    ]
    lines += [f"        Area{{{map_id}, {cpp_string(name)}}}," for map_id, name in sorted(names.items())]
    lines += ["    };", "}", ""]
    with open(os.path.join(repo_root, "src", "parse", "DS2", "Areas.cpp"), "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {len(names)} area names")


def existing_icons(repo_root):
    out_dir = os.path.join(repo_root, ICON_DIR)
    if not os.path.isdir(out_dir):
        return set()
    return {int(f[:-4]) for f in os.listdir(out_dir) if f.endswith(".png") and f[:-4].isdigit()}


def write_qrc(repo_root, icon_ids):
    lines = ["<RCC>", '    <qresource prefix="/ds2_inv_images">']
    lines += [f'        <file alias="{i}">images/ds2/item_icons/{i}.png</file>' for i in sorted(icon_ids)]
    lines += ["    </qresource>", "</RCC>", ""]
    with open(os.path.join(repo_root, "src", "resources", "ds2_inv_images.qrc"), "w", newline="\r\n") as f:
        f.write("\n".join(lines))


def main(game_dir, repo_root, with_icons=True):
    archive = ds2data.Archive(game_dir)
    names = ds2data.parse_fmg(archive.read("/menu/text/english/itemname.fmg"))
    params = ds2data.parse_bnd4(ds2data.read_regulation(game_dir))
    item_rows = ds2data.parse_param(params["ItemParam.param"])

    items = []
    for item_id, row in sorted(item_rows.items()):
        name = names.get(item_id)
        if not name or item_id < 1000:   # "No Item" / "No Spell" placeholders
            continue
        fields = struct.unpack("<21i", row)
        icon_id = fields[0]                 # ItemParam +0x00
        kind = (fields[19] >> 24) & 0xFF
        max_held = (fields[18] >> 16) & 0xFFFF
        order = fields[16] & 0xFFFF
        items.append((item_id, max_held, order, category(item_id, kind), name, icon_id))

    if with_icons:
        icons = write_icons(repo_root, ds2data.icon_textures(archive), {i[5] for i in items})
        print(f"wrote {len(icons)} icons")
        write_category_icons(repo_root, archive)
        write_ui_sprites(repo_root, archive)
    else:
        icons = existing_icons(repo_root)
    write_qrc(repo_root, icons)
    write_areas(repo_root, archive)

    lines = [
        "// Generated by tools/ds2_items/gen_items.py from the game's ItemParam,",
        "// /menu/text/english/itemname.fmg and item icon textures. Do not edit by hand.",
        '#include "Items.h"',
        "",
        "namespace fssm::parse::ds2 {",
        f"    const std::array<BaseItem, {len(items)}> ALL_ITEMS = {{",
    ]
    for item_id, max_held, order, cat, name, icon_id in items:
        image = str(icon_id) if icon_id in icons else ""
        lines.append(f"        BaseItem{{{item_id}, {max_held}, {order}, ItemCategory::{cat}, "
                     f"{cpp_string(name)}, {cpp_string(image)}}},")
    lines += ["    };", "}", ""]
    out_path = os.path.join(repo_root, "src", "parse", "DS2", "Items.cpp")
    with open(out_path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {len(items)} items to {out_path}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], "--no-icons" not in sys.argv[3:])
