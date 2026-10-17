"""Readers for Sekiro game data.

- DataN.bhd/.bdt: BHD5 archives whose headers are RSA-encrypted with public keys
  embedded in sekiro.exe as PEM text (pass the exe, SteamStub does not encrypt
  .rdata), 40 byte file entries, some ranges AES-128-ECB encrypted.
- DCX: DFLT (zlib) or KRAK (Oodle, decompressed with the game's oo2core_6_win64.dll).
- FMG version 2 (64-bit offsets).
- Menu icon atlases: SB_Icon*.tpf + SB_Icon*.layout (XML sprite rects).

Shares BND4/TPF/PARAM parsing and the path hash with tools/ds2_items/ds2data.py.
Requires pycryptodome.
"""
import base64
import ctypes
import os
import re
import struct
import sys
import zlib

from Crypto.Cipher import AES

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ds2_items"))
import ds2data  # noqa: E402

parse_bnd4 = ds2data.parse_bnd4
parse_param = ds2data.parse_param
tpf_textures = ds2data.tpf_textures
path_hash = ds2data.path_hash


def embedded_rsa_keys(exe_path):
    with open(exe_path, "rb") as f:
        data = f.read()
    keys = []
    pattern = rb"-----BEGIN RSA PUBLIC KEY-----[A-Za-z0-9+/=\r\n]+-----END RSA PUBLIC KEY-----"
    for m in re.finditer(pattern, data):
        der = base64.b64decode(b"".join(l for l in m.group().splitlines() if b"---" not in l))
        p = 1
        ln = der[p]
        p += 1
        if ln & 0x80:
            p += ln & 0x7F
        n, p = ds2data._der_int(der, p)
        e, _ = ds2data._der_int(der, p)
        keys.append((n, e))
    return keys


class Oodle:
    def __init__(self, game_dir):
        self.dll = ctypes.WinDLL(os.path.join(game_dir, "oo2core_6_win64.dll"))
        f = self.dll.OodleLZ_Decompress
        f.restype = ctypes.c_longlong
        f.argtypes = [ctypes.c_void_p, ctypes.c_longlong, ctypes.c_void_p, ctypes.c_longlong,
                      ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_longlong,
                      ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_longlong, ctypes.c_int]

    def decompress(self, data, size):
        out = ctypes.create_string_buffer(size)
        src = ctypes.create_string_buffer(data, len(data))
        n = self.dll.OodleLZ_Decompress(src, len(data), out, size, 0, 0, 0, None, 0, None, None, None, 0, 3)
        if n != size:
            raise RuntimeError("Oodle decompression failed")
        return out.raw


class Archive:
    def __init__(self, game_dir, exe_path, name="Data1"):
        with open(os.path.join(game_dir, f"{name}.bhd"), "rb") as f:
            raw = f.read()
        self.bhd = None
        for n, e in embedded_rsa_keys(exe_path):
            data = ds2data._rsa_decrypt(raw, n, e)
            if data[:4] == b"BHD5":
                self.bhd = data
                break
        if self.bhd is None:
            raise RuntimeError(f"no embedded key decrypts {name}.bhd")
        self.bdt_path = os.path.join(game_dir, f"{name}.bdt")
        self.oodle = Oodle(game_dir)
        bucket_count, buckets_offset = struct.unpack_from("<II", self.bhd, 16)
        self.entries = {}
        for i in range(bucket_count):
            count, offset = struct.unpack_from("<II", self.bhd, buckets_offset + i * 8)
            for j in range(count):
                h, size, data_offset, _sha, aes_offset, unpadded = struct.unpack_from(
                    "<IIqqqq", self.bhd, offset + j * 0x28)
                self.entries[h] = (size, data_offset, aes_offset, unpadded)

    def undcx(self, b):
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
            return self.oodle.decompress(data, size)
        raise RuntimeError(f"unsupported DCX format {fmt}")

    def read_hash(self, h):
        size, offset, aes_offset, unpadded = self.entries[h]
        with open(self.bdt_path, "rb") as f:
            f.seek(offset)
            data = bytearray(f.read(size))
        if aes_offset:
            key = self.bhd[aes_offset:aes_offset + 16]
            ranges = struct.unpack_from("<I", self.bhd, aes_offset + 16)[0]
            cipher = AES.new(key, AES.MODE_ECB)
            for r in range(ranges):
                start, end = struct.unpack_from("<qq", self.bhd, aes_offset + 20 + r * 16)
                if start != -1 and end > start:
                    data[start:end] = cipher.decrypt(bytes(data[start:end]))
        if unpadded > 0:
            data = data[:unpadded]
        return self.undcx(bytes(data))

    def read(self, path):
        return self.read_hash(path_hash(path))

    def bnd(self, path):
        return {name: self.undcx(data) for name, data in parse_bnd4(self.read(path)).items()}


