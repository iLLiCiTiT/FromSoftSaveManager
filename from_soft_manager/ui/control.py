import os
import logging
import platform
import shutil
import uuid
import json
import time
from enum import StrEnum
from datetime import datetime

import arrow
from PySide6 import QtCore

from from_soft_manager.parse import (
    Game,
    parse_sl2_file,
    parse_dsr_file,
    DSRSaveFile,
    SL2File,
)
from .structures import (
    SaveItem,
    CharactersInfo,
    ConfigInfo,
    ConfigConfirmData,
)
from .models import ConfigModel
from .keys import keys_are_pressed

NOT_SET = object()


class SaveType(StrEnum):
    quicksave = "quicksave"
    autosave = "autosave"
    manualsave = "manualsave"


class BackgroundThread(QtCore.QThread):
    quicksave_requested = QtCore.Signal()
    quickload_requested = QtCore.Signal()

    def __init__(self, controller):
        super().__init__(controller)
        self._controller = controller

    def run(self):
        if platform.system().lower() != "windows":
            return
        VK_F5 = 0x74  # F5 key
        VK_F8 = 0x77  # F8 key
        quicksave_pressed = False
        quickload_pressed = False
        sleep_time = 10
        while self.isRunning():
            if keys_are_pressed({VK_F5}):
                if not quicksave_pressed:
                    quicksave_pressed = True
                    self.quicksave_requested.emit()
                self.msleep(sleep_time)
                continue
            quicksave_pressed = False

            if keys_are_pressed({VK_F8}):
                if not quickload_pressed:
                    quickload_pressed = True
                    self.quickload_requested.emit()

                self.msleep(sleep_time)
                continue
            quickload_pressed = False
            self.msleep(sleep_time)


def create_backup_metadata(
    game: Game,
    save_type: SaveType,
    filenames: list[str],
    label: str | None = None,
) -> dict:
    return {
        "id": uuid.uuid4().hex,
        "game": game,
        "save_type": save_type.value,
        "label": label,
        "filenames": filenames,
        "datetime": arrow.utcnow().format(),
        "epoch": time.time(),
    }


def index_existing_path(path: str) -> str:
    if not os.path.exists(path):
        return path

    base = path
    ext = ""
    if os.path.isfile(path):
        base, ext = os.path.splitext(path)
    idx = 1
    while True:
        new_path = f"{base}_{idx}{ext}"
        if not os.path.exists(new_path):
            return new_path
        idx += 1


class Controller(QtCore.QObject):
    paths_changed = QtCore.Signal()

    def __init__(self):
        super().__init__()
        self._log = logging.getLogger("Controller")

        config_model = ConfigModel()
        background_thread = BackgroundThread(self)

        config_model.paths_changed.connect(self.paths_changed)
        background_thread.quicksave_requested.connect(
            self._on_quicksave_request
        )
        background_thread.quickload_requested.connect(
            self._on_quickload_request
        )
        background_thread.start()

        self._config_model = config_model
        self._background_thread = background_thread
        self._current_save_id: str | None = None

    def get_config_info(self) -> ConfigInfo:
        return self._config_model.get_config_info()

    def save_config_info(self, config_data: ConfigConfirmData):
        return self._config_model.save_config_info(config_data)

    def get_save_items(self) -> list[SaveItem]:
        return self._config_model.get_save_items()

    def get_dsr_characters(self, save_id: str) -> CharactersInfo:
        path = self._config_model.get_save_path_by_id(save_id)
        info = CharactersInfo(
            save_id,
            [],
            path,
        )
        self._fill_dsr_characters(info)
        return info

    def set_current_save_id(self, save_id: str | None):
        self._current_save_id = save_id

    def _on_quicksave_request(self):
        # TODO warn user if quicksave failed+++.
        if self._current_save_id is None:
            self._log.warning("No current save ID set for quicksave.")
            return
        save_info = self._config_model.get_save_info_by_id(
            self._current_save_id
        )
        src_path = save_info["path"]
        if src_path is None:
            self._log.warning(
                f"No save path found for current save ID:"
                f" {self._current_save_id}"
            )
            return
        if not os.path.exists(src_path):
            self._log.warning(
                f"Save file does not exist: {src_path}"
            )
            return

        backup_dir = self._config_model.get_backup_dir_path(
            save_info["game"], datetime.now().strftime("%Y%m%d_%H%M%S")
        )
        backup_dir = index_existing_path(backup_dir)

        os.makedirs(backup_dir, exist_ok=True)
        src_dir = os.path.dirname(src_path)
        filenames = [
            filename
            for filename in os.listdir(src_dir)
            if os.path.isfile(os.path.join(src_dir, filename))
        ]
        for filename in filenames:
            shutil.copy2(os.path.join(src_dir, filename), backup_dir)

        metadata_path = os.path.join(backup_dir, "metadata.json")
        metadata = create_backup_metadata(
            save_info["game"],
            SaveType.quicksave,
            filenames
        )
        with open(metadata_path, "w") as stream:
            json.dump(metadata, stream)

    def _on_quickload_request(self):
        print("Quickload requested")

    def _fill_dsr_characters(self, info: CharactersInfo):
        if info.path is None:
            info.error = "Save file path is not set."
            return

        if not os.path.exists(info.path):
            info.error = "Save file does not exist."
            return

        try:
            parsed_file: SL2File = parse_sl2_file(info.path)
        except Exception:
            self._log.warning(
                "Failed to parse DSR save file", exc_info=True
            )
            info.error = "Failed to parse save file."
            return

        if parsed_file.game != Game.DSR:
            info.error = (
                "Not Dark Souls Remastered save"
                f" file but '{parsed_file.game}'."
            )
            return

        dsr_file: DSRSaveFile = parse_dsr_file(parsed_file)
        info.characters = dsr_file.characters
