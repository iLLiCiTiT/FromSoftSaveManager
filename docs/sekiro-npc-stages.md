# Sekiro: NPC questline stages and steps

What each NPC flag shown in the World tab means (tables: `NPCS`, `NPC_STAGES`, `NPC_STEPS`
in `tools/sekiro_flags/gen_flags.py`; flag layout: `docs/event-flags.md`, Sekiro).

## How the blocks work

- Every NPC owns 20 global flags (1000-1999, base % 20 == 0). Its map scripts clear base ..
  base + 14 (Batch Set Event Flags off) and set one flag: the questline stage. Most blocks
  have a common_func event per NPC (20006100 .. 20006140) that moves the stage on, plus map
  events for one-off moves.
- common_func 20006002 (entity, flag) sets base + 19 when the character dies. Questline
  events also set base + 19 when an NPC leaves for good (Anayama leaving, Kotaro leaving with
  him, the Blackhat Badger gone after the Divine Dragon), so it reads "dead or gone". A death
  does not clear the stage, so a dead NPC still shows its last stage.
- common_func 20006000 / 20006007 (entity, flag A, flag B, ..., range) set base + 15 or
  base + 16 when the NPC is attacked enough and switch it to team 27 (hostile).
- Stage texts come from what the setting event waits for. Talk flags (7xxxxxxx) are set by
  the talk scripts (`/script/talk/<map>.talkesdbnd.dcx`): either `SetEventFlag(flag, 1)`
  (ESD command 1:11) or a call to one of the script's own talk machines with the line's
  TalkParam id and the flag (e.g. `x26(23100300, 71110150, 0, 1)`); where the trigger is only
  a talk flag the text quotes that line (TalkParam +0 -> `会話.fmg`).
- Steps: the block keeps only the current stage, so a questline step lists its own stage
  and every stage the scripts can only reach after it. Boss, ending and talk flags stay set.

## Shared flags

| flag | meaning | source |
|---|---|---|
| 8301 | the player has the Mortal Blade, the Lotus of the Palace and the Shelter Stone | common event 910 (IfPlayerHasItem 3:4, goods 2400, 2500, 2501) |
| 8302 | Divine Dragon defeated | common event 911, waits for 9310 (m25_00 12505880, health bar "Divine Dragon") |
| 9300 | Genichiro Ashina (prologue fight on the castle top) | m11_02 11125800 |
| 9302 | Lady Butterfly defeated | m10_00 11005800, HandleBossDefeat 1000800 |
| 9303 | Genichiro, Way of Tomoe defeated | m11_01 11115800, HandleBossDefeat 1110801 |
| 9308 | Great Shinobi Owl defeated | m11_01 11115850, character 1110850 dead |
| 9312 | Isshin, the Sword Saint defeated | m11_02 11125860 |
| 9315 | Emma, the Gentle Blade defeated | m11_01 11115900, HandleBossDefeat 1110900 |
| 9316 | Isshin Ashina defeated | m11_01 11115920, character 1110920 dead |
| 9317 | Owl (Father) defeated | m10_00 11005900, HandleBossDefeat 1000900 |
| 6830 | Ending: Immortal Severance | m11_02 11120110 when 11125707 is set |
| 6831 | Ending: Purification | m11_02 11120110 when 11125708 is set |
| 6832 | Ending: Return | m11_02 11120110 when 11125709 is set |
| 6833 | Ending: Shura | m11_01 11110100 after Isshin Ashina |

The endings: after Isshin, the Sword Saint, Kuro's talk script in the pampas field
(t112111, MSB event `Talk_皇子_すすき野原`) offers up to three choices (ESD 1:19 / 5:19,
`イベントテキスト.fmg`): 14000100 "Give Divine Dragon's Tears" sets 11125707, 14000101 "Give
Dragon's Tears and Everblossom" (only with goods 2503 Aromatic Flower) sets 11125708,
14000102 "Give Dragon Tears and Frozen Tears" (only with goods 9091 Frozen Tears) sets
11125709. Event 11120110 then sets 20 / 21 / 22 and 6830 / 6831 / 6832 and awards
achievements 9 / 10 / 11; the Shura event awards 12 and sets 23 / 6833. 20 .. 23 are cleared
in New Game+ while 6830 .. 6833 stay (the test character, on a later cycle, has 6832 and
none of 20 .. 23), so the app shows the 683x flags: endings reached on this character in any
cycle.

## NPC names

