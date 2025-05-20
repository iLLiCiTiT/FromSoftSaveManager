from enum import Enum
from dataclasses import dataclass


class Game(Enum):
    DSR = 1
    DS2 = 2
    DS3 = 3
    ER = 4


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
    name: bytes
    content: bytes


@dataclass
class SL2File:
    game: Game
    header: BND4Header
    entries: list[BND4Entry]
