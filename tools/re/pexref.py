"""Raw PE helpers: function ranges from .pdata, and references to an address via
rel32 operands (call/jmp/lea/mov rip-relative)."""
import struct, bisect, re
class PE:
    def __init__(self, path):
        d = self.d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        nsec = struct.unpack_from('<H', d, pe + 6)[0]; opt = struct.unpack_from('<H', d, pe + 20)[0]
        self.base = struct.unpack_from('<Q', d, pe + 24 + 24)[0]
        exc_rva, exc_size = struct.unpack_from('<II', d, pe + 24 + 112 + 3 * 8)
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + opt + i * 40
            name = d[o:o+8].rstrip(b'\0'); vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8)
            self.secs.append((name, va, vs, ro, rs))
        self.funcs = []
        eo = self.rva2off(exc_rva)
        for i in range(exc_size // 12):
            b, e, u = struct.unpack_from('<III', d, eo + i * 12)
            if b: self.funcs.append((b, e))
        self.funcs.sort(); self.starts = [f[0] for f in self.funcs]
    def rva2off(self, rva):
        for n, va, vs, ro, rs in self.secs:
            if va <= rva < va + rs: return ro + rva - va
    def off2rva(self, off):
        for n, va, vs, ro, rs in self.secs:
            if ro <= off < ro + rs: return va + off - ro
    def func_of(self, va):
        rva = va - self.base
        i = bisect.bisect_right(self.starts, rva) - 1
        if i >= 0 and self.funcs[i][0] <= rva < self.funcs[i][1]: return self.base + self.funcs[i][0]
    def refs_to(self, target_va, text_only=True):
        """Positions whose rel32 operand resolves to target (operand end = next instruction)."""
        out = []
        for n, va, vs, ro, rs in self.secs:
            if text_only and n != b'.text': continue
            blob = self.d[ro:ro+rs]
            base_va = self.base + va
            # rel32 at offset i, instruction ends at i+4 (works for call/jmp E8/E9 and modrm rip forms)
            import numpy as np
            arr = np.frombuffer(blob[:len(blob) - len(blob) % 1], dtype=np.uint8)
            rel = np.frombuffer(blob, dtype='<i4', count=(len(blob)) // 4 * 1) if False else None
            for shift in range(4):
                b = blob[shift:]
                n4 = len(b) // 4
                vals = np.frombuffer(b[:n4*4], dtype='<i4').astype(np.int64)
                idx = np.arange(n4, dtype=np.int64) * 4 + shift
                dest = base_va + idx + 4 + vals
                hits = np.nonzero(dest == target_va)[0]
                for h in hits:
                    out.append(base_va + int(idx[h]))
        return sorted(out)
