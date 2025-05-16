from .structures import SL2File

DS3_KEY = b"\xfd\x46\x4d\x69\x5e\x69\xa3\x9a\x10\xe3\x19\xa7\xac\xe8\xb7\xfa"


def parse_ds3_file(sl2_file: SL2File):
    # Length of DS3 save file is 786432
    for idx, entry in enumerate(sl2_file.entries):
        # Empty slot
        if entry.decrypted_data[0] != 98:
            continue

        # TODO what is 0:108
        idx = 108
        while True:
            v = entry.decrypted_data[idx:idx+8]
            if v != b"\x00\x00\x00\x00\xff\xff\xff\xff":
                print(v)
                break
            idx += 8
