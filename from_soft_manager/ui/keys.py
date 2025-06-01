import platform

from PySide6 import QtCore


if platform.system().lower() == "windows":
    from ._keys_windows import (
        keys_are_pressed,
        qt_combination_to_int,
        int_combination_to_qt,
    )
else:
    def qt_combination_to_int(keys: QtCore.QKeyCombination | None) -> set[int]:
        """
        Convert a set of Qt key codes to an integer representation.
        """
        return set()


    def int_combination_to_qt(keys: set[int]) -> QtCore.QKeyCombination:
        return QtCore.QKeyCombination()


    def keys_are_pressed(keys: set[int]):
        return False


__all__ = (
    "keys_are_pressed",
    "qt_combination_to_int",
    "int_combination_to_qt",
)
