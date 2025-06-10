import struct
from dataclasses import dataclass
from typing import Optional

from .structures import Game, SL2File, BND4Entry
from ._dsr_items import ITEMS_BY_IDS

DSR_KEY = b"\x01\x23\x45\x67\x89\xab\xcd\xef\xfe\xdc\xba\x98\x76\x54\x32\x10"

FULL_FFS = int.from_bytes(b"\xff\xff\xff\xff", "little")


# TODO it will be necessary to capture changes
@dataclass
class InventoryItem:
    item_id: int
    item_type: int
    upgrade_level: int
    infusion: int
    amount: int
    durability: int
    order: int
    idx: int
    content: bytes
    in_botomless_box: bool


def _get_item(
    idx,
    content,
    in_botomless_box,
    item_type,
    item_id,
    amount,
    durability,
    order,
) -> InventoryItem:
    items_by_id = ITEMS_BY_IDS.get(item_type, {})
    item = items_by_id.get(item_id)
    upgrade_level = 0
    infusion = 0
    if not item:
        # NOTE: Last 2 digits if upgrade level
        # NOTE: 3rd digit from the end digit is infusion type
        # - 0 - standard
        # - 1 - crystal
        # - 2 - lightning
        # - 3 - raw
        # - 4 - magic
        # - 5 - enchanted
        # - 6 - divine
        # - 7 - occult
        # - 8 - fire
        # - 9 - chaos
        # NOTE: Pyromancy Flame have different number
        #   increment on level up

        upgrade_level = item_id % 100
        new_id = item_id - upgrade_level
        item = items_by_id.get(new_id)
        if item:
            item_id = new_id
        else:
            infusion = item_id % 1000
            new_id = new_id - infusion
            item = items_by_id.get(new_id)
            if item:
                item_id = new_id

    return InventoryItem(
        item_id,
        item_type,
        upgrade_level,
        infusion,
        amount,
        durability,
        order,
        idx,
        content,
        in_botomless_box,
    )


@dataclass
class AttunementSlot:
    item_id: int
    remaining_uses: int


@dataclass
class DSRCharacter:
    index: int
    name: str
    humanity: int
    souls: int
    hollow_state: int
    covenant_id: int
    covenant_levels: tuple[int, int, int, int, int, int, int, int, int, int]

    used_gestures: list[int]

    hp_current: int
    hp_max: int
    hp_base: int
    stamina_current: int
    stamina_max: int
    stamina_base: int

    level: int
    vitality: int
    attunement: int
    endurance: int
    strength: int
    dexterity: int
    resistance: int
    intelligence: int
    faith: int

    sex: int
    class_id: int
    physique_id: int
    gift_id: int

    bleed_res: int
    poison_res: int
    curse_res: int

    face_id: int
    hair_style_id: int
    hair_color_id: int

    l_hand_slot_1: int
    l_hand_slot_2: int
    r_hand_slot_1: int
    r_hand_slot_2: int
    l_arrows_slot: int
    l_bolts_slot: int
    r_arrows_slot: int
    r_bolts_slot: int
    head_slot: int
    body_slot: int
    arms_slot: int
    legs_slot: int
    l_ring_slot: int
    r_ring_slot: int
    q1_slot: int
    q2_slot: int
    q3_slot: int
    q4_slot: int
    q5_slot: int
    inventory_items: list[InventoryItem]
    botomless_box_items: list[InventoryItem]
    attunement_slots: list[AttunementSlot]


