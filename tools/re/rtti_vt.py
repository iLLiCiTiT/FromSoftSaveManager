"""Resolve MSVC x64 RTTI: class name -> vtable VAs, from a PE file."""
import struct, re, sys, json
def load(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]; opt = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<Q', d, pe + 24 + 24)[0]
    secs = []
    for i in range(nsec):
        o = pe + 24 + opt + i * 40
        name = d[o:o+8].rstrip(b'\0'); vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8)
        secs.append((name, va, vs, ro, rs))
    return d, base, secs
def off2rva(secs, off):
    for n, va, vs, ro, rs in secs:
        if ro <= off < ro + rs: return va + off - ro
def rva2off(secs, rva):
    for n, va, vs, ro, rs in secs:
        if va <= rva < va + max(vs, rs) and rva - va < rs: return ro + rva - va
def resolve(path, pattern):
    d, base, secs = load(path)
    pat = re.compile(pattern)
    tds = {}
    for m in re.finditer(rb'\.\?AV[\w@?$]{3,200}@@', d):
        name = m.group().decode()
        if pat.search(name):
            td_off = m.start() - 0x10
            tds[off2rva(secs, td_off)] = name
    # COLs: signature 1, then offset, cdOffset, pTypeDescriptor(rva), pClassDescriptor(rva), pSelf(rva)
    cols = {}
    for m in re.finditer(rb'\x01\x00\x00\x00', d):
        o = m.start()
        if o % 4: continue
        sig, off_, cd, td, chd, self_ = struct.unpack_from('<IIIIII', d, o)
        if td in tds and self_ == off2rva(secs, o):
            cols.setdefault(tds[td], []).append((self_, off_))
    out = {}
    rdata = [s for s in secs if s[0] in (b'.rdata', b'.data')]
    colva = {base + r: (n, off_) for n, lst in cols.items() for r, off_ in lst}
    for n, va, vs, ro, rs in rdata:
        blob = d[ro:ro+rs]
        for i in range(0, len(blob) - 8, 8):
            p = struct.unpack_from('<Q', blob, i)[0]
            if p in colva:
                cls, off_ = colva[p]
                vt = base + va + i + 8
                fns = []
                for k in range(40):
                    fo = rva2off(secs, vt - base + k * 8)
                    if fo is None: break
                    f = struct.unpack_from('<Q', d, fo)[0]
                    tsec = [s for s in secs if s[0] == b'.text']
                    if not any(base + s[1] <= f < base + s[1] + s[2] for s in tsec): break
                    fns.append(hex(f))
                out.setdefault(cls, []).append({'vtable': hex(vt), 'offset': off_, 'fns': fns})
    return out
if __name__ == '__main__':
    res = resolve(sys.argv[1], sys.argv[2])
    json.dump(res, open(sys.argv[3], 'w'), indent=1)
    for k, v in sorted(res.items()):
        print(k, [(x['vtable'], x['offset'], len(x['fns'])) for x in v])
