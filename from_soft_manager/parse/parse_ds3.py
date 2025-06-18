import struct
from dataclasses import dataclass
from typing import Optional

from .structures import Game, SL2File, BND4Entry

DS3_KEY = b"\xfd\x46\x4d\x69\x5e\x69\xa3\x9a\x10\xe3\x19\xa7\xac\xe8\xb7\xfa"


@dataclass
class DS3Character:
    index: int
    name: str

    hp_current: int
    hp_max: int
    hp_base: int
    fp_current: int
    fp_max: int
    fp_base: int
    stamina_current: int
    stamina_max: int
    stamina_base: int

    level: int
    souls: int
    vigor: int
    attunement: int
    endurance: int
    strength: int
    dexterity: int
    intelligence: int
    faith: int
    luck: int
    vitality: int


@dataclass
class DS3SaveFile:
    sl2_file: SL2File
    characters: list[Optional[DS3Character]]
    menu_enty: BND4Entry
    side_car_enty: BND4Entry


def character_from_entry(
    char_idx: int, menu_entry: BND4Entry, entry: BND4Entry
) -> DS3Character:
    # Empty slot that was never used
    # - character slot keeps data of the last character
    # if entry.content[0] != 98:
    #     return None

    # Don't know what is [0:108] and don't know what is after
    _SKIP_VALUE = b"\x00\x00\x00\x00\xff\xff\xff\xff"
    idx = 108
    for _ in range(6144):
        if entry.content[idx:idx + 8] == _SKIP_VALUE:
            idx += 8
        else:
            idx += 60
    char_start_idx = idx + 8
    menu_idx = 4254 + (554 * char_idx)
    slot_data = menu_entry.content[menu_idx:menu_idx + 554]
    b_name = slot_data[:32]

    while b_name.endswith(b"\x00\x00"):
        b_name = b_name[:-2]
    name = b_name.decode("utf-16")
    (
        level, playtime
    ) = struct.unpack("<II", slot_data[34:42])

    (
        hp_current,
        hp_max,
        hp_base,
        fp_current,
        fp_max,
        fp_base,
        _,  # always 0
        stamina_current,
        stamina_max,
        stamina_base,
        _,  # always 0
        vigor,
        attunement,
        endurance,
        strength,
        dexterity,
        intelligence,
        faith,
        luck,
        _,  # always 0
        _,  # always 0
        vitality,
        level_2,
        current_souls,
        _collected_souls, # Maybe, not verified
    ) = struct.unpack(
        "<IIIIIIIIIIIIIIIIIIIIIIIII",
        entry.content[char_start_idx:char_start_idx + 100]
    )

    _ng_plus = entry.content[char_start_idx+214]  # Unverified
    return DS3Character(
        index=char_idx,
        name=name,
        hp_current=hp_current,
        hp_max=hp_max,
        hp_base=hp_base,
        fp_current=fp_current,
        fp_max=fp_max,
        fp_base=fp_base,
        stamina_current=stamina_current,
        stamina_max=stamina_max,
        stamina_base=stamina_base,
        level=level,
        souls=current_souls,
        vigor=vigor,
        attunement=attunement,
        endurance=endurance,
        strength=strength,
        dexterity=dexterity,
        intelligence=intelligence,
        faith=faith,
        luck=luck,
        vitality=vitality,
    )


def parse_ds3_file(sl2_file: SL2File):
    if sl2_file.game != Game.DS3:
        raise ValueError(
            f"Expected DS3 save file, got {sl2_file.game}"
        )

    menu_entry = sl2_file.entries[10]
    steam_id, = struct.unpack("<q", menu_entry.content[4:12])
    occupied_slots = struct.unpack(
        "<bbbbbbbbbb", menu_entry.content[4244:4254]
    )
    characters = []
    for idx in range(10):
        char = None
        if occupied_slots[idx] == 1:
            char = character_from_entry(
                idx, menu_entry, sl2_file.entries[idx]
            )

        characters.append(char)

    return DS3SaveFile(
        sl2_file,
        characters,
        sl2_file.entries[10],
        sl2_file.entries[11],
    )
