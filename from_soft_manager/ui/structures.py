from typing import Optional
from dataclasses import dataclass

from from_soft_manager.parse import Game


@dataclass
class SaveItem:
    game: Game
    save_id: str
    save_path: str


@dataclass
class CharactersInfo:
    save_id: str
    characters: list
    path: Optional[str]
    error: Optional[str] = None


@dataclass
class ConfigInfo:
    dsr_save_path: str
    dsr_save_path_hint: str
    dsr_save_path_default: Optional[str]


@dataclass
class ConfigConfirmData:
    dsr_save_path: Optional[str]
