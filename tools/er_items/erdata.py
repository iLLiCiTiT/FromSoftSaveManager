"""Readers for Elden Ring game data.

- DataN.bhd / DLC.bhd: BHD5 archives with RSA-encrypted headers (keys embedded in
  eldenring.exe), 40 byte entries keyed by a 64-bit path hash, AES-128-ECB ranges.
- DCX: KRAK (Oodle, via the game's oo2core_6_win64.dll), ZSTD and DFLT.
- regulation.bin: AES-256-CBC encrypted DCX'd BND4 with the game params.

Shares FMG v2 / BND4 / TPF / PARAM parsing with the Sekiro and DS2 tooling.
Requires pycryptodome and zstandard.
"""
import os
import struct
import sys
import zlib

from Crypto.Cipher import AES

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "sekiro_items"))
import sekirodata  # noqa: E402
from sekirodata import parse_bnd4, parse_param, tpf_textures, parse_fmg2  # noqa: E402,F401

# Community documented Elden Ring regulation key (verified: decrypts to a DCX header).
REGULATION_KEY = bytes.fromhex("99BFFC366A6BC8C6F5827D093602D676C42892A01C207FB024D3AF4E493FEF99")


def path_hash(path):
    h = 0
    for ch in path.lower().replace("\\", "/"):
        h = (h * 0x85 + ord(ch)) & 0xFFFFFFFFFFFFFFFF
    return h


def undcx(b, oodle):
    if b[:4] != b"DCX\0":
        return b
    size, compressed = struct.unpack_from(">II", b, 0x1C)
    fmt = b[0x28:0x2C]
    i = b.find(b"DCA\0")
    header = struct.unpack_from(">I", b, i + 4)[0]
    data = b[i + header:i + header + compressed]
    if fmt == b"DFLT":
        return zlib.decompress(data)
    if fmt == b"KRAK":
        return oodle.decompress(data, size)
    if fmt == b"ZSTD":
        import zstandard
        return zstandard.ZstdDecompressor().decompress(data, max_output_size=size)
    raise RuntimeError(f"unsupported DCX format {fmt}")


class Archives:
    """All Elden Ring archives, looked up by path."""

    NAMES = ("Data0", "Data1", "Data2", "Data3", "DLC")

    def __init__(self, game_dir):
        self.game_dir = game_dir
        self.oodle = sekirodata.Oodle(game_dir)
        keys = sekirodata.embedded_rsa_keys(os.path.join(game_dir, "eldenring.exe"))
        self.archives = []
        for name in self.NAMES:
            with open(os.path.join(game_dir, f"{name}.bhd"), "rb") as f:
                raw = f.read()
            bhd = None
            for n, e in keys:
                data = sekirodata.ds2data._rsa_decrypt(raw, n, e)
                if data[:4] == b"BHD5":
                    bhd = data
                    break
            if bhd is None:
                raise RuntimeError(f"no embedded key decrypts {name}.bhd")
            entries = {}
            bucket_count, buckets_offset = struct.unpack_from("<II", bhd, 16)
            for i in range(bucket_count):
                count, offset = struct.unpack_from("<II", bhd, buckets_offset + i * 8)
                for j in range(count):
                    h, size, unpadded, data_offset, _sha, aes_offset = struct.unpack_from(
                        "<QIIqqq", bhd, offset + j * 0x28)
                    entries[h] = (size, unpadded, data_offset, aes_offset)
            self.archives.append((os.path.join(game_dir, f"{name}.bdt"), bhd, entries))

    def has(self, path):
        h = path_hash(path)
        return any(h in entries for _, _, entries in self.archives)

    def read(self, path):
        h = path_hash(path)
        for bdt, bhd, entries in self.archives:
            if h not in entries:
                continue
            size, unpadded, offset, aes_offset = entries[h]
            with open(bdt, "rb") as f:
                f.seek(offset)
                data = bytearray(f.read(size))
            if aes_offset:
                key = bhd[aes_offset:aes_offset + 16]
                ranges = struct.unpack_from("<I", bhd, aes_offset + 16)[0]
                cipher = AES.new(key, AES.MODE_ECB)
                for r in range(ranges):
                    start, end = struct.unpack_from("<qq", bhd, aes_offset + 20 + r * 16)
                    if start != -1 and end > start:
                        data[start:end] = cipher.decrypt(bytes(data[start:end]))
            if unpadded:
                data = data[:unpadded]
            return undcx(bytes(data), self.oodle)
        raise KeyError(path)

    def bnd(self, path):
        return {name: undcx(data, self.oodle) for name, data in parse_bnd4(self.read(path)).items()}

    def regulation(self):
        with open(os.path.join(self.game_dir, "regulation.bin"), "rb") as f:
            raw = f.read()
        data = AES.new(REGULATION_KEY, AES.MODE_CBC, raw[:16]).decrypt(raw[16:])
        return {name: undcx(d, self.oodle) for name, d in parse_bnd4(undcx(data, self.oodle)).items()}
