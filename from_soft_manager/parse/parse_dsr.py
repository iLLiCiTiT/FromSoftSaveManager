import struct
from dataclasses import dataclass

from .structures import SL2File
from ._dsr_items import ITEMS_BY_IDS

DSR_KEY = b"\x01\x23\x45\x67\x89\xab\xcd\xef\xfe\xdc\xba\x98\x76\x54\x32\x10"
DSR_CLASSES = {
    0: "Warrior",
    1: "Knight",
    2: "Wanderer",
    3: "Thief",
    4: "Bandit",
    5: "Hunter",
    6: "Sorcerer",
    7: "Pyromancer",
    8: "Cleric",
    9: "Deprived"
}

@dataclass
class InventoryItem:
    item_id: int
    item_type: int
    upgrade_level: int
    infusion: int
    amount: int
    durability: int
    order: int
    name: str
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
):
    items_by_id = ITEMS_BY_IDS.get(item_type, {})
    item = items_by_id.get(item_id)
    upgrade_level = 0
    infusion = 0
    if not item:
        # NOTE: Last 2 digits if upgrade level
        # NOTE: 3rd digit from the end digit is infusion type
        # - 0 - normal
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

    if item:
        name = item["name"]
    else:
        name = f"NA {item_type} {item_id}"

    return InventoryItem(
        item_id,
        item_type,
        upgrade_level,
        infusion,
        amount,
        durability,
        order,
        name,
        idx,
        content,
        in_botomless_box,
    )


def parse_dsr_file(sl2_file: SL2File):
    # Start of items list 712
    # item["type"] = read_int_le(data, 4)
    # item["id"] = read_int_le(data, 4)
    # item["amount"] = read_int_le(data, 4)
    # skip_bytes(4)
    # have = read_int_le(data, 4)
    # skip_bytes(8)
    for entry in sl2_file.entries[:-1]:
        if entry.content[0] == 0:
            continue

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
            _hp, # always 0
            hp_current,
            hp_max,
            hp_base,
            # Maybe poise?
            something_current,
            something_max,
            something_base,
            _st,  # always 0
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
        if _hp != 0:
            print("!!! _hp not 0", _hp)
        if _st != 0:
            print("!!! _st not 0", _st)

        (
            _, unknown_3,  # Is always 10
            _, unknown_4,
            humanity,
            resistance,
            level,
            souls,
            earned, # Maybe it is 'used'
        ) = struct.unpack("<IIIIIQIII", entry.content[192:232])
        unknown_5 = entry.content[232:244]
        b_name = entry.content[244:258]
        while b_name.endswith(b"\x00\x00"):
            b_name = b_name[:-2]
        name = b_name.decode("utf-16")
        unknown_6 = entry.content[258:278]
        (
            is_male,
            player_class_id,
            body,
            gift,
        ) = struct.unpack("<IBBB", entry.content[278:285])
        male = "male" if is_male == 1 else "female"
        player_class = DSR_CLASSES[player_class_id]
        # I guess this is all the possible modifications of shape of head
        unknown_7 = entry.content[285:352]
        (
            face,
            hairs,
            color,
        ) = struct.unpack("<BBB", entry.content[352:355])
        print(f"{name} ({male} {player_class}) | lvl {level} | hum {humanity} | souls {souls}/{earned}")
        # print(f"Vit {vitality} | Att {attunement} | End {endurance} | Str {strength} | Dex {dexterity} | Res {resistance} | Int {intelligence} | Fth {faith}")
        # print("HP:", hp_current, hp_max, hp_base)
        # print("Stamina:", stamina_current, stamina_max, stamina_base)        inventory_offset = 860
        inventory_offset = 860
        botomless_box_offset = 58504

        full_ffs = int.from_bytes(b"\xff\xff\xff\xff", "little")
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
            if item_id == full_ffs:
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
            if item_id == full_ffs:
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

        inventory_items.sort(key=lambda item: item["order"])
        botomless_box_items.sort(key=lambda item: item["order"])
