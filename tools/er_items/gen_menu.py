"""Extract the Elden Ring menu pieces used by the inventory view from the installed game data.

Usage:
    python gen_menu.py "<...>/ELDEN RING/Game" <repo root>

Writes:
    src/resources/images/er/menu/<alias>.png    menu sprites (see SPRITES)
    src/resources/er_images.qrc                 Qt resource list (keeps the existing entries)

Sprites come from the /menu/hi/01_common layouts and atlases (authored for 4K, so most
are halved).

Requires pycryptodome, zstandard and Pillow.
"""
import io
import os
import re
import sys

import erdata

MENU_DIR = os.path.join("src", "resources", "images", "er", "menu")

# alias -> (sprite name, scale)
SPRITES = {
    # Inventory category tabs (GR_MenuText 40500..40524)
    "tab_recent": ("MENU_Tab_41", 0.5),
    "tab_tools": ("MENU_Tab_00", 0.5),
    "tab_ashes": ("MENU_Tab_29_Buddy", 0.5),
    "tab_crafting": ("MENU_Tab_01", 0.5),
    "tab_bolstering": ("MENU_Tab_19", 0.5),
    "tab_key_items": ("MENU_Tab_03", 0.5),
    "tab_sorceries": ("MENU_Tab_30_PyroxeneMagic", 0.5),
    "tab_incantations": ("MENU_Tab_31_FaithMagic", 0.5),
    "tab_ashes_of_war": ("MENU_Tab_32_MagicStone", 0.5),
    "tab_melee": ("MENU_Tab_07", 0.5),
    "tab_ranged": ("MENU_Tab_08", 0.5),
    "tab_ammo": ("MENU_Tab_14", 0.5),
    "tab_shields": ("MENU_Tab_09", 0.5),
    "tab_head": ("MENU_Tab_10", 0.5),
    "tab_chest": ("MENU_Tab_11", 0.5),
    "tab_arms": ("MENU_Tab_12", 0.5),
    "tab_legs": ("MENU_Tab_13", 0.5),
    "tab_talismans": ("MENU_Tab_37", 0.5),
    "tab_info": ("MENU_Tab_04", 0.5),
    "tab_cursor": ("MENU_FL_Tab_Cursor", 0.5),
    "tab_base": ("MENU_FL_Tab_base", 0.5),
    # Inventory screen
    "inventory_icon": ("MENU_FL_Inventory", 0.5),
    "slot_base": ("MENU_FL_SlotBase_02", 0.5),
    "slot_cursor": ("MENU_SL_Cursor", 0.5),
    "item_frame": ("MENU_Item_waku", 0.5),
    "pouch_mark": ("MENU_InvMark", 1.0),
    "equip_mark": ("MENU_EquMark", 1.0),
    "group_line": ("MENU_Line_01", 0.5),
    "title_line": ("MENU_FL_Cursor_Top", 0.5),
    "scroll_bar": ("MENU_Sc_Bar", 1.0),
    "scroll_frame": ("MENU_Sc_Waku", 1.0),
}


def main(game_dir, repo_root):
    from PIL import Image

    archives = erdata.Archives(game_dir)
    layouts = erdata.parse_bnd4(archives.read("/menu/hi/01_common.sblytbnd.dcx"))
    sprites = {}
    for name, xml in layouts.items():
        if not name.endswith(".layout"):
            continue
        texture = name[:-len(".layout")]
        for m in re.finditer(r'<SubTexture name="([^"]+)\.png" x="(\d+)" y="(\d+)" '
                             r'width="(\d+)" height="(\d+)"', xml.decode("utf-8", "replace")):
            sprites.setdefault(m.group(1), (texture, *map(int, m.groups()[1:])))
    textures = dict(erdata.tpf_textures(archives.read("/menu/hi/01_common.tpf.dcx")))

    out_dir = os.path.join(repo_root, MENU_DIR)
    os.makedirs(out_dir, exist_ok=True)
    atlases = {}
    for alias, (sprite, scale) in SPRITES.items():
        texture, x, y, w, h = sprites[sprite]
        if texture not in atlases:
            atlases[texture] = Image.open(io.BytesIO(textures[texture])).convert("RGBA")
        image = atlases[texture].crop((x, y, x + w, y + h))
        if scale != 1.0:
            image = image.resize((max(1, round(w * scale)), max(1, round(h * scale))), Image.LANCZOS)
        image.save(os.path.join(out_dir, f"{alias}.png"), optimize=True)
    print(f"wrote {len(SPRITES)} sprites to {out_dir}")

    qrc_path = os.path.join(repo_root, "src", "resources", "er_images.qrc")
    with open(qrc_path, encoding="utf-8") as f:
        existing = dict(re.findall(r'<file alias="([^"]+)">([^<]+)</file>', f.read()))
    for alias in SPRITES:
        existing[alias] = f"images/er/menu/{alias}.png"
    lines = ["<RCC>", '    <qresource prefix="/er_images">']
    lines += [f'        <file alias="{a}">{p}</file>' for a, p in sorted(existing.items())]
    lines += ["    </qresource>", "</RCC>", ""]
    with open(qrc_path, "w", newline="\r\n") as f:
        f.write("\n".join(lines))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
