# Reverse-engineering handbook

How the save formats, item databases and event flags in this repo were recovered from the
games, and how to pick the work up again. Companion to `docs/event-flags.md`.

## Workspace (outside the repo)

`C:\Users\iLLiCiT\tools\fromsoft-re` (RE_DIR):

| path | contents |
|---|---|
| `ghidra\gh2` | DSR project `dsr`, program `DarkSoulsRemastered.exe` (fully analyzed) |
| `ghidra\gh_ds2` | DS2 SotFS project `ds2`, program `DarkSoulsII.exe` |
| `ghidra\gh_ds3` | DS3 project `ds3`, program `DarkSoulsIII.exe` (fully analyzed, Arxan) |
| `ghidra\gh_sk` | Sekiro project `sekiro`, program `sekiro_unpacked.exe` (Steamless output) |
| `bin\<game>` | the exact exe copies the projects were made from |
| `third_party\steamless` | Steamless 3.1 (`Steamless.CLI.exe --keepbind <exe>` removes SteamStub) |
| `third_party\bindertool` | BinderTool `DecryptionKeys.cs` (DS3 archive RSA keys) and `DictionaryDS3.csv` |
| `third_party\er-save-editor` | ER-Save-Editor (Apache-2.0) db files used by `tools/er_flags` |
| `venv` | Python 3.11 with pyghidra + `tools/re/requirements.txt` |

Ghidra 11.4.2 is at `C:\Users\iLLiCiT\tools\ghidra_11.4.2_PUBLIC`. Only JDK 25 is installed,
which breaks Ghidra's Java script loader, so everything goes through PyGhidra.
`analyzeHeadless.bat` fails on paths containing `(x86)`: copy an exe elsewhere before importing.
A full analysis of a 50-90 MB exe takes hours; importing without analysis and creating
functions on demand (see `mk` below) is usually enough.

## Query server

```
C:\Users\iLLiCiT\tools\fromsoft-re\venv\Scripts\python.exe tools\re\ghserver.py ^
    C:\Users\iLLiCiT\tools\fromsoft-re gh_ds3 ds3 DarkSoulsIII.exe
```

Then send Python jobs (bash, `RE_DIR` defaults to the folder above):

```
tools/re/q.sh probe <<'EOF'
print(decomp(0x1404c5350))
print(xrefs(0x144752f68)[:10])
EOF
```

Helpers inside jobs: `A`, `func`, `decomp(addr, timeout)`, `xrefs`, `callers`, `u32`, `u64`,
`syms(regex)`, `vtable(addr)`, `scalars(values)` (slow on big exes), plus `program`, `fm`,
`listing`, `mem`, `rm`, `st`. To decompile code Ghidra hasn't turned into a function:

```
from ghidra.app.cmd.function import CreateFunctionCmd
from ghidra.app.cmd.disassemble import DisassembleCommand
tx = program.startTransaction('mk')
DisassembleCommand(A(addr), None, True).applyTo(program, mon); CreateFunctionCmd(A(addr)).applyTo(program, mon)
program.endTransaction(tx, True)
```

Only one job runs at a time; a slow job (e.g. `scalars`) blocks the queue.

Offline helpers in `tools/re` that don't need Ghidra:
- `rtti_vt.py <exe> <regex> <out.json>`: MSVC x64 RTTI class names -> vtables and slots.
- `pexref.py`: `.pdata` function ranges and rel32 reference search (finds callers in Arxan
  protected code that Ghidra did not analyze).
