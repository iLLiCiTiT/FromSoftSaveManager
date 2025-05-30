from abc import ABC
from enum import StrEnum
from dataclasses import dataclass


class Game(StrEnum):
    DSR = "DSR"
    DS2 = "DS2"
    DS3 = "DS3"
    ER = "ER"


@dataclass
class BND4Header:
    bnd_vers: bytes
    unknown_1: int
    files_count: int
    unknown_2: int
    sig: int
    entry_header_size: int
    data_offset: int
    is_utf16: bool
    unknown_3: bytes


@dataclass
class BND4EntryHeader:
    padding: int
    entry_size: int
    entry_data_offset: int
    entry_name_offset: int
    entry_footer_length: int


@dataclass
class BND4Entry:
    header: BND4EntryHeader
    name_b: bytes
    name: str
    content: bytes


@dataclass
class SL2File:
    game: Game
    filepath: str
    header: BND4Header
    entries: list[BND4Entry]


class ParsedFile(ABC):
    def __init__(self, game: Game, sl2_file: SL2File):
        self.game = game
        self.sl2_file = sl2_file

    def __repr__(self):
        return f"{self.game} ParsedFile"
