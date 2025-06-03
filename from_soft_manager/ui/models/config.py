import os
import json
import uuid

import platformdirs
from PySide6 import QtCore

from from_soft_manager.parse import Game
from from_soft_manager.ui.structures import (
    SaveItem,
    ConfigInfo,
    ConfigSavePathInfo,
    ConfigConfirmData,
)
from from_soft_manager.ui.keys import (
    qt_combination_to_int,
    int_combination_to_qt,
)

DOCUMENTS_ID = uuid.UUID("{F42EE2D3-909F-4907-8871-4C22FC0BF756}")
NOT_SET = object()


def _get_windows_documents_dir() -> str:
    import ctypes
    from ctypes import windll, wintypes

    class GUID(ctypes.Structure):
        _fields_ = [
            ("Data1", wintypes.DWORD),
            ("Data2", wintypes.WORD),
            ("Data3", wintypes.WORD),
            ("Data4", wintypes.BYTE * 8)
        ]

        def __init__(self, uuid_):
            ctypes.Structure.__init__(self)
            (
                self.Data1,
                self.Data2,
                self.Data3,
                self.Data4[0],
                self.Data4[1],
                rest
            ) = uuid_.fields
            for i in range(2, 8):
                self.Data4[i] = rest >> (8 - i - 1) * 8 & 0xff

    pathptr = ctypes.c_wchar_p()
    guid = GUID(DOCUMENTS_ID)
    if windll.shell32.SHGetKnownFolderPath(
        ctypes.byref(guid), 0, 0, ctypes.byref(pathptr)
    ):
        return os.path.join(os.environ["USERPROFILE"], "Documents")
    return pathptr.value


