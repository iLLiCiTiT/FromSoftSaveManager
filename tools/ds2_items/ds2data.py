"""Readers for Dark Souls II: Scholar of the First Sin game data.

- GameDataEbl.bhd/.bdt: BHD5 archive, header RSA-encrypted with the shipped
  GameDataKeyCode.pem public key, some file ranges AES-128-ECB encrypted.
- enc_regulation.bnd.dcx: AES-128-CTR encrypted DCX(DFLT) compressed BND4 with params.
- *.fmg: text tables (version 1, 32-bit offsets).
- *.param: PARAM tables (64-bit row headers).

Requires pycryptodome.
"""
import base64
import struct
import zlib

from Crypto.Cipher import AES

# Community documented DS2 regulation key (verified: decrypts to a valid DCX header).
REGULATION_KEY = bytes([0x40, 0x17, 0x81, 0x30, 0xDF, 0x0A, 0x94, 0x54,
                        0x33, 0x09, 0xE1, 0x71, 0xEC, 0xBF, 0x25, 0x4C])


def path_hash(path):
    h = 0
    for ch in path.lower().replace("\\", "/"):
        h = (h * 37 + ord(ch)) & 0xFFFFFFFF
    return h


def _der_int(b, p):
    assert b[p] == 2
    p += 1
    ln = b[p]
    p += 1
    if ln & 0x80:
        n = ln & 0x7F
        ln = int.from_bytes(b[p:p + n], "big")
        p += n
    return int.from_bytes(b[p:p + ln], "big"), p + ln


def _load_rsa_public_key(pem_path):
    with open(pem_path) as f:
        text = f.read()
    der = base64.b64decode("".join(l for l in text.splitlines() if "---" not in l))
    p = 1
    ln = der[p]
    p += 1
    if ln & 0x80:
        p += ln & 0x7F
    n, p = _der_int(der, p)
    e, _ = _der_int(der, p)
    return n, e


def _rsa_decrypt(data, n, e):
    out = bytearray()
    for i in range(0, len(data), 256):
        m = pow(int.from_bytes(data[i:i + 256], "big"), e, n).to_bytes(256, "big")
        out += m[1:]
    return bytes(out)


def undcx(b):
    if b[:4] != b"DCX\0":
        return b
    i = b.find(b"DCA\0")
    header = struct.unpack_from(">I", b, i + 4)[0]
    return zlib.decompress(b[i + header:])


class Archive:
    """One of the Game/<name>Ebl.bhd/.bdt archives."""

    def __init__(self, game_dir, name="GameData"):
        n, e = _load_rsa_public_key(f"{game_dir}/{name}KeyCode.pem")
        with open(f"{game_dir}/{name}Ebl.bhd", "rb") as f:
            self.bhd = _rsa_decrypt(f.read(), n, e)
        assert self.bhd[:4] == b"BHD5", "unexpected BHD header"
        self.bdt_path = f"{game_dir}/{name}Ebl.bdt"
        bucket_count, buckets_offset = struct.unpack_from("<II", self.bhd, 16)
        self.entries = {}
        for i in range(bucket_count):
            count, offset = struct.unpack_from("<II", self.bhd, buckets_offset + i * 8)
            for j in range(count):
                h, size, data_offset, _sha, aes_offset = struct.unpack_from(
                    "<IIqqq", self.bhd, offset + j * 0x20)
                self.entries[h] = (size, data_offset, aes_offset)

    def read(self, path):
        h = path_hash(path)
        if h not in self.entries:
            raise KeyError(path)
        return self.read_hash(h)

    def read_hash(self, h):
        size, offset, aes_offset = self.entries[h]
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
        return undcx(bytes(data))


