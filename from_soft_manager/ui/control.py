import os
import logging

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

NOT_SET = object()


class Controller(QtCore.QObject):
    paths_changed = QtCore.Signal()

    def __init__(self):
        super().__init__()
        self._log = logging.getLogger("Controller")

        config_model = ConfigModel()

        config_model.paths_changed.connect(self.paths_changed)

        self._config_model = config_model

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