def parse_fmg2(d):
    assert d[0] == 0 and d[2] == 2, "expected FMG version 2"
    group_count = struct.unpack_from("<I", d, 0x0C)[0]
    strings_offset = struct.unpack_from("<Q", d, 0x18)[0]
    out = {}
    for g in range(group_count):
        index, first, last, _ = struct.unpack_from("<iiii", d, 0x28 + g * 16)
        for k in range(last - first + 1):
            so = struct.unpack_from("<Q", d, strings_offset + (index + k) * 8)[0]
            if so == 0:
                continue
            end = so
            while d[end:end + 2] != b"\0\0":
                end += 2
            out[first + k] = d[so:end].decode("utf-16le")
    return out


ICON_LAYOUT_PATH = "/menu/hi/01_common.sblytbnd.dcx"
ICON_TEXTURE_PATH = "/menu/hi/01_common.tpf.dcx"


def icon_atlases(archive):
    """Return {icon id: (texture name, x, y, w, h)} and {texture name: dds} for the item icons.

    Most icons are in the SB_Icon* atlases; a few (e.g. Divine Grass) only exist in other
    atlases such as SB_TextEffect_02, which are used when SB_Icon* has no copy."""
    layouts = parse_bnd4(archive.read(ICON_LAYOUT_PATH))
    sprites = {}
    names = sorted((n for n in layouts if n.endswith(".layout")), key=lambda n: (not n.startswith("SB_Icon"), n))
    for name in names:
        texture = name[:-len(".layout")]
        for m in re.finditer(r'<SubTexture name="MENU_ItemIcon_(\d+)\.png" x="(\d+)" y="(\d+)" '
                             r'width="(\d+)" height="(\d+)"', layouts[name].decode("utf-8", "replace")):
            sprites.setdefault(int(m.group(1)), (texture, int(m.group(2)), int(m.group(3)),
                                                 int(m.group(4)), int(m.group(5))))
    textures = dict(tpf_textures(archive.read(ICON_TEXTURE_PATH)))
    return sprites, textures


def menu_sprites(archive):
    """Return {sprite name: (texture name, x, y, w, h)} of every /menu/hi/01_common layout and
    {texture name: dds} of its atlases."""
    layouts = parse_bnd4(archive.read(ICON_LAYOUT_PATH))
    sprites = {}
    for name, xml in layouts.items():
        if not name.endswith(".layout"):
            continue
        texture = name[:-len(".layout")]
        for m in re.finditer(r'<SubTexture name="([^"]+)\.png" x="(\d+)" y="(\d+)" '
                             r'width="(\d+)" height="(\d+)"', xml.decode("utf-8", "replace")):
            sprites.setdefault(m.group(1), (texture, int(m.group(2)), int(m.group(3)),
                                            int(m.group(4)), int(m.group(5))))
    textures = dict(tpf_textures(archive.read(ICON_TEXTURE_PATH)))
    return sprites, textures
