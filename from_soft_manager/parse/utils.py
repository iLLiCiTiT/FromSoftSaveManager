import struct
from contextlib import contextmanager

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

from .structures import BND4Entry, BND4EntryHeader, BND4Header, SL2File, Game
from .parse_dsr import DSR_KEY, parse_dsr_file
from .parse_ds2 import DS2_KEY, parse_ds2_file
from .parse_ds3 import DS3_KEY, parse_ds3_file


def bytes_to_int(in_bytes):
    return int.from_bytes(in_bytes, "little")


def add_pkcs7_padding(data, block_size=16):
    pl = block_size - (len(data) % block_size)
    return data + bytearray([pl for i in range(pl)])


def decrypt_sl2_file(input_sl2_file: str) -> SL2File:
    # TODO return whole file content to be able to unparse it back
    with open(input_sl2_file, "rb") as stream:
        content = stream.read()
    bnd_vers = content[0:4]
    header_data = struct.unpack("<QIQQQQ?", content[4:49])
    bnd4_header = BND4Header(
        bnd_vers,
        *header_data,
        content[49:64],
    )
    # Determine game based on content
    game = None
    key = None
    if bnd4_header.files_count == 11:
        # NOTE: DS1 does not use encryption but DS1 probably
        #   won't be supported
        key = DSR_KEY
        game = Game.DSR
    elif bnd4_header.files_count == 23:
        # NOTE: This is based on DS2 SOFT
        game = Game.DS2
        key = DS2_KEY
    elif bnd4_header.files_count == 12:
        # Elden Ring and Dark Souls 3 have the same number of files and same
        #   header but Elden Ring does not encrypt the save file

        # Look at length of the first save file
        # - length of DS3 save file is 786480
        hs = 64 + 32
        entry_header = BND4EntryHeader(
            *struct.unpack("<QQIIQ", content[hs:hs + 32])
        )
        # QUESTION is that true even if save is without DLCs?
        if entry_header.entry_size not in (786480, 2621456):
            print(
                "WARNING: Save file size is not 786480 or 2621456"
                " which is unknown case, expecting save file is Elden Ring."
            )

        # TODO Try to find out better approach to determine the game
        if entry_header.entry_size == 786480:
            game = Game.DS3
            key = DS3_KEY
        else:
            game = Game.ER
            key = None

    if game is None:
        raise NotImplementedError("It was not possible to detect the game")

    # Will be important for unparsing
    # padding_block_size = 16
    # if game == Game.DS2:
    #     padding_block_size = 8

    decode_fmt = "utf-16" if bnd4_header.is_utf16 else "utf-8"
    entries = []
    for idx in range(bnd4_header.files_count):
        hs = 64 + (idx * 32)
        entry_header = BND4EntryHeader(
            *struct.unpack("<QQIIQ", content[hs:hs+32])
        )

        ns = entry_header.entry_name_offset
        entry_name_b = content[ns:ns+26]
        # entry_name = entry_name_b[24:].decode(decode_fmt)

        ds = entry_header.entry_data_offset
        de = ds + entry_header.entry_size
        entry_data = content[ds:de]
        checksum = entry_data[0:16]
        iv = entry_data[16:32]
        file_content = entry_data[16:]
        if key is not None:
            decryptor = Cipher(algorithms.AES128(key), modes.CBC(iv)).decryptor()
            decrypted_content = (
                decryptor.update(file_content) + decryptor.finalize()
            )
            _out_iv = decrypted_content[0:16]
            decrypted_length = struct.unpack("<i", decrypted_content[16:20])[0]
            decrypted_content = decrypted_content[20:]
            _padding = decrypted_content[decrypted_length:]
            file_content = decrypted_content[:decrypted_length]

        entries.append(
            BND4Entry(
                entry_header,
                entry_name_b,
                file_content,
            )
        )
    return SL2File(game, bnd4_header, entries)


def parse_file(filepath: str):
    sl2_file = decrypt_sl2_file(filepath)
    if sl2_file.game == Game.DSR:
        return parse_dsr_file(sl2_file)

    if sl2_file.game == Game.DS2:
        return parse_ds2_file(sl2_file)

    if sl2_file.game == Game.DS3:
        return parse_ds3_file(sl2_file)
