"""Generate the Elden Ring item database and inventory icons from the installed game data.

Usage:
    python gen_items.py "<...>/ELDEN RING/Game" <repo root> [--icon-size N] [--no-icons]

Writes:
    src/parse/EldenRing/Items.cpp                      item database
    src/resources/images/er/item_icons/<id>.png        item icons (default 128x128)
    src/resources/er_inv_images.qrc                    Qt resource list for the icons

Names come from /msg/engus/item*.msgbnd (base game and DLC), params from
regulation.bin, icons from the SB_Icon* atlases of /menu/hi/01_common.

Param fields used (found by correlating with the icon atlases and known items):
    EquipParamGoods      +0x30 iconId, +0x3E goodsType
    EquipParamWeapon     +0xBE iconId, +0x1A6 wepType
    EquipParamProtector  +0xA6 iconIdM
    EquipParamAccessory  +0x26 iconId
    EquipParamGem        +0x18 iconId

Requires pycryptodome, zstandard and Pillow.
"""
import io
import os
import re
import struct
import sys

import erdata

ICON_DIR = os.path.join("src", "resources", "images", "er", "item_icons")

# Item id type nibble as stored in the save (itemId >> 28).
TYPE_WEAPON = 0x0
TYPE_PROTECTOR = 0x1
TYPE_ACCESSORY = 0x2
TYPE_GOODS = 0x4
TYPE_GEM = 0x8

GOODS_TYPES = {
    0: "Tools", 9: "Tools", 11: "Tools",
    1: "KeyItems",
    2: "CraftingMaterials",
    3: "Remembrances",
    5: "Sorceries", 17: "Sorceries",
    7: "SpiritAshes", 8: "SpiritAshes",
    10: "CrystalTears",
    12: "Info",
    14: "UpgradeMaterials",
    15: "GreatRunes",
    16: "Incantations", 18: "Incantations",
}


def weapon_category(wep_type):
    if wep_type in (50, 51, 53, 55, 56):
        return "RangedWeapons"      # bows, greatbows, crossbows, ballistae
    if wep_type in (57, 61):
        return "Catalysts"          # staves, sacred seals
    if wep_type in (65, 67, 69):
        return "Shields"
    if wep_type in (81, 83, 85, 86):
        return "Ammunition"
    return "MeleeWeapons"


