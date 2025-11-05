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

def _fill_files(
    queue: collections.deque[Path],
    relative_dir: Path,
    dst_element: xmlET.Element,
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
        item: xmlET.Element = xmlET.Element("file", alias=alias)
        item.text = path
        dst_element.append(item)


def fill_dsr_files(
    common_root: xmlET.Element,
    inventory_root: xmlET.Element,
    resources_dir: Path,
    dsr_dir: Path,
) -> None:
    dsr_images_el: xmlET.Element = xmlET.Element(
        "qresource", prefix="/dsr_images"
    )
    common_root.append(dsr_images_el)

    dsr_inv_images_el: xmlET.Element = xmlET.Element(
        "qresource", prefix="/dsr_inv_images"
    )
    inventory_root.append(dsr_inv_images_el)

    common_res_dir_queue: collections.deque[Path] = collections.deque()
    inventory_res_dir_queue: collections.deque[Path] = collections.deque()
    for path in dsr_dir.iterdir():
        if path.name in DSR_INVENTORY_DIRS:
            inventory_res_dir_queue.append(path)
        else:
            common_res_dir_queue.append(path)

    _fill_files(
        common_res_dir_queue,
        resources_dir,
        dsr_images_el,
    )
    _fill_files(
        inventory_res_dir_queue,
        resources_dir,
        dsr_inv_images_el,
    )


def main() -> None:
    resources_dir = CURRENT_DIR / "src" / "resources"
    images_dir = resources_dir / "images"
    dsr_dir = images_dir / "dsr"
    images_rsc_path = resources_dir / "dsr_images.qrc"
    inv_images_rsc_path = resources_dir / "dsr_inventory_images.qrc"

    common_root = xmlET.Element("RCC")
    common_tree: xmlET.ElementTree = xmlET.ElementTree(common_root)

    inventory_root = xmlET.Element("RCC")
    inventory_tree: xmlET.ElementTree = xmlET.ElementTree(inventory_root)

    fill_dsr_files(common_root, inventory_root, resources_dir, dsr_dir)

    xmlET.indent(common_root, INDENT_SPACES)
    xmlET.indent(inventory_root, INDENT_SPACES)
    common_tree.write(str(images_rsc_path), encoding="utf-8")
    inventory_tree.write(str(inv_images_rsc_path), encoding="utf-8")


if __name__ == "__main__":
    main()
