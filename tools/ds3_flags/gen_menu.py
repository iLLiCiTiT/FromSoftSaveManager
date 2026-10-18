"""Extract the Dark Souls III "Load Game" pieces used by the character list from the installed game data.

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
    MENU_ItemParts              atlas with the list scrollbar the screen uses (SCROLLBAR):
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
    for name, box in SEALS.items():
        image = parts.crop(box)
        image.save(os.path.join(out_dir, f"{name}.png"), optimize=True)
        print(f"{name}: {image.width}x{image.height}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)
    main(*sys.argv[1:])
