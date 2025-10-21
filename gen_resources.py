import os
import collections
from pathlib import Path
from xml.etree import ElementTree as xmlET

CURRENT_DIR = Path(__file__).parent
INDENTATION = 4
INDENT_SPACES = " " * INDENTATION
SINGLE_INDENT = f"\n{INDENT_SPACES}"
DOUBLE_INDENT = f"\n{INDENT_SPACES}{INDENT_SPACES}"


def fill_dsr_files(
    root: xmlET.Element,
    ui_dir: Path,
    dsr_dir: Path,
) -> None:
    prev_item: xmlET.Element = root
    dsr_resource: xmlET.Element | None = None
    for item in root:
        prev_item = item
        if item.tag != "qresource":
            continue
        if item.attrib.get("prefix") == "/dsr_images":
            dsr_resource = item
            break

    if dsr_resource is None:
        prev_item.tail = SINGLE_INDENT
        dsr_resource: xmlET.Element = xmlET.Element(
            "qresource", prefix="/dsr_images"
        )
        dsr_resource.text = DOUBLE_INDENT
        dsr_resource.tail = "\n"
        root.append(dsr_resource)

    file_r_by_path: dict[str, xmlET.Element] = {
        file_r.text: file_r
        for file_r in dsr_resource
    }
    dsr_files: set[str] = set()
    dirs_queue: collections.deque[Path] = collections.deque()
    dirs_queue.append(dsr_dir)
    while dirs_queue:
        dir_path = dirs_queue.popleft()
        for path in dir_path.iterdir():
            if path.is_file():
                relative_path = str(path.relative_to(ui_dir))
                dsr_files.add(relative_path.replace("\\", "/"))
            else:
                dirs_queue.append(path)

    existing_paths = set(file_r_by_path.keys())
    for path in existing_paths - dsr_files:
        item: xmlET.Element = file_r_by_path.pop(path)
        dsr_resource.remove(item)
        existing_paths.discard(path)

    last_item = None
    for path in dsr_files - existing_paths:
        alias: str = os.path.basename(path)
        item: xmlET.Element = xmlET.Element("file", alias=alias)
        item.text = path
        item.tail = DOUBLE_INDENT
        last_item = item
        dsr_resource.append(item)

    if last_item is not None:
        last_item.tail = SINGLE_INDENT


def main() -> None:
    ui_dir = CURRENT_DIR / "src" / "ui"
    resources_dir = ui_dir / "resources"
    images_dir = resources_dir / "images"
    dsr_dir = images_dir / "dsr"

    resources_path = ui_dir / "resources.qrc"
    tree = xmlET.parse(resources_path)
    root = tree.getroot()

    fill_dsr_files(root, ui_dir, dsr_dir)

    tree.write(str(resources_path), encoding="utf-8")


if __name__ == "__main__":
    main()
