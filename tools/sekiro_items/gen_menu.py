"""Extract the Sekiro menu pieces used by the Sekiro views from the installed game data.

Usage:
    python gen_menu.py "<...>/Sekiro" <sekiro exe> <repo root>

Writes:
    src/resources/images/sekiro/menu/<name>.png    menu sprites (see SPRITES)
    src/resources/images/sekiro/skill_story/<icon id>.png
                                                   skill illustrations (MENU_SkillStory_<icon id>),
                                                   keyed like the item icons of the skills,
                                                   256 colours to keep them small
    src/resources/sekiro_skill_images.qrc          Qt resource list for the illustrations
                                                   (optional bundle, like the item icons)
    src/resources/sekiro_images.qrc                Qt resource list (keeps the existing bg)

Sprites come from the /menu/hi/01_common layouts and atlases. They are authored for
3840x2160, so the large ones are halved to match the 1920x1080 background.

Requires pycryptodome and Pillow.
"""
import io
import os
import re
import sys

import sekirodata

MENU_DIR = os.path.join("src", "resources", "images", "sekiro", "menu")
SKILL_STORY_DIR = os.path.join("src", "resources", "images", "sekiro", "skill_story")
SKILL_STORY_SCALE = 0.75

# alias -> (sprite name, scale)
SPRITES = {
    "profile_row": ("MENU_Profail_01", 0.5),
    "profile_row_selected": ("MENU_Profail_01_Cursol", 0.5),
    "item_row": ("MENU_ItemBase_02", 0.5),
    "item_row_selected": ("MENU_Cursor_05", 0.5),
    "icon_base": ("MENU_IconBase_02", 0.5),
    "tab": ("MENU_Caption_01", 0.5),
    "tab_selected": ("MENU_Cursor_01", 0.5),
    "title_bar": ("MENU_ItemBase_08", 0.5),
    "stat_bar": ("MENU_Caption_02", 0.5),
    "header_bar": ("MENU_ItemBase_09", 0.5),
    "detail_panel": ("MENU_Dialog_base_02", 0.5),
    "plate": ("MENU_BlackPlate_02", 0.5),
    "category_cursor": ("MENU_Cursor_TB", 1.0),
    "category_quick_items": ("MENU_TB_Item", 1.0),
    "category_key_items": ("MENU_TB_valuable", 1.0),
    "category_materials": ("MENU_TB_Material", 1.0),
    "category_prosthetics": ("MENU_TB_Arm", 1.0),
    "icon_vitality": ("MENU_ST_Vitality", 1.0),
    "icon_attack": ("MENU_ST_Offensive", 1.0),
    "icon_sen": ("MENU_Money", 1.0),
    "arrow": ("MENU_Arrow", 1.0),
}


def main(game_dir, exe_path, repo_root):
    from PIL import Image

    archive = sekirodata.Archive(game_dir, exe_path, "Data1")
    sprites, textures = sekirodata.menu_sprites(archive)
    out_dir = os.path.join(repo_root, MENU_DIR)
    os.makedirs(out_dir, exist_ok=True)
    atlases = {}
    for alias, (sprite, scale) in SPRITES.items():
        texture, x, y, w, h = sprites[sprite]
        if texture not in atlases:
            image = Image.open(io.BytesIO(textures[texture]))
            image.load()
            atlases[texture] = image.convert("RGBA")
        image = atlases[texture].crop((x, y, x + w, y + h))
        if scale != 1.0:
            image = image.resize((max(1, round(w * scale)), max(1, round(h * scale))), Image.LANCZOS)
        image.save(os.path.join(out_dir, f"{alias}.png"), optimize=True)

    story_dir = os.path.join(repo_root, SKILL_STORY_DIR)
    os.makedirs(story_dir, exist_ok=True)
    stories = []
    for name, (texture, x, y, w, h) in sorted(sprites.items()):
        m = re.fullmatch(r"MENU_SkillStory_(\d+)", name)
        if not m:
            continue                    # also skips the "...d" placeholder sprites
        if texture not in atlases:
            image = Image.open(io.BytesIO(textures[texture]))
            image.load()
            atlases[texture] = image.convert("RGBA")
        image = atlases[texture].crop((x, y, x + w, y + h))
        image = image.resize((round(w * SKILL_STORY_SCALE), round(h * SKILL_STORY_SCALE)), Image.LANCZOS)
        icon_id = int(m.group(1))
        image = image.quantize(colors=256, method=Image.Quantize.FASTOCTREE)
        image.save(os.path.join(story_dir, f"{icon_id}.png"), optimize=True)
        stories.append(icon_id)

    lines = ["<RCC>", '    <qresource prefix="/sekiro_images">',
             '        <file alias="bg">images/sekiro/menu/bg.png</file>']
    lines += [f'        <file alias="{alias}">images/sekiro/menu/{alias}.png</file>' for alias in sorted(SPRITES)]
    lines += ["    </qresource>", "</RCC>", ""]
    with open(os.path.join(repo_root, "src", "resources", "sekiro_images.qrc"), "w", newline="\r\n") as f:
        f.write("\n".join(lines))
    lines = ["<RCC>", '    <qresource prefix="/sekiro_skill_images">']
    lines += [f'        <file alias="{i}">images/sekiro/skill_story/{i}.png</file>' for i in stories]
    lines += ["    </qresource>", "</RCC>", ""]
    with open(os.path.join(repo_root, "src", "resources", "sekiro_skill_images.qrc"), "w", newline="\r\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {len(SPRITES)} menu sprites and {len(stories)} skill illustrations")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3])
