# Event flags in FromSoftware saves

Findings on where each game keeps its event flags (bonfires/graces lit, bosses defeated,
NPC progress) and how a flag id maps to a bit. Verified against the live saves on the
development machine (October 2026).

## Common layout (DSR, DS3, Sekiro): SprjEventFlagMan / FrpgEventFlagMan

A flag id is read as decimal digits `G AA B T NNN`:

| part | value | meaning |
|---|---|---|
| G | `(id / 10000000) % 10` | flag group |
| AA, B | `(id / 100000) % 100`, `(id / 10000) % 10` | map area and block (`mAA_0B`) |
| T | `(id / 1000) % 10` | thousands chunk |
| NNN | `id % 1000` | bit in the chunk |

The saved buffer is a sequence of groups in order 0..9. Each group has a fixed number of
map slots; a slot is 0x500 bytes = 10 chunks of 0x80 bytes (1000 flags each).

```
offset = groupBase[G] + slot(AA, B) * 0x500 + T * 0x80 + (NNN >> 5) * 4
value  = (u32le at offset >> (31 - (NNN & 31))) & 1
```

Slot 0 of a group holds flags whose map part is 0; slot `n + 1` belongs to the n-th map
block in the game's sorted world-map list. That list contains blocks a player may never
visit, so slot numbers have gaps.

### Dark Souls III

- Buffer: `CharacterSlot::eventFlags` (0x33e00 bytes, `src/parse/DS3/SaveData.h`).
- Group slots (table at `0x14278b670` in DarkSoulsIII.exe, layout builder FUN_1404c6ad0):
  group 0: 1, groups 1, 2, 5, 6, 7: 33 each.
- Map slots: m30_00 2, m30_01 3, m31_00 4, m32_00 6, m33_00 8, m34_01 10, m35_00 11,
  m37_00 14, m38_00 15, m39_00 16, m40_00 17, m41_00 18, m45_00 19, m50_00 22, m51_00 23,
  m51_01 24.
- Bonfires: regulation `Data0.bdt` BonfireWarpParam (+0 lit flag, +8 text id in
  `FDP_メニューテキスト`). Implemented in the app (World Progress tab).

### Sekiro

- Buffer: payload of record 1000 (`RecordId::Event`) after a 0x10 byte header and a u32
  size (0x33e00): bytes `[0x14, 0x14 + 0x33e00)`.
- Same group slot table as DS3 (found in sekiro.exe at file offset 0x298dc80).
- Map slots: m10_00 1, m11_00 2, m11_01 3, m11_02 4, m13_00 7, m15_00 8, m17_00 10,
  m20_00 12, m25_00 13.
- Idols: gameparam BonfireWarpParam (+0 flag, +8 text id in `NTC_メニューテキスト`);
  all 55 lit idols of the test character resolve to names.
- Bosses: GameAreaParam row id + 10000000 is the defeat flag (18 bosses, e.g. 11000800).
  The params carry no boss names.

### Dark Souls Remastered

- Buffer: `CharacterSlot::eventFlags` (0x16d00 bytes, `src/parse/DSR/SaveData.h`).
- Groups 0: 1 slot, 1, 5, 6, 7: 18 slots each (group bases 0x0, 0x500, 0x5f00, 0xb900,
  0x11300). This matches the long-standing community layout.
- Map slots: m10_00 1, m10_01 2, m10_02 3, m11_00 4, m12_00 5, m12_01 6, m13_00 7,
  m13_01 8, m13_02 9, m14_00 10, m14_01 11, m15_00 12, m15_01 13, m16_00 14, m17_00 15,
  m18_00 16, m18_01 17 (community list; consistent with the 73-slot buffer size).
- The game data has no bonfire table and no boss names; GameAreaParam lists boss entity
  ids. Named flags would need a curated list.

## Elden Ring

- Region: 0x1bf99f bytes in the character slot, after the storage chest, gestures,
  unlocked regions, ride data, menu profile (0x1008), trophies (0x34), GaItemData
  (8 + 0x1b58 * 16), tutorial data (0x408) and 0x1d unknown bytes.
- Flags live in 1000-flag blocks of 125 bytes; a block's position is its rank in the
  game's block table. Bit order is MSB first (bit 7 of the byte = lowest flag).
- Known positions and the grace/boss lists come from ER-Save-Editor (Apache-2.0,
  github.com/ClayAmore/ER-Save-Editor, `src/db/event_flags.rs`, `graces.rs`, `bosses.rs`):
  314 graces and 142 base-game bosses. On the test save: 35 graces / 7 bosses for an
  early character, 261-297 graces / 109-136 bosses for late-game characters.
- Grace names are also available from regulation BonfireWarpParam + PlaceName.

## Dark Souls II

Not investigated yet. DS2 keeps progress in its own records (SaveDataEvent, 0x21c8 bytes
per character, plus SaveDataObj), not in the layout above.
