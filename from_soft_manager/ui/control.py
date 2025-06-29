import os
import logging
import platform
import shutil
import subprocess
import uuid
import json
import time
from datetime import datetime
from typing import Any

import arrow
from PySide6 import QtCore, QtWidgets, QtMultimedia

from from_soft_manager.parse import (
    Game,
    parse_sl2_file,
    SL2File,
    parse_dsr_file,
    DSRSaveFile,
    parse_ds3_file,
    DS3SaveFile,
)
from .structures import (
    SaveItem,
    BackupType,
    BackupInfo,
    CharactersInfo,
    ConfigInfo,
    ConfigConfirmData,
)
from .models import ConfigModel
from .resources import get_save_sound_path, get_load_sound_path
from .keys import keys_are_pressed, qt_combination_to_int

NOT_SET = object()


class SaveChangesThread(QtCore.QThread):
    save_file_changed = QtCore.Signal(str)

    def __init__(self, controller):
        super().__init__()
        self._controller = controller
        self._save_paths_by_game = {}

    def update_save_paths(self):
        config: ConfigInfo = self._controller.get_config_info()
        save_paths_by_game = {}
        for game, path in (
            (Game.DSR, config.dsr_save_path.save_path),
            (Game.DS2_SOTFS, config.ds2_save_path.save_path),
            (Game.DS3, config.ds3_save_path.save_path),
            (Game.Sekiro, config.sekiro_save_path.save_path),
            (Game.ER, config.er_save_path.save_path),
        ):
            if path:
                save_paths_by_game[game] = path

        self._save_paths_by_game = save_paths_by_game

    def run(self):
        self.update_save_paths()
        self._is_running = True
        last_changed_by_game = {}
        for game, filepath in self._save_paths_by_game.items():
            last_changed_by_game[game] = None
            if os.path.exists(filepath):
                last_changed_by_game[game] = os.stat(filepath).st_mtime

        while self._is_running:
            for game, save_path in self._save_paths_by_game.items():
                last_change = last_changed_by_game[game]
                changed = None
                if os.path.exists(save_path):
                    changed = os.stat(save_path).st_mtime
                if changed == last_change:
                    continue

                last_changed_by_game[game] = changed

                self.save_file_changed.emit(game)

            self.msleep(1000)

    def stop(self):
        self._is_running = False


class HotkeysThread(QtCore.QThread):
    quicksave_requested = QtCore.Signal()
    quickload_requested = QtCore.Signal()

    def __init__(self, controller):
        super().__init__(controller)
        self._controller = controller
        self._is_running = False
        self._hotkeys_changed = False
        self._quicksave_hotkey = None
        self._quickload_hotkey = None

    def update_hotkeys(self):
        config: ConfigInfo = self._controller.get_config_info()
        self._quicksave_hotkey = qt_combination_to_int(
            config.quicksave_hotkey
        )
        self._quickload_hotkey = qt_combination_to_int(
            config.quickload_hotkey
        )
        self._hotkeys_changed = True

    def run(self):
        if platform.system().lower() != "windows":
            return
        self.update_hotkeys()
        self._is_running = True
        quicksave_pressed = False
        quickload_pressed = False
        while self._is_running:
            trigger_save = False
            trigger_load = False
            if (
                not self._quicksave_hotkey
                or not keys_are_pressed(self._quicksave_hotkey)
            ):
                quicksave_pressed = False
            elif not quicksave_pressed:
                quicksave_pressed = True
                trigger_save = True

            if (
                not self._quickload_hotkey
                or not keys_are_pressed(self._quickload_hotkey)
            ):
                quickload_pressed = False
            elif not quickload_pressed:
                quickload_pressed = True
                trigger_load = True

            if self._hotkeys_changed:
                # For some reason first loop after hotkeys change always
                #   thinks that hotkeys are pressed.
                self._hotkeys_changed = False
            elif trigger_save:
                self.quicksave_requested.emit()
            elif trigger_load:
                self.quickload_requested.emit()

            self.msleep(10)

    def stop(self):
        self._is_running = False


