"""Generate the Sekiro item database and inventory icons from the installed game data.

Usage:
    python gen_items.py "<...>/Sekiro" <sekiro exe> <repo root> [--no-icons]

The exe only provides the archive keys; the Steam exe works (its .rdata is not
encrypted by SteamStub).

Writes:
    src/parse/Sekiro/Items.cpp                         item database
    src/resources/images/sekiro/item_icons/<id>.png    item icons (160x160)
    src/resources/sekiro_inv_images.qrc                Qt resource list for the icons
    src/parse/Sekiro/SkillTree.cpp                     the five skill trees (SkillParam)

Names and descriptions come from /msg/engus/item.msgbnd.dcx; icon ids from
EquipParamGoods (+0x2C) and EquipParamWeapon (+0xBA), sort order (+0x1C) and maximum
held count (+0x36) and whether the inventory shows the number held (+0x6A == 2: quick
items, materials, Memories, sake and bait; not Prayer Beads or unique key items; and no
Spirit Emblem cost at +0x74, which Spiritfalls and Ninjutsu have; and not the reusable
quick items, +0x2D == 1: Homeward Idol, Illusive Hall Bell, Nightjar Monocular, Hidden
Tooth) from EquipParamGoods in GameParam.parambnd; icons from the item
icon atlases of /menu/hi/01_common. Categories follow Sekiro's item id ranges. Rows without an
English name are internal and skipped.

Requires pycryptodome and Pillow.
"""
import io
import os
import struct
import sys

import sekirodata

ICON_SIZE = (160, 160)
ICON_DIR = os.path.join("src", "resources", "images", "sekiro", "item_icons")

# Item id type nibble used in the save (InventoryItem::itemId >> 28).
TYPE_WEAPON = 0x0
TYPE_GOODS = 0x4


def goods_category(goods_id):
    block = goods_id // 1000
    if block == 1:
        return "SpiritEmblems"      # Spirit Emblem, Skill Point, ...
    if block == 3:
        return "QuickItems"         # Healing Gourd, Pellet, ...
    if block == 4:
        return "PrayerBeads"
    if block == 5:
        return "Memories"
    if block == 6:
        return "Materials"          # Scrap Iron, Black Gunpowder, ...
    if block in (2, 9, 10):
        return "KeyItems"
    return None                     # boss names (11xxx), debug entries (90xxx)


def weapon_category(weapon_id):
    return "ProstheticTools" if weapon_id >= 700000 else "CombatArts"


def description(text):
    if not text or text in ("-", "<?null?>"):
        return ""
    return text.replace("\r", "").strip()


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def write_icons(repo_root, sprites, textures, icon_ids):
    from PIL import Image

    out_dir = os.path.join(repo_root, ICON_DIR)
    os.makedirs(out_dir, exist_ok=True)
    atlases = {}
    written = set()
    for icon_id in sorted(icon_ids):
        sprite = sprites.get(icon_id)
        if sprite is None:
            continue
        texture, x, y, w, h = sprite
        if texture not in atlases:
            image = Image.open(io.BytesIO(textures[texture]))
            image.load()
            atlases[texture] = image.convert("RGBA")
        icon = atlases[texture].crop((x, y, x + w, y + h)).resize(ICON_SIZE, Image.LANCZOS)
        icon.save(os.path.join(out_dir, f"{icon_id}.png"), optimize=True)
        written.add(icon_id)
    return written


def existing_icons(repo_root):
    out_dir = os.path.join(repo_root, ICON_DIR)
    if not os.path.isdir(out_dir):
        return set()
    return {int(f[:-4]) for f in os.listdir(out_dir) if f.endswith(".png") and f[:-4].isdigit()}


def write_qrc(repo_root, icon_ids):
    lines = ["<RCC>", '    <qresource prefix="/sekiro_inv_images">']
    lines += [f'        <file alias="{i}">images/sekiro/item_icons/{i}.png</file>' for i in sorted(icon_ids)]
    lines += ["    </qresource>", "</RCC>", ""]
    with open(os.path.join(repo_root, "src", "resources", "sekiro_inv_images.qrc"), "w", newline="\r\n") as f:
        f.write("\n".join(lines))


