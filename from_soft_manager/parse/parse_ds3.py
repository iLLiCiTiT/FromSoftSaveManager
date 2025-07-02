import struct
from dataclasses import dataclass
from typing import Optional

from .structures import Game, SL2File, BND4Entry
from ._ds3_items import ITEMS_BY_ID

DS3_KEY = b"\xfd\x46\x4d\x69\x5e\x69\xa3\x9a\x10\xe3\x19\xa7\xac\xe8\xb7\xfa"


@dataclass
class InventoryItem:
    item_id: int
    category: str
    infusion: int = 0
    level: int = 0
    amount: int = 1

    @classmethod
    def from_inventory_id(cls, item_id: int) -> Optional["InventoryItem"]:
        if item_id in (
            110000,  # Empty weapon slot
            269335456,  # No head equipment
            269336456,  # No armor equipment
            269337456,  # No hands equipment
            269338456,  # No legs equipment
            4294967295, # Empty item slot
        ):
            return None

        level = infusion = 0
        # Wrong level calculation, to make it work is must be known current
        #   level of Estus flask (how many Estus shard upgrades happened).
        # If 'item level - current estus level = -1' then it is empty flask.
        # Estus flask
        if 1073741975 < item_id <= 1073741990:
            level = item_id - 1073741975
            item_id -= level

        # Ashen Estus flask
        elif 1073742015 < item_id <= 1073742030:
            level = item_id - 1073742015
            item_id -= level

        if 1000000 < item_id <= 23020000:
            level = item_id % 100
            item_id -= level
            infusion = item_id % 10000
            item_id -= infusion
        item = ITEMS_BY_ID.get(item_id)
        category = "tools"
        if item is not None:
            category = item["category"]
        return cls(item_id, infusion=infusion, level=level, category=category)

    @classmethod
    def from_inventory_ids(cls, *item_ids: int) -> list[Optional["InventoryItem"]]:
        return [
            cls.from_inventory_id(item_id) for item_id in item_ids
        ]


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
    collected_souls: int
    vigor: int
    attunement: int
    endurance: int
    strength: int
    dexterity: int
    intelligence: int
    faith: int
    luck: int
    vitality: int

    bleed_res: int
    poison_res: int
    curse_res: int
    frost_res: int

    inventory_items: list[InventoryItem]
    key_items: list[InventoryItem]
    storage_box_items: list[InventoryItem]


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
    idx = idx + 8
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
        collected_souls,
    ) = struct.unpack(
        "<IIIIIIIIIIIIIIIIIIIIIIIII",
        entry.content[idx:idx + 100]
    )
    b_name_2 = entry.content[idx + 112:idx + 144]

    # Warrior of Sunlight attempts and success - maybe?
    wos_attempts, wos_success = struct.unpack(
        "<II", entry.content[idx + 156:idx + 164]
    )
    (
        _toxic_res,
        bleed_res,
        poison_res,
        curse_res,
        frost_res,
    ) = struct.unpack(
        "<IIIII",
        entry.content[idx + 200:idx + 220]
    )

    unknown = entry.content[idx + 220:idx + 288]
    # \x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
    unknown = entry.content[idx + 288:idx + 300]
    # \x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff (11x)
    unknown = entry.content[idx + 300:idx + 476]
    # \x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
    unknown = entry.content[idx + 476:idx + 488]
    # 22x 4-bit integers (sometimes \xff\xff\xff\xff)
    unknown = entry.content[idx + 488:idx + 576]

    # one number and rest is '\x00\x00\x00\x00'
    unknown = entry.content[idx + 576:idx + 604]

    unknown = entry.content[idx + 604:idx + 784]

    # Looks like count of items in inventory - 2
    count, = struct.unpack("<I", entry.content[idx + 784:idx + 788])
    inventory_items = []
    for i in range(1920):
        start = idx + 788 + (i * 16)
        v = entry.content[start:start+16]
        # No idea where durability is stored
        _, item_id, item_count, _ = struct.unpack("<IIII", v)
        if item_count == 0:
            continue
        inv_item = InventoryItem.from_inventory_id(item_id)
        if inv_item is None:
            continue
        inv_item.amount = item_count
        item = ITEMS_BY_ID.get(inv_item.item_id)
        if item is None:
            print("Unknown item id:", inv_item.item_id)
        inventory_items.append(inv_item)

    zeros = entry.content[idx + 31508:idx + 31512]

    key_items = []
    for i in range(128):
        start = idx + 31512 + (i * 16)
        v = entry.content[start:start+16]
        _, item_id, item_count, _ = struct.unpack("<IIII", v)
        if item_count == 0:
            continue
        inv_item = InventoryItem.from_inventory_id(item_id)
        if inv_item is None:
            continue
        inv_item.amount = item_count
        item = ITEMS_BY_ID.get(inv_item.item_id)
        if item is None:
            print("Unknown item id:", inv_item.item_id)
        key_items.append(inv_item)

    zeros = entry.content[idx + 33560:idx + 33564]
    unknown = entry.content[idx + 33564:idx + 35732]
    unknown = entry.content[idx + 35732:idx + 35812]
    unknown = entry.content[idx + 35812:idx + 35864]

    used_gestures_b = entry.content[idx + 35864: idx + 35896]
    used_gestures = []
    for g_idx in range(7):
        offset = g_idx * 4
        gesture_id, _gesture_pos = struct.unpack(
            "<hh", used_gestures_b[offset:offset+4]
        )
        used_gestures.append(gesture_id)

    inv_idx = idx + 35896
    count = int.from_bytes(
        entry.content[inv_idx: inv_idx + 4],
        "little"
    )
    maybe_tools = []
    for i in range(count):
        offset = inv_idx + 4 + (i * 8)
        item_b = entry.content[offset:offset + 8]
        item_id, item_unknown = struct.unpack("<II", item_b)
        maybe_tools.append(InventoryItem.from_inventory_id(item_id))

    equip_offset = inv_idx + 4 + (count * 8)
    (
        l_hand_1, r_hand_1,
        l_hand_2, r_hand_2,
        l_hand_3, r_hand_3,
        arrows_1, bolts_1,
        arrows_2, bolts_2,
        _full_ff, _full_ff,
        head, chest, hands, legs,
        _full_ff,
        ring_1, ring_2, ring_3, ring_4,
        covenant,
        quick_item_1, quick_item_2, quick_item_3, quick_item_4, quick_item_5,
        quick_item_6, quick_item_7, quick_item_8, quick_item_9, quick_item_10,
        toolbelt_1, toolbelt_2, toolbelt_3, toolbelt_4, toolbelt_5,
    ) = struct.unpack(
        "<IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII",
        entry.content[equip_offset:equip_offset + 148]
    )
    (
        l_hand_1, r_hand_1,
        l_hand_2, r_hand_2,
        l_hand_3, r_hand_3,
        arrows_1, bolts_1,
        arrows_2, bolts_2,
        head, chest, hands, legs,
        ring_1, ring_2, ring_3, ring_4,
        covenant,
        quick_item_1, quick_item_2, quick_item_3, quick_item_4, quick_item_5,
        quick_item_6, quick_item_7, quick_item_8, quick_item_9, quick_item_10,
        toolbelt_1, toolbelt_2, toolbelt_3, toolbelt_4, toolbelt_5,
    ) = InventoryItem.from_inventory_ids(
        l_hand_1, r_hand_1,
        l_hand_2, r_hand_2,
        l_hand_3, r_hand_3,
        arrows_1, bolts_1,
        arrows_2, bolts_2,
        head, chest, hands, legs,
        ring_1, ring_2, ring_3, ring_4,
        covenant,
        quick_item_1, quick_item_2, quick_item_3, quick_item_4, quick_item_5,
        quick_item_6, quick_item_7, quick_item_8, quick_item_9, quick_item_10,
        toolbelt_1, toolbelt_2, toolbelt_3, toolbelt_4, toolbelt_5,
    )

    ffs = entry.content[equip_offset + 148:equip_offset + 152]
    face_data = entry.content[equip_offset + 152:equip_offset + 400]

    storage_box_items = []
    storage_offset = equip_offset + 400
    for i in range(1920):
        start = storage_offset + (i * 16)
        v = entry.content[start:start + 16]
        _, item_id, item_count, _ = struct.unpack("<IIII", v)
        if item_count == 0:
            continue
        inv_item = InventoryItem.from_inventory_id(item_id)
        if inv_item is None:
            continue
        inv_item.amount = item_count
        item = ITEMS_BY_ID.get(inv_item.item_id)
        if item is None:
            # 1073741918 - every char has it
            # 1073743837 - some key item?
            # 1073743947 - some key item?
            print("Unknown item id:", inv_item.item_id)
        storage_box_items.append(inv_item)

    # s_idx = storage_offset + (1920 * 16)
    # zeros = entry.content[s_idx:s_idx + 4]
    # some_items_1 = []
    # for i in range(128):
    #     start = s_idx + 4 + (i * 16)
    #     v = entry.content[start:start + 16]
    #     _, item_id, item_count, order = struct.unpack("<IIII", v)
    #     if item_count == 0:
    #         continue
    #     inv_item = InventoryItem.from_inventory_id(item_id)
    #     if inv_item is None:
    #         continue
    #     item = ITEMS_BY_ID.get(inv_item.item_id)
    #     if item is None:
    #         continue
    #     some_items_1.append(inv_item)
    #
    # s_idx += 4 + (128 * 16)
    # zeros = entry.content[s_idx:s_idx + 4]
    # some_items_2 = []
    # for i in range(128):
    #     start = s_idx + 4 + (i * 16)
    #     v = entry.content[start:start + 16]
    #     _, item_id, item_count, order = struct.unpack("<IIII", v)
    #     if item_count == 0:
    #         continue
    #     inv_item = InventoryItem.from_inventory_id(item_id)
    #     if inv_item is None:
    #         continue
    #     item = ITEMS_BY_ID.get(inv_item.item_id)
    #     if item is None:
    #         continue
    #     some_items_2.append(inv_item)
    #
    # s_idx += 4 + (128 * 16)
    # zeros = entry.content[s_idx:s_idx + 4]

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
        collected_souls=collected_souls,
        vigor=vigor,
        attunement=attunement,
        endurance=endurance,
        strength=strength,
        dexterity=dexterity,
        intelligence=intelligence,
        faith=faith,
        luck=luck,
        vitality=vitality,
        bleed_res=bleed_res,
        poison_res=poison_res,
        curse_res=curse_res,
        frost_res=frost_res,
        inventory_items=inventory_items,
        key_items=key_items,
        storage_box_items=storage_box_items,
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
