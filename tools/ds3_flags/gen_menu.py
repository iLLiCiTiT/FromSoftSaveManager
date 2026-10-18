"""Extract the Dark Souls III menu pieces used by the DS3 views from the installed game data.

Usage:
    python gen_menu.py "<...>/DARK SOULS III/Game" <DecryptionKeys.cs> <repo root>

Writes:
    src/resources/images/ds3/menu/<name>.png    sprites (see main), referenced by
                                                src/resources/ds3_images.qrc

The textures are in /menu/01_common.tpf.dcx (Data1-5 archives, see gen_flags.py), BC7:
    MENU_ProfileSelect          a save slot: ornament strip, portrait box, then the
                                name/level area with its gold rules
    MENU_ProfileSelect_Cursor   orange glow over the selected slot (same size)
    MENU_ProfileSelect_Base     atlas; the framed panel the slots sit in is its
                                (41, 4)-(1372, 802) part (the rest is a smoke sprite)
    MENU_DetailStatus_Base      atlas of the status screen's pages; PANELS takes the left
                                one for the character info
    MENU_InventoryBase          atlas; PANELS takes the inventory panel: the inventory
                                screen (/menu/02_020_inventory.gfx, shape 166) places it
                                with the colour transform INVENTORY_TINT, which gives it
                                its purple cast, over an opaque backdrop, so it is
                                flattened onto black (its vignette is in its alpha) and
                                only its ragged edge stays see-through. The character info
                                panel is a translucent dark sheet drawn over the background.
                                The atlas also has the item list's cell layers
                                (CELL_LAYERS, shapes 172/175/178 of the .gfx): its
                                ItemList sprite puts on every cell a shadow (multiply,
                                alpha 115/256), a highlight (screen, 15/256) and, at the
                                cell corners, a cross of grid lines (multiply, 26/256).
                                The cells are 100 x 110 there; the masks 96 x 106.
    MENU_ItemParts              atlas; CELLS takes the orange square over the selected
                                item cell; it also holds the list scrollbar (SCROLLBAR):
                                track and thumb lie horizontally (the game turns them
                                upright), the end caps already point up and down; also
                                the inventory's wax seals (SEALS): red on equipped items,
                                grey on items in the menu's quick items (each also has a
                                darker variant, unused)
The app has no portraits, so the slots lose the portrait box (x 54-204). Sprites are
authored for 4K and halved, except the scrollbar, which is thin enough as it is.

Requires pycryptodome and Pillow.
"""
import io
import os
import struct
import sys

import gen_flags

MENU_DIR = os.path.join("src", "resources", "images", "ds3", "menu")
COMMON_TPF = "/menu/01_common.tpf.dcx"
SCALE = 0.5
# Columns of MENU_ProfileSelect(_Cursor) kept: the left ornament strip, then everything
# after the portrait box.
SLOT_KEEP = ((0, 54), (204, None))
PANEL_BOX = (41, 4, 1372, 802)
# name -> (box in MENU_ItemParts, turned upright)
SCROLLBAR = {
    "scroll_track": ((4, 487, 133, 497), True),
    "scroll_thumb": ((4, 504, 132, 509), True),
    "scroll_up": ((140, 487, 149, 497), False),
    "scroll_down": ((156, 487, 165, 497), False),
}
# name -> (texture, box, colour transform (multiply / 256, add) or None, flatten onto black)
INVENTORY_TINT = ((247, 247, 247), (2, 0, 4))
PANELS = {
    "panel_status": ("MENU_DetailStatus_Base", (8, 6, 650, 802), None, False),
    "panel_inventory": ("MENU_InventoryBase", (4, 24, 624, 696), INVENTORY_TINT, True),
}
# Alpha from which a flattened panel is fully opaque (its least opaque inner part is ~182).
OPAQUE_FROM = 180
# name -> (texture, box), kept at full size
CELLS = {
    "inv_cell_cursor": ("MENU_ItemParts", (7, 307, 177, 470)),
}
# name -> box in MENU_InventoryBase, kept at full size
CELL_LAYERS = {
    "inv_cell_shadow": (268, 904, 364, 1010),
    "inv_cell_highlight": (372, 904, 468, 1010),
    "inv_cell_grid": (586, 763, 686, 873),
}
SEALS = {
    "seal_equipped": (190, 474, 218, 502),
    "seal_quick_menu": (270, 474, 298, 502),
}


