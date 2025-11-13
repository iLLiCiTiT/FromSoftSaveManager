import os
import collections
from pathlib import Path
from xml.etree import ElementTree as xmlET

CURRENT_DIR = Path(__file__).parent
INDENT_SPACES = " " * 4

DSR_INVENTORY_DIRS = {
    "ammunition",
    "armor",
    "consumables",
    "infusions",
    "key_items",
    "materials",
    "rings",
    "spells",
    "weapons_shields",
}
DS3_INVENTORY_DIRS = {
    "infusion_icons",
    "item_icons",
}
ER_INVENTORY_DIRS = set()


def _fill_files(
    queue: collections.deque[Path],
    relative_dir: Path,
    dst_element: xmlET.Element,
    remove_ext: bool = False,
) -> None:
    filepaths: set[str] = set()
    while queue:
        path = queue.popleft()
        if path.is_file():
            relative_path = str(path.relative_to(relative_dir))
            filepaths.add(relative_path.replace("\\", "/"))
            continue

        for child in path.iterdir():
            queue.append(child)

    for path in filepaths:
        alias: str = os.path.basename(path)
        if remove_ext:
            alias = os.path.splitext(alias)[0]
        item: xmlET.Element = xmlET.Element("file", alias=alias)
        item.text = path
        dst_element.append(item)


def fill_files(
    images_el: xmlET.Element,
    inv_images_el: xmlET.Element,
    resources_dir: Path,
    game_dir: Path,
    inv_dirs: set[str],
) -> None:
    common_res_dir_queue: collections.deque[Path] = collections.deque()
    inventory_res_dir_queue: collections.deque[Path] = collections.deque()
    for path in game_dir.iterdir():
        if path.name in inv_dirs:
            inventory_res_dir_queue.append(path)
        else:
            common_res_dir_queue.append(path)

    _fill_files(
        common_res_dir_queue,
        resources_dir,
        images_el,
        remove_ext=True,
    )
    _fill_files(
        inventory_res_dir_queue,
        resources_dir,
        inv_images_el,
        remove_ext=True,
    )


def create_resources(
    resources_dir: Path,
    game_dir: Path,
    common_prefix: str,
    inventory_prefix: str,
    inventory_dirs: set[str],
) -> None:
    images_rsc_path = resources_dir / f"{common_prefix}.qrc"
    inv_images_rsc_path = resources_dir / f"{inventory_prefix}.qrc"

    images_el: xmlET.Element = xmlET.Element(
        "qresource", prefix=f"/{common_prefix}"
    )
    inv_images_el: xmlET.Element = xmlET.Element(
        "qresource", prefix=f"/{inventory_prefix}"
    )

    common_root = xmlET.Element("RCC")
    common_root.append(images_el)

    inventory_root = xmlET.Element("RCC")
    inventory_root.append(inv_images_el)

    fill_files(
        images_el,
        inv_images_el,
        resources_dir,
        game_dir,
        inventory_dirs,
    )
    for (root, element, path) in (
        (common_root, images_el, images_rsc_path),
        (inventory_root, inv_images_el, inv_images_rsc_path),
    ):
        if len(element) == 0:
            if path.exists():
                path.unlink()
            continue
        element[:] = sorted(element, key=lambda x: x.attrib["alias"])
        tree: xmlET.ElementTree = xmlET.ElementTree(root)
        xmlET.indent(tree, INDENT_SPACES)
        tree.write(str(path), encoding="utf-8")


def create_dsr_resources(resources_dir: Path) -> None:
    dsr_dir = resources_dir / "images" / "dsr"
    create_resources(
        resources_dir,
        dsr_dir,
        "dsr_images",
        "dsr_inv_images",
        DSR_INVENTORY_DIRS,
    )


def create_ds3_resources(resources_dir: Path) -> None:
    images_dir = resources_dir / "images"
    ds3_dir = images_dir / "ds3"
    create_resources(
        resources_dir,
        ds3_dir,
        "ds3_images",
        "ds3_inv_images",
        DS3_INVENTORY_DIRS,
    )


def create_er_resources(resources_dir: Path) -> None:
    er_dir = resources_dir / "images" / "er"
    create_resources(
        resources_dir,
        er_dir,
        "er_images",
        "er_inv_images",
        ER_INVENTORY_DIRS,
    )


def main() -> None:
    resources_dir = CURRENT_DIR / "src" / "resources"
    create_dsr_resources(resources_dir)
    create_ds3_resources(resources_dir)
    create_er_resources(resources_dir)


if __name__ == "__main__":
    main()