def protector_category(protector_id):
    return ("HeadArmor", "ChestArmor", "ArmsArmor", "LegsArmor")[(protector_id % 1000) // 100 % 4]


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def load_names(archives):
    names = {}
    for path in ("/msg/engus/item.msgbnd.dcx", "/msg/engus/item_dlc01.msgbnd.dcx", "/msg/engus/item_dlc02.msgbnd.dcx"):
        if not archives.has(path):
            continue
        for fmg_name, data in archives.bnd(path).items():
            if not fmg_name.endswith("Name.fmg"):
                continue
            table = names.setdefault(fmg_name, {})
            for item_id, text in erdata.parse_fmg2(data).items():
                if text and not text.startswith("[ERROR]"):
                    table[item_id] = text
    return names


def icon_sprites(archives):
    layouts = erdata.parse_bnd4(archives.read("/menu/hi/01_common.sblytbnd.dcx"))
    sprites = {}
    for name, xml in layouts.items():
        if not name.endswith(".layout"):
            continue
        texture = name[:-len(".layout")]
        for m in re.finditer(r'<SubTexture name="MENU_ItemIcon_(\d+)\.png" x="(\d+)" y="(\d+)" '
                             r'width="(\d+)" height="(\d+)"', xml.decode("utf-8", "replace")):
            sprites.setdefault(int(m.group(1)), (texture, int(m.group(2)), int(m.group(3)),
                                                 int(m.group(4)), int(m.group(5))))
    textures = dict(erdata.tpf_textures(archives.read("/menu/hi/01_common.tpf.dcx")))
    return sprites, textures


def write_icons(repo_root, sprites, textures, icon_ids, size):
    from PIL import Image

    out_dir = os.path.join(repo_root, ICON_DIR)
    os.makedirs(out_dir, exist_ok=True)
    atlases = {}
    written = set()
    for icon_id in sorted(icon_ids):
        sprite = sprites.get(icon_id)
        if sprite is None or sprite[0] not in textures:
            continue
        texture, x, y, w, h = sprite
        if texture not in atlases:
            image = Image.open(io.BytesIO(textures[texture]))
            image.load()
            atlases[texture] = image.convert("RGBA")
        icon = atlases[texture].crop((x, y, x + w, y + h)).resize((size, size), Image.LANCZOS)
        icon.save(os.path.join(out_dir, f"{icon_id}.png"), optimize=True)
        written.add(icon_id)
    return written


def existing_icons(repo_root):
    out_dir = os.path.join(repo_root, ICON_DIR)
    if not os.path.isdir(out_dir):
        return set()
    return {int(f[:-4]) for f in os.listdir(out_dir) if f.endswith(".png") and f[:-4].isdigit()}


def write_qrc(repo_root, icon_ids):
    lines = ["<RCC>", '    <qresource prefix="/er_inv_images">']
    lines += [f'        <file alias="{i}">images/er/item_icons/{i}.png</file>' for i in sorted(icon_ids)]
    lines += ["    </qresource>", "</RCC>", ""]
    with open(os.path.join(repo_root, "src", "resources", "er_inv_images.qrc"), "w", newline="\r\n") as f:
        f.write("\n".join(lines))


def main(game_dir, repo_root, with_icons=True, icon_size=128):
    archives = erdata.Archives(game_dir)
    names = load_names(archives)
    params = archives.regulation()

    def rows(param):
        return erdata.parse_param(params[param])

    items = []

    def add(type_nibble, item_id, category, name, icon_id):
        items.append(((type_nibble << 28) | item_id, category, name, icon_id))

    goods_names = names.get("GoodsName.fmg", {})
    for item_id, row in sorted(rows("EquipParamGoods.param").items()):
        name = goods_names.get(item_id)
        category = GOODS_TYPES.get(row[0x3E])
        if name and category:
            add(TYPE_GOODS, item_id, category, name, struct.unpack_from("<H", row, 0x30)[0])

    weapon_names = names.get("WeaponName.fmg", {})
    for item_id, row in sorted(rows("EquipParamWeapon.param").items()):
        name = weapon_names.get(item_id)
        if name:
            wep_type = struct.unpack_from("<H", row, 0x1A6)[0]
            add(TYPE_WEAPON, item_id, weapon_category(wep_type), name, struct.unpack_from("<H", row, 0xBE)[0])

    protector_names = names.get("ProtectorName.fmg", {})
    for item_id, row in sorted(rows("EquipParamProtector.param").items()):
        name = protector_names.get(item_id)
        if name:
            add(TYPE_PROTECTOR, item_id, protector_category(item_id), name,
                struct.unpack_from("<H", row, 0xA6)[0])

    accessory_names = names.get("AccessoryName.fmg", {})
    for item_id, row in sorted(rows("EquipParamAccessory.param").items()):
        name = accessory_names.get(item_id)
        if name:
            add(TYPE_ACCESSORY, item_id, "Talismans", name, struct.unpack_from("<H", row, 0x26)[0])

    gem_names = names.get("GemName.fmg", {})
    for item_id, row in sorted(rows("EquipParamGem.param").items()):
        name = gem_names.get(item_id)
        if name:
            add(TYPE_GEM, item_id, "AshesOfWar", name, struct.unpack_from("<H", row, 0x18)[0])

    if with_icons:
        sprites, textures = icon_sprites(archives)
        icons = write_icons(repo_root, sprites, textures, {i[3] for i in items}, icon_size)
        print(f"wrote {len(icons)} icons")
    else:
        icons = existing_icons(repo_root)
    write_qrc(repo_root, icons)

    lines = [
        "// Generated by tools/er_items/gen_items.py from regulation.bin, /msg/engus/item*.msgbnd",
        "// and the SB_Icon atlases. Do not edit by hand.",
        '#include "Items.h"',
        "",
        "namespace fssm::parse::er {",
        f"    const std::array<BaseItem, {len(items)}> ALL_ITEMS = {{",
    ]
    for item_id, category, name, icon_id in sorted(items):
        image = str(icon_id) if icon_id in icons else ""
        lines.append(f"        BaseItem{{0x{item_id:08x}, ItemCategory::{category}, "
                     f"{cpp_string(name)}, {cpp_string(image)}}},")
    lines += ["    };", "}", ""]
    out_path = os.path.join(repo_root, "src", "parse", "EldenRing", "Items.cpp")
    with open(out_path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {len(items)} items to {out_path}")


if __name__ == "__main__":
    args = sys.argv[3:]
    size = int(args[args.index("--icon-size") + 1]) if "--icon-size" in args else 128
    main(sys.argv[1], sys.argv[2], "--no-icons" not in args, size)