def character_from_entry(
    index: int, entry: BND4Entry
) -> Optional[DSRCharacter]:
    if entry.content[0] == 0:
        return None

    unknown_1 = entry.content[0:92]

    # print(unknown_1[0:4] == b"G\x00\x00\x00")
    # print(unknown_1[4:8])
    # print(unknown_1[8:12])
    # print(unknown_1[12:16] == b"\\\x00\x00\x00")
    # print(unknown_1[16:24] == b"\x8d\xf2\x01\x00\\\x00\x00\x00")
    # print(unknown_1[24:32] == b"\xac\xe5\x01\x00\xde\xe6\x01\x00")
    # print(unknown_1[32:40] == b"@\x00\x00\x00H\xe6\x01\x00")
    # if unknown_1[16:24] != b"\x8d\xf2\x01\x00\\\x00\x00\x00":
    #     print(unknown_1[16:24])
    # if unknown_1[24:32] != b"\xac\xe5\x01\x00\xde\xe6\x01\x00":
    #     print(unknown_1[24:32])
    # if unknown_1[32:40] != b"@\x00\x00\x00H\xe6\x01\x00":
    #     print(unknown_1[32:40])
    (
        unknown_2, # always 0
        hp_current,
        hp_max,
        hp_base,
        # Maybe poise?
        something_current,
        something_max,
        something_base,
        unknown_3,  # always 0
        stamina_current,
        stamina_max,
        stamina_base,
        _, vitality,
        _, attunement,
        _, endurance,
        _, strength,
        _, dexterity,
        _, intelligence,
        _, faith,
    ) = struct.unpack(
        "<IIIIIIIIIIIIIIIIIIIIIIIII",
        entry.content[92:192]
    )
    if unknown_2 != 0:
        print("!!! unknown_2 not 0", unknown_2)
    if unknown_3 != 0:
        print("!!! unknown_3 not 0", unknown_3)

    (
        _, unknown_4,  # Is always 10
        _, unknown_5,
        humanity,
        resistance,
        level,
        souls,
        earned, # Maybe it is 'used'
        unknown_6,  # Is always 0
        hollow_state, # 0 human / 8 hollow
    ) = struct.unpack("<IIIIIQIIQII", entry.content[192:244])
    b_name = b""
    name_idx = 244
    while True:
        v = entry.content[name_idx:name_idx + 2]
        name_idx += 2
        if v == b"\x00\x00":
            break
        b_name += v

    name = b_name.decode("utf-16")
    unknown_7 = entry.content[258:278]
    (
        sex,  # 0 female / 1 male
        class_id,
        physique_id,
        gift_id,
    ) = struct.unpack("<IBBB", entry.content[278:285])
    unknown_8 = struct.unpack("<BBBBBIIIII", entry.content[285:310])
    covenant_lvls = struct.unpack("<BBBBBBBBBB", entry.content[310:320])
    unknown_9 = struct.unpack("<III", entry.content[320:332]) # 3x '0'

    (
        toxic_res, # Could be swapped with poison res (should have same value)
        bleed_res,
        poison_res,
        curse_res,
    ) = struct.unpack("<IIII", entry.content[332:348])
    unknown_10 = struct.unpack("<BBB", entry.content[348:351])
    covenant_id = entry.content[351]

    # These are presets and don't mean anything
    # - user can customize anything
    (
        face_id,
        hair_style_id,
        hair_color_id,
    ) = struct.unpack("<BBB", entry.content[352:355])

    # I guess this is all the possible modifications of shape of head
    unknown_11 = entry.content[355:376]

    # 2nd name of the character in the game
    b_name_2 = b""
    name_idx = 376
    while True:
        v = entry.content[name_idx:name_idx + 2]
        name_idx += 2
        if v == b"\x00\x00":
            break
        b_name_2 += v
    name_2 = b_name_2.decode("utf-16")
    unknown_12 = entry.content[name_idx:420]
    # App version used to create/save character
    # app_version = entry.content[420:428].rstrip(b"\x00").decode("utf-8")

    unknown_14 = struct.unpack(f"<{20*'I'}", entry.content[428:508])
    if set(unknown_14) != {0}:
        print(f"!!! Expected only zeros got: {unknown_14}")

    unknown_15 = struct.unpack(f"<{51*'I'}", entry.content[508:712])
    (
        l_ring_slot_item_type,
        r_ring_slot_item_type,
        q1_slot_item_type,
        q2_slot_item_type,
        q3_slot_item_type,
        q4_slot_item_type,
        q5_slot_item_type,
        one_double_handling, # 3 if is dual wielding
        l_hand_flag, # Maybe 1 if can cast magic or is empty
        r_hand_flag, # Maybe 1 if can cast magic or is empty
        unknown_17_flag, # Can be 0 or 1
        unknown_18_flag, # Can be 0 or 1
        unknown_19_flag, # Can be 0 or 1
        unknown_20_flag, # Can be 0 or 1
        l_hand_slot_1,
        l_hand_slot_2,
        r_hand_slot_1,
        r_hand_slot_2,
        l_arrows_slot,
        l_bolts_slot,
        r_arrows_slot,
        r_bolts_slot,
        head_slot,
        body_slot,
        arms_slot,
        legs_slot,
        unknown_21_flag,
        l_ring_slot,
        r_ring_slot,
        q1_slot,
        q2_slot,
        q3_slot,
        q4_slot,
        q5_slot,
        backpack_count, # Without key items
        unknown_22_flag,
        max_inventory_count, # Maybe? (it is set to 2048)
    ) = struct.unpack(f"<{'I'*37}", entry.content[712:860])

    inventory_offset = 860
    attunement_slots_offset = 58208
    botomless_box_offset = 58504

    inventory_items = []
    botomless_box_items = []
    for idx in range(2028):
        start_idx = inventory_offset + (idx * 28)
        item_content = entry.content[start_idx:start_idx + 28]
        (
            item_type, item_id, amount, order, _un1, durability, _un2
        ) = struct.unpack(
            "<IIIIIII", item_content
        )
        if item_id == FULL_FFS:
            continue

        # Ignore empty hands and empty slots in inventory
        # if item_type == 0 and item_id == 900000:
        #     continue
        # if item_type == 268435456 and item_id in {
        #     900000,
        #     901000,
        #     902000,
        #     903000,
        # }:
        #     continue

        item = _get_item(
            idx,
            item_content,
            False,
            item_type,
            item_id,
            amount,
            durability,
            order
        )
        inventory_items.append(item)

    # No idea what is this (inbetween inventory and attunement slots)
    # Values I've had set at this position
    # - 303
    # - 340
    # - 463
    # - 609
    unknown_12 = entry.content[58204:58208]

    # Attunement slots
    attunement_slots = []
    for idx in range(12):
        start_idx = attunement_slots_offset + (idx * 8)
        item_content = entry.content[start_idx:start_idx + 8]
        (
            item_id,
            remaining_uses,
        ) = struct.unpack("<II", item_content)
        # For some reason remaining uses are stored as multiplied by 3
        attunement_slots.append(
            AttunementSlot(item_id, remaining_uses)
        )

    unknown_13 = struct.unpack("<IIIIIII", entry.content[58304:58332])

    used_gestures = struct.unpack("<hhhhhhhhhhhhhhhhhh", entry.content[58332:58368])

    # Progress in the game or bonfires or both
    unknown_14 = entry.content[58368:58504]

    for idx in range(2048):
        start_idx = botomless_box_offset + (idx * 32)
        item_content = entry.content[start_idx:start_idx + 32]
        (
            _ui1,
            _ui2,
            item_id,
            order,
            amount,
            _ui4,
            durability,
            _ui5,
            _ui6
        ) = struct.unpack("<IIIIhhIII", item_content)
        if item_id == FULL_FFS:
            continue
        item_type = 0
        if item_id >= 1073741824:
            item_type = 1073741824
        elif item_id >= 536870912:
            item_type = 536870912
        elif item_id >= 268435456:
            item_type = 268435456
        item_id -= item_type

        item = _get_item(
            idx,
            item_content,
            True,
            item_type,
            item_id,
            amount,
            durability,
            order,
        )

        botomless_box_items.append(item)

    # 124040
    # 0, 0, ?, 3, 65541, 131079, 196617, 262155
    # 327692/327693
    # 393230/393231
    # 458768/458769
    # 524306/524307
    # 589844/589845
    # 655382/655383
    # 720920/720921
    # 786458/786459
    # 851996/851997
    # 917534/917535
    # 4294967294 (b"\xfe\xff\xff\xff")
    some_count, _zero = struct.unpack("<II", entry.content[124116:124124])
    s_idx = 124124
    e_idx = s_idx + (some_count * 4)
    some_ids = struct.unpack(f"<{'I' * some_count}", entry.content[s_idx:e_idx])

    s_idx = e_idx
    e_idx = s_idx + (16 * 4)
    _zeros = struct.unpack(f"<{'I' * 16}", entry.content[s_idx:e_idx])

    s_idx = e_idx
    e_idx = s_idx + 45
    unknown_xxx = entry.content[s_idx:e_idx]

    s_idx = e_idx
    e_idx = s_idx + 32
    unknown_xxx = entry.content[s_idx:e_idx]

    s_idx = e_idx
    e_idx = s_idx + 77
    unknown_xxx = entry.content[s_idx:e_idx]

    # b'\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00'
    s_idx = e_idx
    e_idx = s_idx + 12
    unknown_xxx = entry.content[s_idx:e_idx]

    # Repeating b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00'
    s_idx = e_idx
    e_idx = s_idx + (127 * 16)
    unknown_xxx = entry.content[s_idx:e_idx]

    # Repeating b'\xff\xff\xff\xff\x00\x00\x00\x00'
    s_idx = e_idx
    e_idx = s_idx + (121 * 8)
    unknown_xxx = entry.content[s_idx:e_idx]

    return DSRCharacter(
        index,
        name,
        humanity,
        souls,
        hollow_state,
        covenant_id,
        covenant_lvls,

        list(used_gestures),

        hp_current,
        hp_max,
        hp_base,
        stamina_current,
        stamina_max,
        stamina_base,

        level,
        vitality,
        attunement,
        endurance,
        strength,
        dexterity,
        resistance,
        intelligence,
        faith,

        sex,
        class_id,
        physique_id,
        gift_id,

        bleed_res,
        poison_res,
        curse_res,

        face_id,
        hair_style_id,
        hair_color_id,

        l_hand_slot_1,
        l_hand_slot_2,
        r_hand_slot_1,
        r_hand_slot_2,
        l_arrows_slot,
        l_bolts_slot,
        r_arrows_slot,
        r_bolts_slot,
        head_slot,
        body_slot,
        arms_slot,
        legs_slot,
        l_ring_slot,
        r_ring_slot,
        q1_slot,
        q2_slot,
        q3_slot,
        q4_slot,
        q5_slot,
        inventory_items,
        botomless_box_items,
        attunement_slots,
    )


@dataclass
class DSRSaveFile:
    sl2_file: SL2File
    characters: list[Optional[DSRCharacter]]
    side_car_enty: BND4Entry


def parse_dsr_file(sl2_file: SL2File) -> DSRSaveFile:
    if sl2_file.game != Game.DSR:
        raise ValueError(
            f"Expected DSR save file, got {sl2_file.game}"
        )

    characters = [
        character_from_entry(idx, entry)
        for idx, entry in enumerate(sl2_file.entries[:-1])
    ]

    return DSRSaveFile(
        sl2_file,
        characters,
        sl2_file.entries[-1],
    )
