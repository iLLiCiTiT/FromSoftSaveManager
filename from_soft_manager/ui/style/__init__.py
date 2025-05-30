import os

from PySide6 import QtGui

RESOURCES_DIR = os.path.dirname(os.path.abspath(__file__))

# Define colors used in the stylesheet
# - used for faster testing of stylesheets
_COLORS = {
    "default-bg": "#060507",
    "default-fg": "#FFFFFF",
    "inputs-bg": "#0B0D0D",
    "inputs-border": "#5B4229",
    "button-bg": "#232629",
    "button-bg-hover": "#4F5B62",
    "button-fg": "#ffffff",
    "tab-button-bg-hover": "#232629",
    "tab-button-border": "#232629",
    "tab-button-border-selected": "#5B4229",
}


class _Cache:
    stylesheet = None
    font_ids = None


def get_resource(*args) -> str:
    return os.path.join(RESOURCES_DIR, *args)


def load_stylesheet() -> str:
    if _Cache.stylesheet is None:
        stylesheet_path = get_resource("style.qss")
        with open(stylesheet_path, "r") as file:
            content = file.read()
        for key, value in _COLORS.items():
            content = content.replace(f"{{{key}}}", value)
        _Cache.stylesheet = content
        _load_font()
    return _Cache.stylesheet


def get_liberation_font_path(bold: bool = False, italic: bool = False):
    font_name = "LiberationSans"
    suffix = ""
    if bold:
        suffix += "Bold"
    if italic:
        suffix += "Italic"

    if not suffix:
        suffix = "Regular"

    filename = f"{font_name}-{suffix}.ttf"
    font_path = get_resource("fonts", font_name, filename)
    return font_path



def _load_font():
    """Load and register fonts into Qt application."""
    # Check if font ids are still loaded
    if _Cache.font_ids is not None:
        for font_id in tuple(_Cache.font_ids):
            font_families = QtGui.QFontDatabase.applicationFontFamilies(
                font_id
            )
            # Reset font if font id is not available
            if not font_families:
                _Cache.font_ids = None
                break

    if _Cache.font_ids is None:
        _Cache.font_ids = []
        font_dir = get_resource("fonts", "Noto_Sans")

        loaded_fonts = []
        for filename in os.listdir(font_dir):
            if os.path.splitext(filename)[1] not in [".ttf"]:
                continue
            full_path = os.path.join(font_dir, filename)
            font_id = QtGui.QFontDatabase.addApplicationFont(full_path)
            _Cache.font_ids.append(font_id)
            font_families = QtGui.QFontDatabase.applicationFontFamilies(
                font_id
            )
            loaded_fonts.extend(font_families)
        print("Registered font families: {}".format(", ".join(loaded_fonts)))
