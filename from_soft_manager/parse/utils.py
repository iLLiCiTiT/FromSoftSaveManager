import struct
from contextlib import contextmanager

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

from .structures import BND4Entry, BND4EntryHeader, BND4Header, SL2File, Game
from .parse_dsr import DSR_KEY, parse_dsr_file
from .parse_ds2 import DS2_KEY, parse_ds2_file
from .parse_ds3 import DS3_KEY, parse_ds3_file


class _FileReader:
    def __init__(self, content):
        self._content = content
        self._offset = 0
        self._content_size = len(content)

    @classmethod
    def from_path(cls, path):
        with open(path, "rb") as stream:
            content = stream.read()
        return cls(content)

    def take(self, size):
        if self._offset + size > len(self._content):
            raise ValueError("Not enough data to read")
        data = self._content[self._offset:self._offset + size]
        self._offset += size
        return data

    @contextmanager
    def go_to(self, offset):
        original_offset = self._offset
        self._offset = offset
        try:
            yield
        finally:
            self._offset = original_offset


def bytes_to_int(in_bytes):
    return int.from_bytes(in_bytes, "little")


def add_pkcs7_padding(data, block_size=16):
    pl = block_size - (len(data) % block_size)
    return data + bytearray([pl for i in range(pl)])


def decrypt_sl2_file(input_sl2_file: str) -> SL2File:
    # TODO return whole file content to be able to unparse it back
    reader = _FileReader.from_path(input_sl2_file)
    bnd_vers = reader.take(4)
    header_data = struct.unpack("<QIQQQQ?", reader.take(45))
    bnd4_header = BND4Header(
        bnd_vers,
        *header_data,
        reader.take(15),
    )
    # TODO better decision to find out what game it this
    game = None
    key = None
    if bnd4_header.files_count == 11:
        key = DSR_KEY
        game = Game.DSR
    elif bnd4_header.files_count == 23:
        game = Game.DS2
        key = DS2_KEY
    elif bnd4_header.files_count == 12:
        game = Game.DS3
        key = DS3_KEY

    if game is None:
        raise NotImplementedError("Game not supported")

    padding_block_size = 16
    if game == Game.DS2:
        padding_block_size = 8

    decode_fmt = "utf-16" if bnd4_header.is_utf16 else "utf-8"
    entries = []
    for idx in range(bnd4_header.files_count):
        entry_header = BND4EntryHeader(
            *struct.unpack("<QQIIQ", reader.take(32))
        )
        with reader.go_to(entry_header.entry_name_offset):
            entry_name_b = reader.take(26)
            # entry_name = entry_name_b[24:].decode(decode_fmt)

        with reader.go_to(entry_header.entry_data_offset):
            entry_data = reader.take(entry_header.entry_size)
        checksum = entry_data[0:16]
        iv = entry_data[16:32]
        # NOTE: DS1 does not use encryption
        # NOTE: Elden ring does not use encryption - how to find out it is ER?
        decryptor = Cipher(algorithms.AES128(key), modes.CBC(iv)).decryptor()
        decrypted_content = (
            decryptor.update(entry_data[16:]) + decryptor.finalize()
        )
        _out_iv = decrypted_content[0:16]
        decrypted_length = struct.unpack("<i", decrypted_content[16:20])[0]
        decrypted_content = decrypted_content[20:]
        _padding = decrypted_content[decrypted_length:]
        entries.append(
            BND4Entry(
                entry_header,
                entry_name_b,
                decrypted_content[:decrypted_length],
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
