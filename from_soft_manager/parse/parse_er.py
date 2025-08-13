import struct
from dataclasses import dataclass
from typing import Optional

from .structures import Game, SL2File, BND4Entry


@dataclass
class InventoryItem:
    item_id: int
    quantity: int


@dataclass
class GaItem:
    handle: int
    item_id: int
    unk2: int
    unk3: int
    aow_handle: int
    unk4: int


@dataclass
class EldenRingCharacter:
    index: int
    ver: int
    map_id: bytes
    _unknown_1: bytes
    ga_items: list[InventoryItem]
    # Char info
    _unknown_2: bytes
    _unknown_3: bytes
    hp_current: int
    hp_max: int
    hp_base: int
    fp_current: int
    fp_max: int
    fp_base: int
    _unknown_4: bytes
    stamina_current: int
    stamina_max: int
    stamina_base: int
    _unknown_5: bytes
    vigor: int
    mind: int
    endurance: int
    strength: int
    dexterity: int
    intelligence: int
    faith: int
    arcane: int
    _unknown_6: bytes
    _unknown_7: bytes
    _unknown_8: bytes
    level: int
    runes: int
    runes_memory: int
    name: str


@dataclass
class EldenRingSaveFile:
    sl2_file: SL2File
    characters: list[Optional[EldenRingCharacter]]
    menu_enty: BND4Entry
    side_car_enty: BND4Entry


def character_from_entry(
    index: int, entry: BND4Entry
) -> Optional[EldenRingCharacter]:
    version = int.from_bytes(entry.content[0:4], "little")
    if version == 0:
        return None

    map_id = entry.content[4:8]
    _unknown_1 = entry.content[8:32]
    ga_items = []
    idx = 32
    for item_idx in range(5120):
        (
            handle,
            item_id,
        ) = struct.unpack(
            "<II",
            entry.content[idx:idx+8]
        )
        idx += 8

        ga_unknown_1 = 0
        ga_unknown_2 = 0
        aow_handle = 0
        ga_unknown_3 = 0
        if item_id == 0:
            pass
        elif item_id & 0xf0000000 == 0:
            (
                ga_unknown_1,
                ga_unknown_2,
                aow_handle,
                ga_unknown_3,
            ) = struct.unpack(
                "<IIIc",
                entry.content[idx:idx+13]
            )
            idx += 13
        elif item_id & 0xf0000000 == 0x10000000:
            (
                ga_unknown_1,
                ga_unknown_2,
            ) = struct.unpack(
                "<II",
                entry.content[idx:idx+8]
            )
            idx += 8

        ga_items.append(GaItem(
            handle=handle,
            item_id=item_id,
            unk2=ga_unknown_1,
            unk3=ga_unknown_2,
            aow_handle=aow_handle,
            unk4=ga_unknown_3
        ))

    # Char info
    (
        _unknown_2,
        _unknown_3,
        hp_current,
        hp_max,
        hp_base,
        fp_current,
        fp_max,
        fp_base,
        _unknown_4,
        stamina_current,
        stamina_max,
        stamina_base,
        _unknown_5,
        vigor,
        mind,
        endurance,
        strength,
        dexterity,
        intelligence,
        faith,
        arcane,
        _unknown_6,
        _unknown_7,
        _unknown_8,
        level,
        runes,
        runes_memory,
    ) = struct.unpack(
        "<IIIIIIIIIIIIIIIIIIIIIIIIIII",
        entry.content[idx:idx+108]
    )
    idx += 108

    _unknown_9 = entry.content[idx:idx+40]
    idx += 40

    b_name = entry.content[idx:idx+32]
    idx += 32
    while b_name.endswith(b"\x00\x00"):
        b_name = b_name[:-2]
    name = b_name.decode("utf-16")
    return EldenRingCharacter(
        index,
        version,
        map_id,
        _unknown_1,
        ga_items,
        _unknown_2,
        _unknown_3,
        hp_current,
        hp_max,
        hp_base,
        fp_current,
        fp_max,
        fp_base,
        _unknown_4,
        stamina_current,
        stamina_max,
        stamina_base,
        _unknown_5,
        vigor,
        mind,
        endurance,
        strength,
        dexterity,
        intelligence,
        faith,
        arcane,
        _unknown_6,
        _unknown_7,
        _unknown_8,
        level,
        runes,
        runes_memory,
        name,
    )

def parse_er_file(sl2_file: SL2File) -> EldenRingSaveFile:
    if sl2_file.game != Game.ER:
        raise ValueError(
            f"Expected ER save file, got {sl2_file.game}"
        )

    menu_entry = sl2_file.entries[10]
    steam_id, = struct.unpack("<q", menu_entry.content[4:12])
    # occupied_slots = struct.unpack(
    #     "<bbbbbbbbbb", menu_entry.content[4244:4254]
    # )
    characters = []
    for idx in range(10):
        # char = None
        # if occupied_slots[idx] == 1:
        char = character_from_entry(
            idx, sl2_file.entries[idx]
        )

        characters.append(char)

    return EldenRingSaveFile(
        sl2_file,
        characters,
        sl2_file.entries[10],
        sl2_file.entries[11],
    )
