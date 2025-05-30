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
        config_info = []
        for game in (Game.DSR, Game.DS2_SOTFS, Game.DS3, Game.ER):
            info = game_save_files.get(game)
            path = None
            if info:
                path = info.get("path")
            path_hint, success = self._get_default_save_path(game)
            default_path = None
            if success:
                default_path = path_hint
            config_info.append(
                ConfigSavePathInfo(path, path_hint, default_path)
            )

        return ConfigInfo(*config_info)

    def save_config_info(self, config_data: ConfigConfirmData):
        dsr_save_path = config_data.dsr_save_path

        game_save_files = self._config_data.setdefault("game_save_files", {})
        dsr_info = game_save_files.setdefault(Game.DSR, {})
        changed = False
        if dsr_info.get("path") != dsr_save_path:
            changed = True
            if not dsr_info.get("save_id"):
                dsr_info["save_id"] = uuid.uuid4().hex
            dsr_info["path"] = dsr_save_path
            self._save_info_by_id[dsr_info["save_id"]] = {
                "path": dsr_save_path,
                "game": Game.DSR
            }

        if changed:
            self._save_config()
            self.paths_changed.emit()

    def get_backup_dir_path(self, *args) -> str:
        return os.path.join(self._app_dir, "backups", *args)

    def get_save_items(self) -> list[SaveItem]:
        output = []
        game_save_files = self._config_data.get("game_save_files", {})
        for game in (Game.DSR, Game.DS2_SOTFS, Game.DS3, Game.ER):
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
        info = self._save_info_by_id.get(save_id)
        if info:
            return info
        return {
            "path": None,
            "game": None,
        }

    def get_save_path_by_id(self, save_id: str) -> str | None:
        return self.get_save_info_by_id(save_id)["path"]

    def _get_default_save_path(self, game: Game) -> tuple[str, bool]:
        if game == Game.DSR:
            return self._get_default_dsr_save_path()
        elif game == Game.DS2_SOTFS:
            return self._get_default_ds2_save_path()
        elif game == Game.DS3:
            return self._get_default_ds3_save_path()
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
        for game in (Game.DSR, Game.DS2_SOTFS, Game.DS3, Game.ER):
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

        self._save_info_by_id = info_by_id
        self._config_data = config_data

    def _save_config(self):
        config_dir = os.path.dirname(self._config_path)
        os.makedirs(config_dir, exist_ok=True)
        with open(self._config_path, "w") as stream:
            json.dump(self._config_data, stream, indent=4)