def tpf_textures(d):
    """Return [(name, dds_bytes)] of a PC TPF texture pack."""
    assert d[:4] == b"TPF\0"
    count = struct.unpack_from("<I", d, 8)[0]
    utf16 = d[0x0E] == 1
    out = []
    for i in range(count):
        offset, size, _fmt, _type, _mips, _flags, name_offset = struct.unpack_from("<IIBBBBI", d, 0x10 + i * 0x14)
        end = name_offset
        if utf16:
            while d[end:end + 2] != b"\0\0":
                end += 2
            name = d[name_offset:end].decode("utf-16le", "replace")
        else:
            while d[end] != 0:
                end += 1
            name = d[name_offset:end].decode("ascii", "replace")
        out.append((name, d[offset:offset + size]))
    return out


def icon_textures(archive):
    """Map every single-texture item icon pack ("*_IC_<10 digit id>") to its DDS data."""
    icons = {}
    for h, (size, offset, aes_offset) in archive.entries.items():
        if size > 2_000_000:
            continue
        if not aes_offset:
            with open(archive.bdt_path, "rb") as f:
                f.seek(offset)
                if f.read(4) not in (b"TPF\0", b"DCX\0"):
                    continue
        try:
            d = archive.read_hash(h)
        except Exception:
            continue
        if d[:4] != b"TPF\0" or struct.unpack_from("<I", d, 8)[0] != 1:
            continue
        name, dds = tpf_textures(d)[0]
        if "_IC_" in name:
            icons[name] = dds
    return icons


def read_regulation(game_dir):
    with open(f"{game_dir}/enc_regulation.bnd.dcx", "rb") as f:
        raw = f.read()
    iv = bytes([0x80]) + raw[:11] + b"\0\0\0\1"
    data = AES.new(REGULATION_KEY, AES.MODE_CTR, nonce=b"", initial_value=iv).decrypt(raw[32:])
    return undcx(data)


def parse_bnd4(b):
    assert b[:4] == b"BND4"
    count = struct.unpack_from("<I", b, 0x0C)[0]
    header_size = struct.unpack_from("<Q", b, 0x20)[0]
    unicode = b[0x30] == 1
    files = {}
    for i in range(count):
        o = 0x40 + i * header_size
        size = struct.unpack_from("<q", b, o + 8)[0]
        if header_size >= 0x24:     # entries with uncompressed size and id
            offset, _file_id, name_offset = struct.unpack_from("<IiI", b, o + 0x18)
        else:
            offset, name_offset = struct.unpack_from("<II", b, o + 0x10)
        end = name_offset
        if unicode:
            while b[end:end + 2] != b"\0\0":
                end += 2
            name = b[name_offset:end].decode("utf-16le")
        else:
            while b[end] != 0:
                end += 1
            name = b[name_offset:end].decode("shift_jis", "replace")
        files[name.replace("\\", "/").split("/")[-1]] = b[offset:offset + size]
    return files


def parse_fmg(d):
    assert d[0] == 0 and d[2] == 1, "expected little endian FMG version 1"
    group_count, _string_count, strings_offset = struct.unpack_from("<III", d, 0x0C)
    out = {}
    for g in range(group_count):
        index, first, last = struct.unpack_from("<iii", d, 0x1C + g * 12)
        for k in range(last - first + 1):
            so = struct.unpack_from("<I", d, strings_offset + (index + k) * 4)[0]
            if so == 0:
                continue
            end = so
            while d[end:end + 2] != b"\0\0":
                end += 2
            out[first + k] = d[so:end].decode("utf-16le")
    return out


def parse_param(b):
    rows = struct.unpack_from("<H", b, 0x0A)[0]
    strings_offset = struct.unpack_from("<I", b, 0)[0]
    headers = []
    for i in range(rows):
        row_id, _pad, data_offset, _name_offset = struct.unpack_from("<iIqq", b, 0x40 + i * 0x18)
        headers.append((row_id, data_offset))
    offsets = sorted({o for _, o in headers})
    out = {}
    for row_id, data_offset in headers:
        later = [o for o in offsets if o > data_offset]
        out[row_id] = b[data_offset:later[0] if later else strings_offset]
    return out
