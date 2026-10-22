# Moving characters between save files

Can character slot N of save file A be copied into slot M of save file B, including when A
belongs to a different Steam account? Short answer: yes for all five games. It is
implemented for DSR, DS3, Sekiro and Elden Ring (`src/parse/CharacterCopy.*`, the
"Copy character to..." / "Import character from another save..." context menu of the
character lists). DS2 is not implemented yet, see below.

| Game | Encryption | Integrity | Steam ID stored | Verdict |
|---|---|---|---|---|
| DSR | AES-128-CBC per entry | MD5 per entry | nowhere | implemented |
| DS2 SotFS | AES-128-CBC per entry | MD5 per entry | nowhere (found) | feasible, not implemented |
| DS3 | AES-128-CBC per entry | MD5 per entry + adler32 slot checksum | system entry + every slot | implemented |
| Sekiro | none | MD5 per entry | system entry + every slot | implemented |
| Elden Ring | none | MD5 per entry | system entry + every slot | implemented |

The Steam ID findings come from searching every decrypted entry of the user's saves for the
64-bit Steam ID and its 32-bit account id (DSR's folder name).

## Common container (BND4, `SL2File.*`)

- The header and entry table never change size: every character slot of a game has the same
  fixed entry size, so a copy rewrites entries in place and leaves all offsets alone.
- Entry layout: `md5[16]` followed by the payload. The MD5 covers everything after it.
- Encrypted games (DSR, DS2, DS3): payload = `iv[16] | AES-128-CBC(u32 length | data | padding)`
  with a fixed per-game key, MD5 over `iv + ciphertext`. A rewritten entry keeps its IV and
  padding bytes; only `data` changes, then it is re-encrypted and re-hashed.
- Unencrypted games (Sekiro, ER): payload = data, MD5 over it.
- The game checks the MD5s; a wrong one shows the save as corrupted.

What else could make a game reject the file, and how the copy handles it:

- The system entry's Steam ID must match the logged-in account. The copy never changes the
  destination system entry's Steam ID, so a file keeps working for its own account.
- A character slot whose Steam ID doesn't match its file's is the reason save editors
  rewrite it when importing characters; how each game reacts to a mismatch was not tested
  here. The copy always rewrites it to the destination's.
- Load Game summaries live in the system entry, not the slot: without copying them the
  slot is invisible (occupied flag 0) or shows the old character's name/level.
- Version mismatches: copying from a save written by a newer game patch into one from an
  older patch is not checked. Both files should come from the same game version (in
  practice, both up to date).
- The game overwrites the file from memory when it saves, so it must be closed (or at the
  title screen with no character loaded) while copying.

## Dark Souls Remastered

- Entries: `USER_DATA000..009` characters (0x60000 each), `USER_DATA010` system.
- Steam ID: not stored anywhere in the decrypted data; saves are only tied to the account
  by the folder (`NBGI\DARK SOULS REMASTERED\<account id>`).
- Copy: the slot as is; in `USER_DATA010` the `ProfileSummary` (`occupied[10]`,
  `unk[6]`, 10 x 400 byte entries) occupied byte and entry of the slot.
- No per-slot checksum besides the entry MD5.

## Dark Souls II: Scholar of the First Sin (not implemented)

- 23 entries: `USER_DATA000` = Option + LoadInfo records, `USER_DATA001..010` character
  data, `USER_DATA011..020` the same characters' world data, `USER_DATA021` regulation,
  `USER_DATA022` a second LoadInfo record (version 0x6e, the one SotFS uses).
- Steam ID: not found in any decrypted entry. The BND4 header carries an 8 character
  signature (`14e503cb` in the user's file) that other games have as `00000001`; whether it
  depends on the account was not checked.
- A copy would need both entries of the character (`1 + N` and `11 + N`), the
  `LoadInfoSlot` (0x1f0 bytes) in both LoadInfo records and their slot flags
  (`LoadInfoSlot.flags` bit 0, `LoadInfo.flags`). Not implemented because the LoadInfo
  header flags and the header signature aren't understood yet and couldn't be checked
  against an in-game load. Every slot of the user's DS2 file is in use, so there was also no
  empty slot to try it on.

## Dark Souls III

- Entries: `USER_DATA000..009` characters (0xc0000), `USER_DATA010` system (0x60000),
  `USER_DATA011` regulation (untouched).
- Steam ID: `SystemSlot::steamId` (offset 4) and `CharacterSlotEnd::steamId` at the end of
  every slot (position varies with the slot's sections, found through the typed model).
- Slot checksum: `CharacterSlotHeader::checksum` = adler32 of `[0x5c, end of
  CharacterSlotEnd)` * 2 (`compute_character_checksum`). The Steam ID is inside the range,
  so the checksum is recomputed after rewriting it.
- System entry: `ProfileSummary` (`occupied[10]` + 10 x 0x22a entries).

## Sekiro

- Entries: `USER_DATA000..009` characters (0x100000), `USER_DATA010` system,
  `USER_DATA011` ghost data (untouched). Every entry is a record list.
- Steam ID: system Option record (id 0) payload +4; character GameDataMan record (id 1001)
  payload +4. The copy checks that the slot's id equals its file's before rewriting. Stale
  bytes after the End record may hold another copy; the game doesn't read them.
- System entry: ProfileSummary record (id 2, 0x14fc bytes). Layout from the exe
  (FUN_1407c94a0 writer, FUN_1407c9080 reader): `occupied[10]`, two bytes, then 10 x 0x218
  entries at payload +0xc. The entry's u32 at +0x28 is the "slot in use" flag the app
  reads; the occupied bytes don't match the used slots in the user's file (byte 4 set, slot
  0 used), so the copy carries both over from the source slot unchanged. Note:
  `Sekiro/SaveFile.cpp` reads offsets relative to the payload start with stride 0x218, so
  its `PROFILE_*` constants are entry offsets + 0xc.

## Elden Ring

- Entries: `USER_DATA000..009` characters (0x280000), `USER_DATA010` system,
  `USER_DATA011` regulation (untouched).
- Steam ID: `USER_DATA010` offset 4, and inside each slot after the event flags. Field order
  from ER-Save-Editor (Apache-2.0, `save_slot.rs`): ..., u32 (2 for used slots), 0x20000
  CSNetDataChunks, 0xc weather, 0xc time, 0x10 unknown, steam id. Its offset varies with the
  variable-size fields before it, so the copy searches for the source file's Steam ID and
  keeps only matches with the value 2 at the fixed distance before it; exactly one match is
  required.
- System entry: after version, steam id, 0x140 settings and MenuSystemSaveLoad (u16, u16,
  u32 length, data) come `occupied[10]` and 10 x 0x24c profile summaries.

## Verification

`CharacterCopy` (`tools/character_copy.cpp`) runs the copy and checks the output file:
every entry MD5, all bytes outside the two rewritten entries unchanged, the copied slot equal
to the source except Steam ID/checksum, every slot round-tripping through the SaveData
models, the destination keeping its Steam ID, the slot's summary copied, and the app's
own parser listing the character.

```
CharacterCopy list <file.sl2>
CharacterCopy copy <src.sl2> <srcSlot> <dst.sl2> <dstSlot> <out.sl2>
```

Checked on copies of the user's saves (slots are 0-based here):

| Game | Copy | Bytes of the slot that differ from the source |
|---|---|---|
| DSR | slot 1 -> slot 9 (overwrite) | 0 |
| DS3 | other-account file slot 0 -> empty slot 6 | 8 (checksum, Steam ID) |
| Sekiro | other-account file slot 0 -> empty slot 3 | 4 (Steam ID) |
| ER | other-account file slot 1 -> empty slot 6 | 4 (Steam ID) |
| ER | slot 4 -> slot 0 (overwrite) | 0 |

The "other-account" files were made by replacing every stored Steam ID with
76561198000000001 (and fixing the DS3 checksums and the MD5s). All checks passed.

Not verified: loading a copied character in the games. To check it:

1. Close the game. In the app, right-click a character, "Copy character to...", pick an
   empty slot (or "Import character from another save..." and browse to the other file).
2. Confirm. The app adds a backup labelled "Before copying a character into slot N" to the
   game's backups first (or writes `<file>.<timestamp>.bak` next to a file that isn't one
   of the configured saves).
3. Start the game: the Load Game screen should show the character in that slot; load it and
   check stats, inventory and position. Also load one of the untouched characters.
4. If anything is wrong, restore the backup from the Backups panel.
