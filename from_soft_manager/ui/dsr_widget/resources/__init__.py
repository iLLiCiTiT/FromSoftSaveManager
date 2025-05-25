import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))


def get_icon_path(subdir: str, filename: str) -> str:
    return os.path.join(CURRENT_DIR, subdir, filename)


def get_status_icon_path(status_name: str) -> str:
    return get_icon_path("status_icons", status_name + ".png")