class ConfigModel(QtCore.QObject):
    paths_changed = QtCore.Signal()
    hotkeys_changed = QtCore.Signal()

    def __init__(self):
        super().__init__()
        self._default_dsr_save_info = None

        self._app_dir = platformdirs.user_data_dir(
            "FromSoftSaveManager", "FromSoftSaveManager"
        )
        self._config_path = os.path.join(
            self._app_dir, "config.json"
        )
        self._config_data = None
        self._save_info_by_id = {}
        self._load_config()

    def get_config_info(self) -> ConfigInfo:
        game_save_files = self._config_data.get("game_save_files", {})
        config_info = {}
        for game, config_key in (
            (Game.DSR, "dsr_save_path"),
            (Game.DS2_SOTFS, "ds2_save_path"),
            (Game.DS3, "ds3_save_path"),
            (Game.Sekiro, "sekiro_save_path"),
            (Game.ER, "er_save_path"),
        ):
            info = game_save_files.get(game)
            path = None
            if info:
                path = info.get("path")
            path_hint, success = self._get_default_save_path(game)
            default_path = None
            if success:
                default_path = path_hint
            config_info[config_key] = (
                ConfigSavePathInfo(path, path_hint, default_path)
            )

        hotkeys = self._config_data["hotkeys"]
        quicksave_hotkey = hotkeys["quicksave"]
        if quicksave_hotkey is not None:
            quicksave_hotkey = int_combination_to_qt(set(quicksave_hotkey))

        quickload_hotkey = hotkeys["quickload"]
        if quickload_hotkey is not None:
            quickload_hotkey = int_combination_to_qt(set(quickload_hotkey))
        config_info["quicksave_hotkey"] = quicksave_hotkey
        config_info["quickload_hotkey"] = quickload_hotkey

        return ConfigInfo(**config_info)

    def save_config_info(self, config_data: ConfigConfirmData):
        paths_changed = False
        game_save_files = self._config_data.setdefault("game_save_files", {})
        for game, filepath in (
            (Game.DSR, config_data.dsr_save_path),
            (Game.DS2_SOTFS, config_data.ds2_save_path),
            (Game.DS3, config_data.ds3_save_path),
            (Game.Sekiro, config_data.sekiro_save_path),
            (Game.ER, config_data.er_save_path),
        ):
            if filepath is None:
                continue
            info = game_save_files.setdefault(game, {})
            if info.get("path") != filepath:
                paths_changed = True
                if not info.get("save_id"):
                    info["save_id"] = uuid.uuid4().hex
                info["path"] = filepath
                self._save_info_by_id[info["save_id"]] = {
                    "path": filepath,
                    "game": game
                }

        hotkeys_changed = False
        hotkeys = self._config_data.setdefault("hotkeys", {})
        for key, hotkey in (
            ("quicksave", config_data.quicksave_hotkey),
            ("quickload", config_data.quickload_hotkey),
        ):
            if (
                hotkey is not None
                and hotkeys.get(key) != hotkey
            ):
                hotkeys_changed = True
                hotkeys[key] = list(qt_combination_to_int(
                    hotkey
                ))

        changed = paths_changed or hotkeys_changed
        if not changed:
            return

        self._save_config()
        if paths_changed:
            self.paths_changed.emit()
        if hotkeys_changed:
            self.hotkeys_changed.emit()

    def get_backup_dir_path(self, *args) -> str:
        return os.path.join(self._app_dir, "backups", *args)

    def get_save_items(self) -> list[SaveItem]:
        output = []
        game_save_files = self._config_data.get("game_save_files", {})
        for game in (
            Game.DSR, Game.DS2_SOTFS, Game.DS3, Game.Sekiro, Game.ER
        ):
            info = game_save_files.get(game)
            if info and info["path"]:
                output.append(
                    SaveItem(
                        game=game,
                        save_id=info["save_id"],
                        save_path=info["path"],
                    )
                )

        return output

    def get_save_info_by_id(self, save_id: str) -> dict | None:
        return self._save_info_by_id.get(save_id)

    def get_save_path_by_id(self, save_id: str) -> str | None:
        info = self.get_save_info_by_id(save_id)
        if info is None:
            return None
        return info["path"]

    def _get_default_save_path(self, game: Game) -> tuple[str, bool]:
        if game == Game.DSR:
            return self._get_default_dsr_save_path()
        elif game == Game.DS2_SOTFS:
            return self._get_default_ds2_save_path()
        elif game == Game.DS3:
            return self._get_default_ds3_save_path()
        elif game == Game.Sekiro:
            return self._get_default_sekiro_save_path()
        elif game == Game.ER:
            return self._get_default_er_save_path()
        return "", False

    def _get_default_dsr_save_path(self) -> tuple[str, bool]:
        if self._default_dsr_save_info is not None:
            return self._default_dsr_save_info
        save_dir = os.path.join(
            _get_windows_documents_dir(), "NBGI", "DARK SOULS REMASTERED",
        )
        self._default_dsr_save_info = (save_dir, False)
        if os.path.exists(save_dir):
            for name in os.listdir(save_dir):
                path = os.path.join(save_dir, name, "DRAKS0005.sl2")
                if os.path.exists(path):
                    self._default_dsr_save_info = (path, True)
                    break
        return self._default_dsr_save_info

    def _get_default_ds2_save_path(self) -> tuple[str, bool]:
        save_dir = os.path.join(os.getenv("APPDATA"), "DarkSoulsII")
        if os.path.exists(save_dir):
            for name in os.listdir(save_dir):
                path = os.path.join(save_dir, name, "DS2SOFS0000.sl2")
                if os.path.exists(path):
                    return path, True
        return save_dir, False

    def _get_default_ds3_save_path(self) -> tuple[str, bool]:
        save_dir = os.path.join(os.getenv("APPDATA"), "DarkSoulsIII")
        if os.path.exists(save_dir):
            for name in os.listdir(save_dir):
                path = os.path.join(save_dir, name, "DS30000.sl2")
                if os.path.exists(path):
                    return path, True
        return save_dir, False

    def _get_default_er_save_path(self) -> tuple[str, bool]:
        save_dir = os.path.join(os.getenv("APPDATA"), "EldenRing")
        if os.path.exists(save_dir):
            for name in os.listdir(save_dir):
                path = os.path.join(save_dir, name, "ER0000.sl2")
                if os.path.exists(path):
                    return path, True
        return save_dir, False

    def _get_default_sekiro_save_path(self) -> tuple[str, bool]:
        save_dir = os.path.join(os.getenv("APPDATA"), "Sekiro")
        if os.path.exists(save_dir):
            for name in os.listdir(save_dir):
                path = os.path.join(save_dir, name, "S0000.sl2")
                if os.path.exists(path):
                    return path, True
        return save_dir, False

    def _load_config(self):
        if self._config_data is not None:
            return

        config_data = {}
        if os.path.exists(self._config_path):
            try:
                with open(self._config_path, "r") as stream:
                    config_data = json.load(stream)
            except Exception:
                pass

        game_save_files = config_data.setdefault("game_save_files", {})
        for game in (
            Game.DSR, Game.DS2_SOTFS, Game.DS3, Game.Sekiro, Game.ER
        ):
            if game in game_save_files:
                continue
            default_path, success = self._get_default_save_path(game)
            if success:
                game_save_files[game] = {
                    "path": default_path,
                    "save_id": uuid.uuid4().hex,
                }

        info_by_id = {}
        for key, info in game_save_files.items():
            save_id = info.get("save_id")
            path = info.get("path")
            if save_id:
                info_by_id[save_id] = {"path": path, "game": key}

        hotkeys = config_data.setdefault("hotkeys", {})
        if "quicksave" not in hotkeys:
            hotkeys["quicksave"] = list(qt_combination_to_int(
                QtCore.QKeyCombination(QtCore.Qt.Key_F5)
            ))

        if "quickload" not in hotkeys:
            hotkeys["quickload"] = list(qt_combination_to_int(
                QtCore.QKeyCombination(QtCore.Qt.Key_F8)
            ))

        self._save_info_by_id = info_by_id
        self._config_data = config_data

    def _get_mapped_key(self, key: QtCore.Qt.Key) -> int | None:
        # for key, value in KEYS_MAPPING.items():
        #     if value == key:
        #         return key
        return None

    def _save_config(self):
        config_dir = os.path.dirname(self._config_path)
        os.makedirs(config_dir, exist_ok=True)
        # Create json string before writing to file
        json_string = json.dumps(self._config_data, indent=4)
        with open(self._config_path, "w") as stream:
            stream.write(json_string)