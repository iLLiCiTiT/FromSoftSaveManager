import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))


def get_icon_path(filename) -> str:
    return os.path.join(CURRENT_DIR, filename)
