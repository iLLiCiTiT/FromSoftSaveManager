import struct

from .structures import SL2File

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


def parse_dsr_file(sl2_file: SL2File):
    # Start of items list 712
    # item["type"] = read_int_le(data, 4)
    # item["id"] = read_int_le(data, 4)
    # item["amount"] = read_int_le(data, 4)
    # skip_bytes(4)
    # have = read_int_le(data, 4)
    # skip_bytes(8)
    for entry in sl2_file.entries[:-1]:
        if entry.decrypted_data[0] == 0:
            continue

        unknown_1 = entry.decrypted_data[0:92]

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
        ) = struct.unpack("<IIII", entry.decrypted_data[92:108])
        if _hp != 0:
            print("!!! _hp not 0", _hp)
        # Maybe poise?
        (
            something_current,
            something_max,
            something_base,
        ) = struct.unpack("<III", entry.decrypted_data[108:120])
        (
            _st, # always 0
            stamina_current,
            stamina_max,
            stamina_base,
        ) = struct.unpack("<IIII", entry.decrypted_data[120:136])
        if _st != 0:
            print("!!! _st not 0", _st)
        (
            _, vitality,
            _, attunement,
            _, endurance,
            _, strength,
            _, dexterity,
            _, intelligence,
            _, faith,
        ) = struct.unpack(
            "<IIIIIIIIIIIIII", entry.decrypted_data[136:192]
        )
        # Maybe this is resistance?
        _, unknown_3 = struct.unpack("<II", entry.decrypted_data[192:200]) # Is always 10
        _, unknown_4 = struct.unpack("<II", entry.decrypted_data[200:208])
        (
            humanity,
            resistance,
            level,
            souls,
            earned, # Maybe it is 'used'
        ) = struct.unpack("<IQIII", entry.decrypted_data[208:232])
        unknown_5 = entry.decrypted_data[232:244]
        b_name = entry.decrypted_data[244:258]
        while b_name.endswith(b"\x00\x00"):
            b_name = b_name[:-2]
        name = b_name.decode("utf-16")
        unknown_6 = entry.decrypted_data[258:278]
        is_male = struct.unpack("<I", entry.decrypted_data[278:282])[0] == 1
        male = "male" if is_male else "female"
        player_class = DSR_CLASSES[entry.decrypted_data[282]]
        body = entry.decrypted_data[283]
        gift = entry.decrypted_data[284]
        unknown_7 = entry.decrypted_data[285:352]
        face = entry.decrypted_data[352]
        hairs = entry.decrypted_data[353]
        color = entry.decrypted_data[354]
        (
            slot_1, slot_2, slot_3, slot_4, slot_5
        ) = struct.unpack("<IIIII", entry.decrypted_data[692:712])
        # print(20*"-")
        print(f"{name} ({male} {player_class}) | lvl {level} | hum {humanity} | souls {souls}/{earned}")
        print(slot_1, slot_2, slot_3, slot_4, slot_5)
        # print(f"Vit {vitality} | Att {attunement} | End {endurance} | Str {strength} | Dex {dexterity} | Res {resistance} | Int {intelligence} | Fth {faith}")
        # print("HP:", hp_current, hp_max, hp_base)
        # print("Stamina:", stamina_current, stamina_max, stamina_base)