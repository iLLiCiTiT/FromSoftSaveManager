from enum import StrEnum
from dataclasses import dataclass

import arrow
from PySide6 import QtCore

from from_soft_manager.parse import Game


@dataclass
class SaveItem:
    game: Game
    save_id: str
    save_path: str


class BackupType(StrEnum):
    quicksave = "quicksave"
    autosave = "autosave"
    manualsave = "manualsave"


@dataclass
class BackupInfo:
    backup_id: str
    backup_type: BackupType
    datetime: arrow.Arrow
    label: str | None


@dataclass
class CharactersInfo:
    save_id: str
    characters: list
    path: str | None
    error: str | None = None


@dataclass
class ConfigSavePathInfo:
    save_path: str
    save_path_hint: str
    save_path_default: str | None


@dataclass
class ConfigInfo:
    dsr_save_path: ConfigSavePathInfo
    ds2_save_path: ConfigSavePathInfo
    ds3_save_path: ConfigSavePathInfo
    er_save_path: ConfigSavePathInfo

    quicksave_hotkey: QtCore.QKeyCombination | None = None
    quickload_hotkey: QtCore.QKeyCombination | None = None


@dataclass
class ConfigConfirmData:
    dsr_save_path: str | None = None
    ds2_save_path: str | None = None
    ds3_save_path: str | None = None
    er_save_path: str | None = None

    quicksave_hotkey: QtCore.QKeyCombination | None = None
    quickload_hotkey: QtCore.QKeyCombination | None = None
