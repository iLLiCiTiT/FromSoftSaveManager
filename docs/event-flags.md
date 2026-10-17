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

### Dark Souls II

Not decoded yet. DS2 keeps progress in its own records (SaveDataEvent, 0x21c8 bytes per
character, plus SaveDataObj), not in the layout above, and the record writers are Arxan
protected. Useful game data for a later attempt (regulation `enc_regulation.bnd.dcx`):

- MapObjectBonfireParam (83 rows): map id at +0x0, a text id at +0x1c / +0x24
  (probably `bonfirename.fmg` / area names).
- BossParam (55 rows): boss entries with a flag-like id at +0xc (e.g. 10153000).
- EventFlagListParam (563 rows): row id -> event flag id.