def main(game_dir, exe_path, repo_root, with_icons=True):
    archive = sekirodata.Archive(game_dir, exe_path, "Data1")
    msg = archive.bnd("/msg/engus/item.msgbnd.dcx")
    goods_names = sekirodata.parse_fmg2(msg["アイテム名.fmg"])
    weapon_names = sekirodata.parse_fmg2(msg["武器名.fmg"])
    goods_descriptions = sekirodata.parse_fmg2(msg["アイテム説明.fmg"])
    weapon_descriptions = sekirodata.parse_fmg2(msg["武器説明.fmg"])
    params = archive.bnd("/param/gameparam/gameparam.parambnd.dcx")
    goods = sekirodata.parse_param(params["EquipParamGoods.param"])
    weapons = sekirodata.parse_param(params["EquipParamWeapon.param"])

    items = []
    seen = set()
    for item_id, row in sorted(goods.items()):
        name = goods_names.get(item_id)
        category = goods_category(item_id)
        # Usable from the quick item slots (+0x43 == 0): listed with the quick items
        # whatever the id block, e.g. Dragon's Blood Droplet.
        if category not in (None, "SpiritEmblems") and row[0x43] == 0:
            category = "QuickItems"
        if not name or name in ("-", "<?null?>") or category is None:
            continue
        icon_id = struct.unpack_from("<H", row, 0x2C)[0]
        sort_id = struct.unpack_from("<i", row, 0x1C)[0]
        max_num = struct.unpack_from("<h", row, 0x36)[0]
        reusable_quick_item = row[0x43] == 0 and row[0x2D] == 1
        shows_count = row[0x6A] == 2 and row[0x74] == 0 and not reusable_quick_item
        items.append(((TYPE_GOODS << 28) | item_id, category, name, icon_id, sort_id, max(0, max_num),
                      shows_count, description(goods_descriptions.get(item_id))))
        seen.add(item_id)
    for item_id, row in sorted(weapons.items()):
        name = weapon_names.get(item_id)
        if not name or name in ("-", "<?null?>"):
            continue
        icon_id = struct.unpack_from("<H", row, 0xBA)[0]
        items.append(((TYPE_WEAPON << 28) | item_id, weapon_category(item_id), name, icon_id, item_id, 0,
                      False, description(weapon_descriptions.get(item_id))))

    if with_icons:
        sprites, textures = sekirodata.icon_atlases(archive)
        icons = write_icons(repo_root, sprites, textures, {i[3] for i in items})
        print(f"wrote {len(icons)} icons")
    else:
        icons = existing_icons(repo_root)
    write_qrc(repo_root, icons)

    lines = [
        "// Generated by tools/sekiro_items/gen_items.py from EquipParamGoods/EquipParamWeapon,",
        "// /msg/engus/item.msgbnd and the item icon atlases. Do not edit by hand.",
        '#include "Items.h"',
        "",
        "namespace fssm::parse::sekiro {",
        f"    const std::array<BaseItem, {len(items)}> ALL_ITEMS = {{",
    ]
    for item_id, category, name, icon_id, sort_id, max_num, shows_count, desc in sorted(items):
        image = str(icon_id) if icon_id in icons else ""
        lines.append(f"        BaseItem{{0x{item_id:08x}, ItemCategory::{category}, "
                     f"{cpp_string(name)}, {cpp_string(image)}, {sort_id}, {max_num}, "
                     f"{'true' if shows_count else 'false'},")
        lines.append(f"                 {cpp_string(desc)}}},")
    lines += ["    };", "}", ""]
    out_path = os.path.join(repo_root, "src", "parse", "Sekiro", "Items.cpp")
    with open(out_path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {len(items)} items to {out_path}")
    write_skill_tree(repo_root, params, {i[0] for i in items})


def write_skill_tree(repo_root, params, item_ids):
    """SkillParam rows of the five Acquire Skills trees.

    Row layout: +0x00 skill item (weapon id), +0x08/+0x0C prerequisite rows (-1 none),
    +0x28 menu position (column << 16 | row), +0x34 tree (0..4, 255 = not in a tree).
    Rows 700000+ are the prosthetic upgrade tree and are skipped.
    """
    rows = sekirodata.parse_param(params["SkillParam.param"])
    nodes = []
    for row_id, row in sorted(rows.items()):
        item_id, = struct.unpack_from("<i", row, 0x00)
        pre1, pre2 = struct.unpack_from("<ii", row, 0x08)
        position, = struct.unpack_from("<I", row, 0x28)
        tree = row[0x34]
        if tree > 4 or row_id >= 700:
            continue
        if (TYPE_WEAPON << 28) | item_id not in item_ids:
            raise RuntimeError(f"skill row {row_id} grants unknown item {item_id}")
        nodes.append((row_id, item_id, tree, position >> 16, position & 0xFFFF, pre1, pre2))

    lines = [
        "// Generated by tools/sekiro_items/gen_items.py from SkillParam. Do not edit by hand.",
        '#include "SkillTree.h"',
        "",
        "namespace fssm::parse::sekiro {",
        f"    const std::array<SkillNode, {len(nodes)}> SKILL_TREE = {{{{",
    ]
    for row_id, item_id, tree, column, line, pre1, pre2 in nodes:
        lines.append(f"        {{{row_id}, 0x{(TYPE_WEAPON << 28) | item_id:08x}, {tree}, {column}, {line}, "
                     f"{{{pre1}, {pre2}}}}},")
    lines += ["    }};", "}", ""]
    out_path = os.path.join(repo_root, "src", "parse", "Sekiro", "SkillTree.cpp")
    with open(out_path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {len(nodes)} skill tree nodes to {out_path}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3], "--no-icons" not in sys.argv[4:])