NpcParam +0x8 (`NPC名.fmg`) of the characters the events act on: Sculptor, Kuro the Divine
Heir, Divine Child of Rejuvenation, Hanbei the Undying, Anayama the Peddler, Emma the
Physician, Isshin Ashina, Blackhat Badger, Basket Wearer Shosuke, Fujioka the Info Broker,
the six Memorial Mobs
(the boss names "Owl (Father)", "Great Shinobi - Owl" and "Emma, the Gentle Blade" are their
health bar names). The others have name id 0 and are named by their own dialogue (each
speaker's lines share a 100000 id range in `会話.fmg`) and the MSB talk event of their talk
script (`EVENT_PARAM_ST` entries named `Talk_<name>`):

| block | name | talk event | line |
|---|---|---|---|
| 1120 | Inosuke's Mother | `Talk_鈴ばばあ` (t110160) | "Oh... is that you, Inosuke?" |
| 1160 | Inosuke Nogami | `Talk_鈴ばばあの息子` (t110181, t100180) | "My name is Inosuke Nogami." (18010004) |
| 1360 | Faithful One | `Talk_八百比丘尼` (t111290, t130290, t170290, t200290) | no name in her lines; named after her key item "Rot Essence: Faithful One" (goods 9516, cure event 9059 checks block 1360) |
| 1240, 1340 | Owl | `Talk_忍軍の長` (t100120, t111120) | "As your father, I order you to forsake your master." |
| 1420 | Jinzaemon Kumano | `Talk_魅了された侍` (t112300, t130300, t150300) | "My name is Jinzaemon Kumano." (30010005) |
| 1440 | Kotaro | `Talk_さらわれ下男` (t200260, t130260) | Anayama: "Take a look at Kotaro here." |
| 1460 | Doujun | `Talk_不死実験の医僧` (t130600) | "I am Doujun." (31010003) |
| 1660 | Tengu of Ashina | `Talk_スキル爺(剣聖)` (t110240) | "I am the Tengu of Ashina!" (24100016) |

The Memorial Mobs (1800 .. 1900) are tied to their names by their Rot Essence key items
(Crow, Wartorn, Jail, Toxic, Pious, Drunk Mob: cure events 9060 .. 9065 check blocks 1800 ..
1900) and their maps (MSB enemy parts with those NpcParam names); their stage text is the
nearest Sculptor's Idol.

Blocks left out: 1140 / 1180 (Inosuke and his mother in the Hirata Estate memory, a single
stage each), 1220 (Anayama in the Hirata Estate), 1300, 1400, 1480, 1500, 1560, 1600, 1620,
1640, 1740, 1760, 1780, 1940, 1980 (side characters, Pot Nobles, the Fountainhead maids) and 1520 / 1540 (Lady Butterfly and
Gyoubu, bosses).

## Stages

`+k` is the NPC's own stage flag base + k; triggers are the conditions of the setting event.

### Sculptor (1000-1019)

| flag | stage | trigger |
|---|---|---|
| 1000 | Dilapidated Temple | starting state (common 50, 20006100) |
| 1001 | After the Divine Dragon | 1000 AND 8302 (20006100) |

Dead or gone: 1019.

### Kuro, the Divine Heir (1020-1039)

| flag | stage | trigger |
|---|---|---|
| 1020 | Ashina Castle, his room | starting state (20006110) |
| 1021 | Prologue: escaping the castle with you | prologue state 8306, talk 71120102 "My Lord." (m11_02 11125202) |
| 1027 | Prologue: taken by Genichiro | 9300 in the prologue (m11_02 11125800) |
| 1022 | His room, Genichiro defeated | 9303 (m11_01 11115712) |
| 1026 | After the Mortal Blade, Lotus, Shelter Stone | 1022 AND 8301 (20006110) |
| 1025 | After the Great Shinobi Owl | 1026 AND 9308 (m11_01 11115711) |
| 1023 | After the Divine Dragon | 1025 AND 8302 (20006110) |

Steps: the chain above (1022 -> 1026 -> 1025 -> 1023) and the four endings.

### Divine Child of Rejuvenation (1040-1059)

| flag | stage | trigger |
|---|---|---|
| 1040 | Senpou Temple, inner sanctum | starting state (20006111) |
| 1041 | Senpou Temple, after "This is for you." | 1040 AND 72000361 AND 72000362 (t200220 "This is for you.") |
| 1042 | Has the Holy Chapter: Dragon's Return | 1041 AND 72000015 (t200222 "I... don't want to lose them.") AND has goods 9209 |
| 1043 | After "Yes, I've found them." | 1042 AND 72000017 (t200220) (20006111, m20_00 12005702) |
| 1044 | Writhing in pain beyond the door | 1043 AND 72000019 (t200223 "(Divine Child is writhing in pain beyond the door)") AND 8415 (Kuro, t111111 "...how is the wound?") |

### Hanbei the Undying (1060-1079)

| flag | stage | trigger |
|---|---|---|
| 1060 | Ashina Outskirts | starting state (20006118) |
| 1061 | Ashina Outskirts, after "I will... of course." | 1060 AND 71100240 (t110130) |
| 1062 | Ashina Outskirts, later | 1061 AND 71100245 |

### Inosuke's Mother (1120-1139)

| flag | stage | trigger |
|---|---|---|
| 1120 | Ashina Outskirts | starting state (20006101) |
| 1121 | Ashina Outskirts, after Lady Butterfly | 1120 AND Inosuke's block 1160 AND 9302, neither sick (20006101) |
| 1122 | Ashina Outskirts, later | 1159 (the Hirata Estate memory's block, set by its talk script) |

1139 is set together with 1121 and when she dies, so it is not shown as a state.

### Inosuke Nogami (1160-1179)

| flag | stage | trigger |
|---|---|---|
| 1160 | Ashina Outskirts | starting state (20006103) |
| 1161 | Ashina Outskirts, after Lady Butterfly | set with his mother's 1121 (20006101) |
| 1162 | Ashina Outskirts, later | 1199 (the Hirata Estate memory's block, set by its talk script) |

1179 is set together with 1161 / 1162 and when he dies, so it is not shown as a state.

### Anayama the Peddler (1200-1219)

| flag | stage | trigger |
|---|---|---|
| 1200 | Ashina Outskirts | starting state (20006105) |
| 1201 | With Kotaro, after the Divine Dragon | 1200 AND 8302 AND 71100283 (Kotaro sent to him) |
| 1202 | Gone, killed in the Hirata Estate | 1239 (his Hirata Estate self dead) |
| 1214 | Gone, after the Divine Dragon | 1200 AND 8302 without Kotaro |

71100283 is the last of his talk ladder 71100280 .. 71100286 (20006105): set when Kotaro was
told "There's a peddler named Anayama near Ashina Castle looking for help." (72009001, t200260).

### Owl (Father), Hirata Estate (1240-1259)

| flag | stage | trigger |
|---|---|---|
| 1240 | Hirata Estate | starting state (20006108) |
| 1241 | Hirata Estate, after "Father..." | 1240 AND 71000101 (t100120) |
| 1242 | After Lady Butterfly and the Owl | 9308 AND 9302 |

Dead or gone: 9317.

### Emma, the Gentle Blade (1260-1279)

| flag | stage | trigger |
|---|---|---|
| 1262 | Dilapidated Temple | starting state (20006114) |
| 1260 | Dilapidated Temple, later | 1262 AND 131 |
| 1261 | Ashina Castle | 1260 or 1262, AND 9403 (set by the Genichiro, Way of Tomoe event, m11_01 11115751) |
| 1263 | After Isshin asked about the Mortal Blade | 1261 AND 8400 (Isshin, t111240 "Where is this Mortal Blade?") |
| 1265 | Ashina Castle, after "Purification?" | 1263 AND 71110165 (t110230 / t111230) |
| 1268 | Ashina Castle, after Purification | 1265 AND 71110164 |
| 1270 | Shura: after Isshin's death | 9316 AND 71110020 (m11_01 11115910) |
| 1272 | After the Divine Dragon | 8302 |

Hostile: 1276 (the Shura fight). Dead or gone: 9315.

### Isshin Ashina (1280-1299)

| flag | stage | trigger |
|---|---|---|
| 1280 | Ashina Castle watchtower | starting state (20006113) |
| 1281 | Ashina Castle watchtower, you know where he is | 1280 AND (Kuro's 71110059 "Lord Isshin is not within this castle citadel. I'm told he is recuperating in one of these watchtowers." OR the Tengu's 71100500 / 71100501) (m11_01 11115762) |
| 1282 | After the Divine Dragon | 1281 AND 8302 |
| 1290 | Shura: fought you | 9316 (m11_01 11115930) |

Hostile: 1296. Dead or gone: 1299 (set by the Isshin Ashina and Sword Saint events).

### Great Shinobi Owl, Ashina Castle (1340-1359)

| flag | stage | trigger |
|---|---|---|
| 1340 | Ashina Castle | starting state (20006107) |
| 1341 | After the Mortal Blade, Lotus, Shelter Stone | 9303 AND 8301 |
| 1342 | Fought you | 9308, or after "...No, I cannot." (71110021, m11_01 11115860) |
| 1354 | Defeated | 9308 (m11_01 11115850) |

Hostile: 1356. Dead or gone: 1359.

### Faithful One (1360-1379)

| flag | stage | trigger |
|---|---|---|
| 1360 | Praying for the Divine Child | starting state (20006112, in m11_01, m13_00, m17_00, m20_00) |
| 1361 | After you met the Divine Child | 1360 AND (72000041, the Divine Child's "Shinobi of the Divine Heir.", OR 71110102, "Did you meet with the Divine Child of the rejuvenating waters?") OR (8301 AND 72000101) |
| 1363 | After the Divine Child's "Yes, I've found them." | 1361 AND 72000042 (t200220) |

Dead or gone: 1379.

### Blackhat Badger (1380-1399)

| flag | stage | trigger |
|---|---|---|
| 1380 | Ashina Castle | starting state (20006115, MSB `Talk_白らっぱ`) |
| 1381 | Senpou Temple | 1380 AND (71110308 "Well, that's that..." AND 138, or 8301) (20006115, m20_00 12005741) |
| 1382 | Senpou Temple, at the cliff | 1381 AND 71110300 AND reached the cliff area (72000249, m20_00 12005742; talk event `Talk_白らっぱ_凧揚げ前`, "I need to cross to the other side of the cliff") |
| 1383 | After the Divine Dragon | 8302 AND 72000202 (t200140 "Well, well, if it isn't old chuckles.") |
| 1384 | Gone | 8302 without that talk; also sets 1399 |

Hostile: 1395, 1396 (20006007). Dead or gone: 1399.

### Jinzaemon Kumano (1420-1439)

| flag | stage | trigger |
|---|---|---|
| 1427 | Ashina Reservoir (prologue) | starting state (20006117) |
| 1420 | Ashina Reservoir | 1427 AND 9300 |
| 1426 | Abandoned Dungeon, following the melody | 1420 AND entered the dungeon (71300048) AND 71120150 (t112300 "Hey.") (m13_00 11305702) |
| 1421 | Mibu Village, following the melody | 1426 AND 71300049 (m15_00 11505712) |
| 1422 | Mibu Village, later | 1421 AND 11500200 (m15_00 11505713) |
| 1424 | Abandoned Dungeon, sent to Doujun | 71500192 ("Try the abandoned dungeon.") (m13_00 11305703, 20006120) |
| 1425 | Changed by Doujun's treatment | Doujun's 1463 AND 71300062 AND 1424 (20006120) |

Hostile: 1435, 1436 (20006007). Dead or gone: 1439.

### Kotaro (1440-1459)

| flag | stage | trigger |
|---|---|---|
| 1440 | Senpou Temple, Halls of Illusion | starting state (20006119, `Talk_さらわれ下男_無限回廊`) |
| 1441 | Ashina Outskirts, with Anayama | 72009001 ("There's a peddler named Anayama near Ashina Castle looking for help.") (20006105) |
| 1443 | Abandoned Dungeon, sent to Doujun | 72009002 ("Make your way to the abandoned dungeon of Ashina castle.") (20006120) |
| 1444 | Changed by Doujun's treatment | Doujun's 1463 AND 71300062 AND 1443 (20006120) |
| 1445 | With Anayama, after the Divine Dragon | Anayama's 1201 (20006105) |
| 1447 | Gone with Anayama | Anayama's 1202 / 1214 while 1441 or 1445; also sets 1459 |
| 1442 | Killed at Senpou Temple | he died there (m20_00 12005711 / 12005712) |
| 1446 | Killed at Senpou Temple (white flower) | as 1442, with 72009010 ("Is this the \"pure white flower\"?") |

Dead or gone: 1459.

### Doujun (1460-1479)

| flag | stage | trigger |
|---|---|---|
| 1460 | Abandoned Dungeon | starting state (20006120) |
| 1461 | Abandoned Dungeon, a patient arrived | Jinzaemon's 1424 or Kotaro's 1443 |
| 1462 | After "It's about time I embark..." | 1461 AND 71300056 |
| 1463 | After "H-How is this, Master Dousaku?" | 1462 AND 71300058 |
| 1464 | Treated the patient | 1463 AND 71300062 ("With this, we can advance the treatment even further...") with the patient there |
| 1465 | After "Unh... Aahhh..." | 1464 AND 71300063 |
| 1466 | After "Answer me, Dousaku..." | 1465 AND 71300066; also sets 1476 |

Hostile: 1475, 1476 (m13_00 11305711, attacked). Dead or gone: 1479.

### Basket Wearer Shosuke (1580-1599)

| flag | stage | trigger |
|---|---|---|
| 1580 | Mibu Village | starting state (20006122) |
| 1581 | Mibu Village, talked | 71500043 AND 71500000 (t150270) |

Hostile: 1596 (after "I drank it all...!", m15_00 11505732; `Talk_まともな村人_ゾンビ化`). Dead or gone: 1599.

### Tengu of Ashina (1660-1679)

| flag | stage | trigger |
|---|---|---|
| 1660 | Ashina Castle | starting state (20006113) |
| 1661 | After Isshin asked about the Mortal Blade | 1660 AND 71100505 (t110240) AND 8400 |
| 1674 | After the Mortal Blade, Lotus, Shelter Stone | 1660 or 1661, AND 8301 AND 71100533 (t111242 "Certainly.") |

### Memorial Mobs (1800, 1820, 1840, 1860, 1880, 1900)

One stage each, set when their map first runs (20006129 .. 20006134).

| flag | name | stage |
|---|---|---|
| 1800 | Crow's Bed Memorial Mob | Ashina Outskirts, by the Outskirts Wall |
| 1820 | Battlefield Memorial Mob | Ashina Outskirts, by the castle gate |
| 1840 | Dungeon Memorial Mob | Ashina Castle, by the Abandoned Dungeon |
| 1860 | Toxic Memorial Mob | Sunken Valley |
| 1880 | Shugendo Memorial Mob | Senpou Temple, Shugendo |
| 1900 | Exiled Memorial Mob | Mibu Village |

### Fujioka the Info Broker (1920-1939)

| flag | stage | trigger |
|---|---|---|
| 1920 | Ashina Castle | starting state (20006128, MSB `Talk_荒れ寺ショップ店員`) |
| 1921 | Dilapidated Temple | 1920 AND (71100859 "See ya, pal. Maybe we'll cross paths again someday." OR 8301 OR 9303) |

Hostile: 1935, 1936. Dead or gone: 1939.

## Dragonrot

Each NPC that can sicken has 5 flags (70002000 + 5k). Common event 50 picks the NPCs that
sicken as Dragonrot spreads and sets the group's first flag, then + 1 and + 2 on later
spreads; the NPC's cure event in common.emevd (9050 .. 9066) clears the group (Batch Set
Event Flags off). Each cure event checks one "Rot Essence: <name>" key item (goods 9500 ..)
and the NPC's flag blocks, which tie the group to the NPC; the generator reads this table
from the events (`dragonrot_groups`). The app shows "Dragonrot" while any flag of the
group is set (after Dead or gone and Hostile).

| group | Rot Essence | cure event | NPC block |
|---|---|---|---|
| 70002000 | Sculptor | 9050 | 1000 Sculptor |
| 70002005 | Timid Maid | 9058 | 1120 Inosuke's Mother |
| 70002010 | Fine Son | 9056 | 1160 Inosuke Nogami |
| 70002015 | Newcomer | 9051 | 1200 Anayama |
| 70002020 | Faithful One | 9059 | 1360 Faithful One |
| 70002025 | Lost Child | 9053 | 1440 Kotaro |
| 70002030 | Charmed One | 9054 | 1420 Jinzaemon |
| 70002035 | Surgeons | 9055 | 1460 Doujun |
| 70002040 | Thirsty One | 9057 | 1580 Shosuke |
| 70002045 | Black Hat | 9052 | 1380 Blackhat Badger |
| 70002050 .. 70002075 | Crow, Wartorn, Jail, Toxic, Pious, Drunk Mob | 9060 .. 9065 | 1800 .. 1900 Memorial Mobs |
| 70002080 | Info Broker | 9066 | 1920 Fujioka |

Checked on the test save: the late-game character has no flag of these groups set; a new
character (51 minutes in) has 70002000 and 70002005, the Sculptor and Inosuke's Mother.

## Prosthetic tools

The ten tools are weapons 70000 (Loaded Shuriken), 71000, ... 79000 (Finger Whistle); their
upgrades are 70100, 70200, ... (EquipParamWeapon, `武器名.fmg`). A tool counts as found when
the player's inventory (Gaitem record, PlayerGameData inventory) holds any weapon in its
range of 1000 ids. The test character holds all ten tools and all their upgrades.