- `sl2g.py`: decrypt `.sl2` entries (`load(path, 'dsr'|'ds2'|'ds3')`).
- `capwin.ps1` / `printwin.ps1` (screenshot a window, the latter works behind full-screen
  games), `clickwin.ps1`, `movewin.ps1`, `sizewin.ps1` (drive the app's window),
  `presskey.ps1` (send a key to a game by scan code).

## Method that worked

1. RTTI names give the save classes (`PlayerGameData`, `EquipGameData`, ...); vtable slot 1 is
   Serialize, slot 2 Deserialize. Decompile the Serialize chain and mirror it in
   `src/parse/<Game>/SaveData.*`, keeping unknown bytes as `unk_*` fields.
2. Find the top-level slot writer from a size or magic constant (DSR 0x60000, DS3 the
   `" RHC"`/`" JBO"` map-state magics, then callers via `pexref`).
3. Verify with the round-trip tools (`DSRRoundTrip`, `DS2RoundTrip`, `DS3RoundTrip`,
   `SekiroRoundTrip`): parse + serialize must reproduce every entry byte for byte.
4. For game data, correlate param fields with known items/screens rather than guessing.

## Per game

### Dark Souls Remastered
- Saves: AES-128-CBC per entry (key in `src/parse/SL2File.cpp`). Slot model: `src/parse/DSR/SaveData.h`.
- Game files are loose under the install folder; params are BND3.

### Dark Souls II SotFS
- Saves are `SaveDataBase` record lists (FUN_1402e5240): 0x20-byte header + payload,
  `_SaveDataEnd` terminator. Model: `src/parse/DS2/SaveData.h`.
- Archives: BHD5 with `.pem` keys next to the game; path hash `h*37`. Regulation
  `enc_regulation.bnd.dcx`: AES-CTR, key `40178130DF0A94543309E171ECBF254C`,
  IV = 0x80 + first 11 bytes + 00000001. Reader: `tools/ds2_items/ds2data.py`.
- Param id list (index -> name): FUN_14048b620.
- Player record: stats order VGR END VIT ATN STR DEX INT FTH ADP; HP at +0x28.
  Inventory record tail = equip table (u16 inventory indices: R1 L1 R2 L2 R3 L3, armor x4,
  rings x4, ammo x4, quick items x10, spells).
- Status icons: `/menu/99.febnd.dcx` `Icon_tex` (9x9 grid, 27 px pitch).

### Dark Souls III
- Saves: AES-128-CBC; slot = 0x5c header + 16 random bytes + sections; checksum
  `adler32 * 2` (FUN_1406299a0). Writers: slot FUN_14062af10, GameDataMan FUN_140598b20,
  system FUN_140599190, PlayerGameData FUN_1405a7b70, gaitems FUN_140a2c310.
- Event flags: manager global 0x144752f68; getter FUN_1404c5350, block lookup
  FUN_1404c7f00, layout builder FUN_1404c6ad0 with the slot table at 0x14278b670; world map
  list FUN_14061f710.
- Archives: BHD5 RSA keys from BinderTool `DecryptionKeys.cs`; regulation `Data0.bdt`:
  AES-256-CBC key `ds3#jn/8_7(rsY9pg55GFN7VFL#+3n/)`, IV = first 16 bytes, then DCX DFLT.
  Reader: `tools/ds3_flags/gen_flags.py` (archives, FMG, EMEVD, MSB).

### Sekiro
- Code needs Steamless first; `.rdata` (PEM archive keys) is readable in the Steam exe.
  Record-list saves without encryption (MD5 only). Model: `src/parse/Sekiro/SaveData.h`.
- DCX KRAK = Oodle via the game's `oo2core_6_win64.dll`. Reader: `tools/sekiro_items/sekirodata.py`.
- Event flag slot table identical to DS3 (file offset 0x298dc80).

### Elden Ring
- Archives: PEM keys in the exe, path hash 64-bit `h*0x85`, DCX ZSTD; regulation
  AES-256-CBC key `99BFFC36…EF99` (full key in `tools/er_items/erdata.py`). Reader: `erdata.py`.
- No Ghidra project yet. Save slot walk to the event flags: `tools/er_flags`, docs/event-flags.md.

## Icons and menus
- Sekiro / Elden Ring item icons: `SB_Icon*` atlases in `/menu/hi/01_common.tpf.dcx` with
  sprite rects in `/menu/hi/01_common.sblytbnd.dcx` (`MENU_ItemIcon_<iconId>`), BC7 via Pillow.
- DS2: item icons from `*_IC_<id>` texture packs, menu art from `/menu/40.febnd.dcx`,
  `/menu/99.febnd.dcx`, `/menu/17.febnd.dcx` (see `tools/ds2_items/gen_items.py`).
