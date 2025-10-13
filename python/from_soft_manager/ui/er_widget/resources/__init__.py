import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))


def get_resource(*args) -> str:
    return os.path.join(CURRENT_DIR, *args)


def get_icon_path(subdir: str, filename: str) -> str:
    return get_resource(subdir, filename)
