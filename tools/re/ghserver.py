"""Persistent PyGhidra query server for the FromSoft exes.

Usage (from a Python 3.11 venv with the pyghidra wheel from Ghidra's
Ghidra/Features/PyGhidra/pypkg/dist installed):

    python ghserver.py <RE_DIR> <project dir under RE_DIR/ghidra> <project name> <program name>
    e.g. python ghserver.py C:/Users/iLLiCiT/tools/fromsoft-re gh_ds3 ds3 DarkSoulsIII.exe

It opens the Ghidra project once, then runs every RE_DIR/jobs/*.py file it finds (see q.sh)
with these helpers in scope: A(addr), func(addr), decomp(addr), xrefs(addr), callers(addr),
u32/u64(addr), syms(regex), vtable(addr), scalars(values) and the Ghidra program objects
(program, listing, fm, st, rm, mem). Output of each job goes to <job>.out.
"""
import os, sys, time, glob, io, traceback, contextlib
os.environ.setdefault("GHIDRA_INSTALL_DIR", os.path.expanduser(r"~/tools/ghidra_11.4.2_PUBLIC"))
import pyghidra
pyghidra.start()
S = sys.argv[1]; PROJDIR = sys.argv[2] if len(sys.argv) > 2 else "gh2"; PROJ = sys.argv[3] if len(sys.argv) > 3 else "dsr"; EXE = sys.argv[4] if len(sys.argv) > 4 else "DarkSoulsRemastered.exe"
JOBS = os.path.join(S, "jobs")
os.makedirs(JOBS, exist_ok=True)
from ghidra.base.project import GhidraProject
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
proj = GhidraProject.openProject(os.path.join(S, "ghidra", PROJDIR) if os.path.isdir(os.path.join(S, "ghidra", PROJDIR)) else os.path.join(S, PROJDIR), PROJ, True)
program = proj.openProgram("/", EXE, True)
mon = ConsoleTaskMonitor()
di = DecompInterface(); di.openProgram(program)
af = program.getAddressFactory(); listing = program.getListing(); fm = program.getFunctionManager()
st = program.getSymbolTable(); rm = program.getReferenceManager(); mem = program.getMemory()
def A(x): return af.getDefaultAddressSpace().getAddress(x if isinstance(x, int) else int(str(x), 16))
def func(x): return fm.getFunctionContaining(A(x))
def decomp(x, timeout=180):
    f = func(x)
    if f is None: return "// no function at %s" % x
    r = di.decompileFunction(f, timeout, mon)
    return "// ===== %s %s\n%s" % (f.getEntryPoint(), f.getName(), r.getDecompiledFunction().getC() if r.decompileCompleted() else "// failed " + str(r.getErrorMessage()))
def xrefs(x):
    out = []
    for r in rm.getReferencesTo(A(x)):
        f = fm.getFunctionContaining(r.getFromAddress())
        out.append((str(r.getFromAddress()), str(f.getEntryPoint()) if f else None, f.getName() if f else None, str(r.getReferenceType())))
    return out
def callers(x): return xrefs(func(x).getEntryPoint().getOffset())
def u64(x): return mem.getLong(A(x)) & 0xffffffffffffffff
def u32(x): return mem.getInt(A(x)) & 0xffffffff
def syms(pattern):
    import re; p = re.compile(pattern); out = []
    it = st.getSymbolIterator(True)
    while it.hasNext():
        s = it.next(); n = s.getName(True)
        if p.search(n): out.append((n, str(s.getAddress())))
    return out
def vtable(addr, n=120):
    out = []
    for i in range(n):
        p = u64(A(addr).add(i * 8).getOffset())
        if not mem.getExecuteSet().contains(A(p)): break
        f = fm.getFunctionAt(A(p)); out.append((i, hex(p), f.getName() if f else None, f.getBody().getNumAddresses() if f else 0))
    return out
def scalars(vals, fn_filter=None):
    from ghidra.program.model.scalar import Scalar
    vals = set(vals); out = []
    it = listing.getInstructions(True)
    while it.hasNext():
        ins = it.next()
        for op in range(ins.getNumOperands()):
            for o in ins.getOpObjects(op):
                if isinstance(o, Scalar) and o.getUnsignedValue() in vals:
                    f = fm.getFunctionContaining(ins.getAddress())
                    out.append((str(ins.getAddress()), str(f.getEntryPoint()) if f else None, str(ins)))
    return out
G = dict(globals())
print("READY", flush=True)
open(os.path.join(S, "ghserver.ready"), "w").write("ok")
while True:
    for j in sorted(glob.glob(os.path.join(JOBS, "*.py"))):
        buf = io.StringIO()
        try:
            code = open(j, encoding="utf-8").read()
            with contextlib.redirect_stdout(buf):
                exec(code, G)
        except Exception:
            buf.write(traceback.format_exc())
        open(j[:-3] + ".out.tmp", "w", encoding="utf-8").write(buf.getvalue())
        os.replace(j[:-3] + ".out.tmp", j[:-3] + ".out")
        os.remove(j)
    time.sleep(0.3)