def parse_tpf(data):
    if data[:4] != b"TPF\0":
        raise RuntimeError("expected TPF")
    count = struct.unpack_from("<I", data, 8)[0]
    textures = {}
    for i in range(count):
        offset, size, _fmt, _type, _mips, _flags, name_offset = struct.unpack_from("<IIBBBBI", data, 0x10 + i * 0x14)
        end = name_offset
        while data[end:end + 2] != b"\0\0":
            end += 2
        textures[data[name_offset:end].decode("utf-16le")] = data[offset:offset + size]
    return textures


def main(game_dir, keys_path, repo_root):
    from PIL import Image

    archives = [gen_flags.Archive(game_dir, keys_path, name) for name in gen_flags.ARCHIVES]
    data = gen_flags.read_file(archives, COMMON_TPF)
    if data is None:
        raise RuntimeError(f"{COMMON_TPF} not found")
    textures = parse_tpf(data)

    def texture(name):
        image = Image.open(io.BytesIO(textures[name]))
        image.load()
        return image.convert("RGBA")

    def without_portrait(image):
        parts = [image.crop((start, 0, end or image.width, image.height)) for start, end in SLOT_KEEP]
        out = Image.new("RGBA", (sum(p.width for p in parts), image.height))
        x = 0
        for part in parts:
            out.paste(part, (x, 0))
            x += part.width
        return out

    sprites = {
        "load_slot": without_portrait(texture("MENU_ProfileSelect")),
        "load_slot_cursor": without_portrait(texture("MENU_ProfileSelect_Cursor")),
        "load_panel": texture("MENU_ProfileSelect_Base").crop(PANEL_BOX),
    }
    for name, (texture_name, box, tint, flatten) in PANELS.items():
        image = texture(texture_name).crop(box)
        if tint is not None:
            (mr, mg, mb), (ar, ag, ab) = tint
            r, g, b, a = image.split()
            r = r.point(lambda v: min(255, v * mr // 256 + ar))
            g = g.point(lambda v: min(255, v * mg // 256 + ag))
            b = b.point(lambda v: min(255, v * mb // 256 + ab))
            image = Image.merge("RGBA", (r, g, b, a))
        if flatten:
            alpha = image.getchannel("A")
            flat = Image.alpha_composite(Image.new("RGBA", image.size, (0, 0, 0, 255)), image)
            flat.putalpha(alpha.point(lambda v: min(255, v * 255 // OPAQUE_FROM)))
            image = flat
        sprites[name] = image
    out_dir = os.path.join(repo_root, MENU_DIR)
    os.makedirs(out_dir, exist_ok=True)
    for name, image in sprites.items():
        image = image.resize((round(image.width * SCALE), round(image.height * SCALE)), Image.LANCZOS)
        image.save(os.path.join(out_dir, f"{name}.png"), optimize=True)
        print(f"{name}: {image.width}x{image.height}")
    parts = texture("MENU_ItemParts")
    for name, (box, upright) in SCROLLBAR.items():
        image = parts.crop(box)
        if upright:
            image = image.transpose(Image.ROTATE_90)
        image.save(os.path.join(out_dir, f"{name}.png"), optimize=True)
        print(f"{name}: {image.width}x{image.height}")
    for name, (texture_name, box) in CELLS.items():
        image = texture(texture_name).crop(box)
        image.save(os.path.join(out_dir, f"{name}.png"), optimize=True)
        print(f"{name}: {image.width}x{image.height}")
    inventory_base = texture("MENU_InventoryBase")
    for name, box in CELL_LAYERS.items():
        image = inventory_base.crop(box)
        image.save(os.path.join(out_dir, f"{name}.png"), optimize=True)
        print(f"{name}: {image.width}x{image.height}")
    for name, box in SEALS.items():
        image = parts.crop(box)
        image.save(os.path.join(out_dir, f"{name}.png"), optimize=True)
        print(f"{name}: {image.width}x{image.height}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)
    main(*sys.argv[1:])