def create_backup_metadata(
    game: Game,
    backup_type: BackupType,
    filenames: list[str],
    label: str | None = None,
) -> dict:
    return {
        "id": uuid.uuid4().hex,
        "game": game,
        "backup_type": backup_type,
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


class AutoBackuphandler(QtCore.QObject):
    def __init__(self, controller):
        super().__init__(controller)

        autobackup_timer = QtCore.QTimer(self)
        autobackup_timer.setSingleShot(False)
        autobackup_timer.setInterval(1000)

        autobackup_timer.timeout.connect(self._on_timer)
        controller.game_save_changed.connect(self._on_game_save_changed)
        controller.paths_changed.connect(self._on_paths_changed)
        controller.autobackup_config_changed.connect(
            self._on_autobackup_config_changed
        )

        autobackup_timer.start()

        self._controller = controller
        self._autobackup_timer = autobackup_timer
        self._enabled = None
        self._frequency = None
        self._max_backups = None
        self._changed_games = set()
        self._paths_by_game = {}

    def _on_timer(self):
        if self._enabled is None:
            self._init_config_info()

        if not self._enabled:
            self._autobackup_timer.stop()
            return

        if not self._changed_games:
            return

        for game in set(self._changed_games):
            filepath = self._paths_by_game[game]
            if not filepath or not os.path.exists(filepath):
                continue
            self._controller.create_autobackup(game, filepath)
            self._controller.cleanup_autobackups(game, self._max_backups)
            self._changed_games.discard(game)

    def _on_game_save_changed(self, game: Game):
        self._changed_games.add(game)

    def _on_paths_changed(self):
        config_info: ConfigInfo = self._controller.get_config_info()
        self._fill_paths(config_info)

    def _on_autobackup_config_changed(self):
        config_info: ConfigInfo = self._controller.get_config_info()
        self._fill_autobackup(config_info)

    def _init_config_info(self):
        config_info: ConfigInfo = self._controller.get_config_info()
        self._fill_autobackup(config_info)
        self._fill_paths(config_info)

    def _fill_autobackup(self, config_info):
        self._enabled = config_info.autobackup_enabled
        self._frequency = config_info.autobackup_frequency
        self._max_backups = config_info.max_autobackups

        self._autobackup_timer.setInterval(self._frequency * 1000)
        if self._enabled:
            self._autobackup_timer.start()

    def _fill_paths(self, config_info: ConfigInfo):
        self._paths_by_game = {
            Game.DSR: config_info.dsr_save_path.save_path,
            Game.DS2_SOTFS: config_info.ds2_save_path.save_path,
            Game.DS3: config_info.ds3_save_path.save_path,
            Game.Sekiro: config_info.sekiro_save_path.save_path,
            Game.ER: config_info.er_save_path.save_path,
        }
        self._changed_games = set()


class Controller(QtCore.QObject):
    paths_changed = QtCore.Signal()
    game_save_changed = QtCore.Signal(str)
    save_id_changed = QtCore.Signal(str)
    hotkeys_changed = QtCore.Signal()
    autobackup_config_changed = QtCore.Signal()

    def __init__(self):
        super().__init__()
        self._log = logging.getLogger("Controller")

        save_sound = QtMultimedia.QSoundEffect()
        save_sound.setSource(QtCore.QUrl.fromLocalFile(get_save_sound_path()))
        save_sound.setVolume(0.5)

        load_sound = QtMultimedia.QSoundEffect()
        load_sound.setSource(QtCore.QUrl.fromLocalFile(get_load_sound_path()))
        load_sound.setVolume(0.5)

        config_model = ConfigModel()

        background_thread = HotkeysThread(self)
        save_changes_thread = SaveChangesThread(self)
        autobackup_handler = AutoBackuphandler(self)

        config_model.paths_changed.connect(self.paths_changed)
        config_model.hotkeys_changed.connect(self._on_hotkeys_change)
        config_model.autobackup_changed.connect(
            self.autobackup_config_changed
        )
        background_thread.quicksave_requested.connect(
            self._on_quicksave_request
        )
        background_thread.quickload_requested.connect(
            self._on_quickload_request
        )
        background_thread.start()

        save_changes_thread.save_file_changed.connect(
            self._game_save_changed
        )
        save_changes_thread.start()

        app = QtWidgets.QApplication.instance()
        app.aboutToQuit.connect(self._on_exit)

        self._save_sound = save_sound
        self._load_sound = load_sound
        self._config_model = config_model
        self._background_thread = background_thread
        self._save_changes_thread = save_changes_thread
        self._autobackup_handler = autobackup_handler
        self._current_save_id: str | None = None

    def _on_exit(self):
        if self._background_thread.isRunning():
            self._background_thread.stop()
            self._background_thread.wait()

        if self._save_changes_thread.isRunning():
            self._save_changes_thread.stop()
            self._save_changes_thread.wait()

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

    def get_ds3_characters(self, save_id: str) -> CharactersInfo:
        path = self._config_model.get_save_path_by_id(save_id)
        info = CharactersInfo(
            save_id,
            [],
            path,
        )
        self._fill_ds3_characters(info)
        return info

    def set_current_save_id(self, save_id: str | None):
        self._current_save_id = save_id

    def create_manual_backup(self, label: str):
        # TODO warn user if save failed.
        label = label.strip()
        if not label:
            label = datetime.now().strftime("%Y%m%d - %H%M%S")
        self._backup_current_save(BackupType.manualsave, label)

    def get_backup_items(self) -> list[BackupInfo]:
        if not self._current_save_id:
            return []
        save_info = self._config_model.get_save_info_by_id(
            self._current_save_id
        )
        return self._get_backups_by_game(save_info["game"])

    def open_backup_dir(self):
        if platform.system().lower() != "windows":
            self._log.warning(
                "Open backup directory is supported only on Windows."
            )
            return

        if not self._current_save_id:
            return

        save_info = self._config_model.get_save_info_by_id(
            self._current_save_id
        )

        game_backup_dir = self._config_model.get_backup_dir_path(
            save_info["game"]
        )
        if not os.path.exists(game_backup_dir):
            # Nothing happens? Should be created? Or return reason why?
            self._log.info("Backup directory does not exist yet.")
            return
        os.system(subprocess.list2cmdline(["start", game_backup_dir]))

    def restore_by_backup_id(self, backup_id: str):
        save_id = self._current_save_id
        save_info = self._config_model.get_save_info_by_id(save_id)
        game = save_info["game"]

        backup_info = None
        for backup_dir, metadata in self._collect_game_save_backups(game):
            if metadata["id"] == backup_id:
                backup_info = backup_dir, metadata
                break

        # TODO handle failure
        if backup_info is None:
            return
        backup_dir, metadata = backup_info
        self._restore_backup_save(backup_dir, metadata, save_id, save_info)

    def delete_backups(self, backup_ids: set[str]):
        if not backup_ids:
            return

        save_id = self._current_save_id
        save_info = self._config_model.get_save_info_by_id(save_id)
        game = save_info["game"]

        for backup_dir, metadata in self._collect_game_save_backups(game):
            if metadata["id"] in backup_ids:
                shutil.rmtree(backup_dir)

    def create_autobackup(self, game: Game, filepath: str):
        save_id = self._config_model.get_save_id_by_game(game)
        if save_id is None:
            self._log.warning(
                f"No save ID found for game: {game}."
                f" Failed to create autobackup."
            )
            return

        self._backup_save(save_id, BackupType.autosave)

    def cleanup_autobackups(self, game: Game, max_backups: int):
        if max_backups < 1:
            return

        backups = self._get_backups_by_game(game)
        filtered_backups = [
            backup
            for backup in backups
            if backup.backup_type == BackupType.autosave
        ]
        filtered_backups.sort(key=lambda backup: backup.datetime)
        while len(filtered_backups) > max_backups:
            backup = filtered_backups.pop(0)
            shutil.rmtree(backup.backup_dir)

    def _get_backups_by_game(self, game: Game) -> list[BackupInfo]:
        backups = []
        for backup_dir, metadata in self._collect_game_save_backups(game):
            backup_info = BackupInfo(
                backup_dir,
                metadata["id"],
                metadata["backup_type"],
                arrow.get(metadata["datetime"]),
                metadata.get("label"),
            )
            backups.append(backup_info)
        return backups

    def _on_hotkeys_change(self):
        self._background_thread.update_hotkeys()
        self.hotkeys_changed.emit()

    def _on_quicksave_request(self):
        # TODO warn user if quicksave failed.
        self._backup_current_save(BackupType.quicksave)

    def _on_quickload_request(self):
        if self._current_save_id is None:
            self._log.warning("No current save ID set for quicksave.")
            return
        save_id = self._current_save_id
        save_info = self._config_model.get_save_info_by_id(save_id)
        dst_path = save_info["path"]
        if not dst_path:
            self._log.warning(
                f"No save path found for current save ID:"
                f" {self._current_save_id}"
            )
            return

        backup_dir, metadata = self._find_last_quicksave(save_info["game"])
        if not backup_dir:
            self._log.info(
                f"No quicksave found for game: {save_info['game']}"
            )
            return

        self._restore_backup_save(backup_dir, metadata, save_id, save_info)

    def _restore_backup_save(
        self,
        backup_dir: str,
        metadata: dict[str, Any],
        save_id: str,
        save_info: dict[str, Any],
    ):
        dst_path = save_info["path"]
        filenames = metadata.get("filenames")
        if not filenames:
            self._log.warning(
                f"No filenames found in metadata for quicksave: {backup_dir}"
            )
            return

        dst_dir = os.path.dirname(dst_path)
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir, exist_ok=True)

        # Create backup of existing files
        tmp_dir = index_existing_path(os.path.join(dst_dir, ".backup"))
        os.makedirs(tmp_dir, exist_ok=True)

        bckup_mapping = []
        for filename in tuple(os.listdir(dst_dir)):
            src_path = os.path.join(dst_dir, filename)
            if not os.path.isfile(src_path):
                continue
            dsr_path = os.path.join(tmp_dir, filename)
            bckup_mapping.append((src_path, dsr_path))

        for src_path, dsr_path in bckup_mapping:
            os.rename(src_path, dsr_path)

        failed = False
        copied_filenames = []
        try:
            for filename in filenames:
                src_path = os.path.join(backup_dir, filename)
                if not os.path.exists(src_path):
                    failed = True
                    self._log.warning(
                        f"File does not exist in quicksave backup: {src_path}"
                    )
                    break
                dst_path = os.path.join(dst_dir, filename)
                shutil.copy2(src_path, dst_path)
                copied_filenames.append(filename)

        except Exception:
            self._log.error(
                "Failed to copy files from quicksave backup.",
                exc_info=True
            )
            failed = True
        finally:
            if failed:
                for filename in copied_filenames:
                    path = os.path.join(dst_dir, filename)
                    os.remove(path)
                # Restore backup
                for src_path, dsr_path in bckup_mapping:
                    os.rename(dsr_path, src_path)

            shutil.rmtree(tmp_dir)

        self._load_sound.play()

        self.save_id_changed.emit(save_id)

    def _backup_current_save(
        self, backup_type: BackupType, label: str | None = None
    ) -> dict | None:
        # TODO warn user if quicksave failed.
        if self._current_save_id is None:
            self._log.warning("No current save ID set for quicksave.")
            return
        self._backup_save(self._current_save_id, backup_type, label)

    def _backup_save(
        self, save_id: str, backup_type: BackupType, label: str | None = None
    ):
        save_info = self._config_model.get_save_info_by_id(save_id)
        src_path = save_info["path"]
        if src_path is None:
            self._log.warning(
                f"No save path found for save ID: {save_id}"
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
            backup_type,
            filenames,
            label,
        )
        with open(metadata_path, "w") as stream:
            json.dump(metadata, stream)

        if backup_type != BackupType.autosave:
            self._save_sound.play()

    def _collect_game_save_backups(self, game: Game) -> list[tuple[str, dict]]:
        all_metadata = []
        game_backup_dir = self._config_model.get_backup_dir_path(game)
        if not os.path.exists(game_backup_dir):
            return all_metadata

        for filename in os.listdir(game_backup_dir):
            backup_dir = os.path.join(game_backup_dir, filename)
            metadata_path = os.path.join(backup_dir, "metadata.json")
            if not os.path.exists(metadata_path):
                continue
            try:
                with open(metadata_path, "r") as stream:
                    metadata = json.load(stream)
            except Exception:
                self._log.warning(
                    f"Failed to read metadata from {metadata_path}",
                    exc_info=True
                )
                continue

            if "save_type" in metadata:
                metadata["backup_type"] = metadata.pop("save_type")

            all_metadata.append((backup_dir, metadata))
        return all_metadata

    def _find_last_quicksave(self, game: Game) -> tuple[str | None, dict]:
        filtered_metadata = []
        for item in self._collect_game_save_backups(game):
            backup_dir, metadata = item
            if not metadata.get("epoch", 0):
                continue

            if metadata.get("backup_type") == BackupType.quicksave:
                filtered_metadata.append(item)

        if not filtered_metadata:
            return None, {}
        filtered_metadata.sort(key=lambda item: item[1]["epoch"])
        return filtered_metadata[-1]

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

    def _fill_ds3_characters(self, info: CharactersInfo):
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

        if parsed_file.game != Game.DS3:
            info.error = (
                "Not Dark Souls Remastered save"
                f" file but '{parsed_file.game}'."
            )
            return

        dsr_file: DS3SaveFile = parse_ds3_file(parsed_file)
        info.characters = dsr_file.characters

    def _game_save_changed(self, game: Game):
        save_id = self._config_model.get_save_id_by_game(game)
        if save_id is not None:
            self.game_save_changed.emit(game)
            self.save_id_changed.emit(save_id)