import os
import logging
from typing import Optional

from from_soft_manager.parse import (
    Game,
    parse_sl2_file,
    parse_dsr_file,
    DSRSaveFile,
)

NOT_SET = object()


class Controller:
    def __init__(self):
        self._log = logging.getLogger("Controller")

        self._dsr_file = NOT_SET

        # Implement settings for paths
        self._dsr_save_path = os.path.join(
            os.path.join(os.environ["USERPROFILE"]),
            "Documents",  # NOTE 'Documents' can vary by language settings
            "NBGI", "DARK SOULS REMASTERED", "52882570", "DRAKS0005.sl2"
        )

    def get_dsr_chars(self):
        parsed_file = self._get_dsr_file()
        if parsed_file is None:
            return []
        return parsed_file.characters

    def _get_dsr_file(self) -> Optional[DSRSaveFile]:
        if self._dsr_file is not NOT_SET:
            return self._dsr_file

        self._dsr_file = None
        if not os.path.exists(self._dsr_save_path):
            self._log.debug(
                "DSR save file does not exist at path: %s",
                self._dsr_save_path
            )
            return None

        try:
            parsed_file = parse_sl2_file(self._dsr_save_path)
        except Exception:
            self._log.warning(
                "Failed to parse DSR save file", exc_info=True
            )
            return None

        if parsed_file.game != Game.DSR:
            self._log.warning(
                "Parsed file is not a DSR save file: %s",
                parsed_file.game
            )
            return None

        self._dsr_file = parse_dsr_file(parsed_file)
        return self._dsr_file
