# Dark Souls Remastered: NPC questline stages

What each flag of an NPC's event flag block means (blocks: `docs/event-flags.md`, DSR NPCs).

## How the blocks work

- Every questline step is a map script event that waits for its conditions, then clears the
  NPC's whole block (Batch Set Event Flags off) and sets one flag. So **exactly one flag of the
  block is set** at any time (plus the hostile flag, see below), and that flag is the NPC's
  current stage. Checked on the 17 characters of the two current test saves: every block has
  exactly one flag set.
- The first flag of most blocks is set by common.emevd event 50 at the start of the game
  ("not met yet" / starting state).
- The meanings below translate the developers' event names from the `.emeld` files (quoted),
  with the conditions read from the events. The talk scripts never set block flags; they set
  "talked to" flags in the map's own range (e.g. 11410590) that the stage events wait for.
- The hostile flag is set without clearing the block, and Oswald's absolution (common event
  770) clears it again; the dead flag clears the block.
- Flags of a block that no event sets (e.g. Siegmeyer 1495, 1496, 1498-1500, 1508-1510) are
  only read by talk scripts or unused.

## Example: Siegmeyer of Catarina (1490-1539)

| flag | stage | developer name, condition |
|---|---|---|
| 1490 | Not met yet | start of the game |
| 1492 | Sen's Fortress: waiting at the balcony, first meeting | 助けて欲しい2・初対面: Sen's gate opened (11500200), not talked to before |
| 1491 | Sen's Fortress: waiting at the balcony | 助けて欲しい2: gate opened, talked to him earlier (11500591) |
| 1493 | Left Sen's Fortress | 助けて欲しい（王城1離れ）: Iron Golem defeated (11500850) |
| 1494 | Anor Londo: thanked you | 感謝（王城2）: the three enemies near him (1510300-302) dead |
| 1497 | Firelink Shrine: resting | 王城ルートクリア: talked to him in Anor Londo (11510590) |
| 1501 | Blighttown: asleep, needs help | 助けて欲しい（影町）: talked to him at Firelink (11020592), you entered Blighttown |
| 1502 | Blighttown: thanked you | 感謝（影町）: talked to him in Blighttown (11400590) |
| 1503 | Lost Izalith: wants to help (asleep before the Chaos Bugs) | 助けたい（廃都）: you entered Lost Izalith |
| 1504 | Lost Izalith: charging the Chaos Bugs | 突撃する（廃都）: a bug (1410410-413) killed, or you talked (11410590) |
| 1505 | Lost Izalith: thanked you, you killed the bugs | 感謝・PCが倒した |
| 1506 | Lost Izalith: thanked you, near death | 感謝・瀕死: bugs dead, his HP below 50% |
| 1507 | Lost Izalith: thanked you, survived | 感謝・生存: bugs dead, his HP 50% or more |
| 1511 | Died at Ash Lake (hollowed, killed by Sieglinde) | 死亡（地底湖）: 1507, talked in Izalith (11410593), Sieglinde told (11020606) |
| 1512 | Hostile | NPC敵対 / 敵対（廃都） |
| 1513 | Dead | NPC死亡 / 死亡（廃都） / 力尽きる（廃都） (succumbed after 1506) |
| 1514 | Gone | どこにもいない: left Lost Izalith after 1505, or Ash Lake after his death |

## All NPCs

Flags are listed in block order. "Hostile" and "Dead" are the generic NPC敵対 / NPC死亡
events unless noted.

### Solaire of Astora (1000-1029)
- 1000 not met yet
- 1001 Undead Burg: met, "call me any time" (困ったらいつでも呼んでね)
- 1007 Sunlight Altar: offers the Warriors of Sunlight covenant (誓約したい)
- 1008 Anor Londo: met again (久しぶり)
- 1009 Lost Izalith: came looking for his sun (太陽を探しに来た)
- 1002 Lost Izalith: found his sun, the Sunlight Maggot (太陽を見つけた)
- 1003 Lost Izalith: did not find his sun (太陽は見つからなかった)
- 1012 wants to fight alongside you, after Izalith or at the Kiln (共闘したい)
- 1011 dead after finding his sun (死亡（太陽を見つけた）)
- 1004 hostile, 1005 dead

### Darkmoon Knightess (1030-1036)
- 1030 not met yet
- 1031 Anor Londo: wants to talk, before Ornstein and Smough (話したい（クリア前）)
- 1036 Anor Londo: wants to talk, after the Lordvessel (話したい（王の器後）)
- 1033 hostile (敵対（王城2）), 1034 dead

### Oscar, Knight of Astora (1060-1074)
- 1060 starting state (Undead Asylum)
- 1061 hollowed, Asylum revisit (亡者)
- 1062 killed as a Hollow (死亡（亡者）)
- 1073 killed in the Asylum (NPC死亡（チュートリアル）)

### Big Hat Logan (1090-1109)
- 1090 not met yet (Sen's Fortress cell)
- 1091 Sen's Fortress: thanks you for freeing him (感謝する)
- 1092 Firelink Shrine: teaches sorceries (魔法を教えたい（拠点）)
- 1093 Duke's Archives: wants to escape his cell (脱出したい（結晶塔）)
- 1094 Duke's Archives: thanked you (感謝（結晶塔）)
- 1095 Duke's Archives: teaches sorceries (魔法を教えたい（結晶塔）)
- 1099 turned hostile in conversation, gone mad (会話で敵対)
- 1096 hostile, 1097 dead

### Griggs of Vinheim (1110-1119)
- 1110 not met yet (locked in the Undead Burg)
- 1111 Undead Burg: thanked you (感謝)
- 1112 Firelink Shrine: teaches sorceries (下位, basic)
- 1113 Firelink Shrine: teaches sorceries (上位, advanced)
- 1117 hollowed, hostile (敵対（亡者）)
- 1114 hostile, 1115 dead

### Dusk of Oolacile (1120-1139)
- 1120 not met yet (inside the Golden Crystal Golem)
- 1121 Darkroot Basin: thanked you (感謝)
- 1122 summon sign available, "call me any time" (いつでも呼んで)
- 1123 teaches sorceries (魔法を教えたい)
- 1127 needs help, taken to Oolacile (助けてほしい（過去森）)
- 1128 Oolacile: thanked you (感謝（過去森）)
- 1129 after Oolacile: "call me any time" (いつでも呼んで（過去森後）)
- 1130 after Oolacile: teaches sorceries (魔法を教えたい（過去森後）)
- 1126 gone (どこにもいない)
- 1125 dead (also 亡国の王女 死亡 in Oolacile)

### Anastacia of Astora, Fire Keeper (1140-1169)
All four stage events run when she dies; she can be revived with her soul.
- 1140 starting state
- 1141 killed, humanity lost (人間性喪失)
- 1146 "waiting" (待っている)
- 1142 "wants to talk" (話したい)
- 1145 "good luck" (頑張って)

### Rhea of Thorolund (1170-1189)
- 1170 not met yet
- 1171 Firelink Shrine: arrived with Petrus, Vince and Nico (白の御一行 到着)
- 1173 left for the Catacombs (旅立つ / 危機迫る)
- 1174 Tomb of the Giants: thanked you (感謝)
- 1175 teaches miracles, Undead Parish (奇跡を教えたい)
- 1181 hollowed, hostile (敵対（亡者）), Duke's Archives
- 1176, 1179 hostile (1179: attacked without resisting)
- 1177 dead, 1180 dead (killed without resisting)

### Petrus of Thorolund (1190-1209)
- 1202 starting state
- 1192 left with Rhea (旅立つ)
- 1193 came back alone (逃げてきた)
- 1194 showed his true nature (本性だした)
- 1195 hostile after the betrayal, 1197 hostile
- 1196 dead after the betrayal, 1198 dead

### Vince of Thorolund (1210-1219), Nico of Thorolund (1220-1229)
- 1210 / 1220 not met yet
- 1211 / 1221 Firelink Shrine: arrived (到着)
- 1216 / 1226 left with Rhea (旅立つ / 危機迫る)
- 1213 / 1223 hostile, 1214 / 1224 dead

### Gwynevere (1230-1239)
- 1230 starting state, 1232 dead

### Laurentius of the Great Swamp (1250-1259)
- 1250 not met yet (in the barrel in the Depths)
- 1251 Depths: thanked you (感謝)
- 1252 Firelink Shrine: teaches pyromancy (呪術を教えたい)
- 1256 found out about Chaos, went to Blighttown (クラグを見つける)
- 1257 hollowed, hostile in Blighttown (敵対（影町）)
- 1253 hostile, 1254 dead

### Fair Lady (1270-1279)
- 1270 starting state, 1272 dead

### Eingyi (1280-1289)
- 1280 not met yet
- 1281 cold, before you become an egg-bearer (ツン)
- 1286 friendly (デレ)
- 1282 hostile, revenge for the Fair Lady (敵対（復讐）), 1283 hostile, 1287 hostile after being friendly
- 1284 dead

### Quelana of Izalith (1290-1309)
- 1290 not met yet
- 1291 teaches pyromancy (呪術を教えたい)
- 1292 leaves (いなくなる)
- 1293 thanks you for defeating the Bed of Chaos (倒してくれて感謝)
- 1296 gone (どこにもいない)
- 1294 hostile, 1295 dead

### Ingward (1310-1319)
- 1310 not met yet
- 1311 gives you the Key to the Seal (鍵を渡したい)
- 1312 asks you to defeat the Darkwraiths (吸魂鬼を倒して欲しい)
- 1313 heals curses, at Firelink Shrine after leaving New Londo (癒したい)
- 1314 hostile, 1315 dead

### Merchants and smiths
- Andre of Astora 1320 starting, 1321 hostile, 1322 dead
- Vamos 1340 / 1341 / 1342
- Giant Blacksmith 1360 / 1361 / 1362
- Rickert of Vinheim 1380 / 1381 / 1382
- Undead Merchant (Male) 1400 / 1401 / 1402
- Undead Merchant (Female) 1410 / 1411 (専用敵対) / 1412
- Crestfallen Merchant 1420 / 1421 / 1422
- Oswald of Carim 1700 / 1701 / 1702

### Domhnall of Zena (1430-1459)
- 1430 not met yet (Depths)
- 1431 left the Depths, sells at Firelink Shrine (交換したい / 売りたい)
- 1434 hostile, 1435 dead

### Crestfallen Warrior (1460-1489)
- 1460 starting state (Firelink Shrine)
- 1464 went hollow, moved to New Londo (亡者になる)
- 1461 hostile, 1462 dead

### Sieglinde of Catarina (1540-1569)
- 1540 not met yet
- 1541 Duke's Archives: locked in a cell, needs help (助けて欲しい)
- 1542 Duke's Archives: thanked you (感謝)
- 1543 can't find her father (父に会えない)
- 1544 Firelink Shrine: searching for her father (父を捜索中)
- 1545 found her father (父に会えた)
- 1546 had to kill her hollowed father at Ash Lake (父を殺した)
- 1549 gone (どこにもいない)
- 1547 hostile, 1548 dead

### Knight Lautrec of Carim (1570-1599)
- 1570 not met yet (Undead Parish cell)
- 1571 freed, thanked you (感謝)
- 1577 not rescued, gone from the cell (助けなかった)
- 1572 Firelink Shrine: looking for his target (ターゲットを探している)
- 1573 out (おでかけ): has gone after the Fire Keeper
- 1578 hostile: left Firelink, invades in Anor Londo (敵対（拠点離れ） / 敵対（王城2）)
- 1574 hostile, 1575 dead

### Shiva of the East (1600-1619), Shiva's Bodyguard (1760-1769)
- 1600 / 1760 starting state
- 1601 Blighttown: offers to sell (売りたい)
- 1766 Shiva and the bodyguard are in Darkroot Garden (同胞に挨拶 / 森庭に出張中)
- 1606, 1607 Painted World invasion state (野武士世界制御)
- 1608 killed in the Painted World, you hold the Chaos Blade; 1609 without it
- 1603 / 1763 hostile, 1604 / 1764 dead

### Patches (1620-1631)
- 1620 not met yet
- 1621 Catacombs: kicked you down, satisfied (落として満足（墓地1）)
- 1623 plans to kick you down, Tomb of the Giants (落としてやる)
- 1624 Tomb of the Giants: kicked you down, satisfied (落として満足（墓地2）)
- 1626 Firelink Shrine: selling (売りたい)
- 1625 hostile, cleric route (聖職者ルート), 1627 hostile
- 1628 dead

### Kingseeker Frampt (1640-1649), Darkstalker Kaathe (1670-1678)
- 1640 / 1670 starting state
- 1641 about to wake (起きそう)
- 1642 / 1671 gives the first task (使命を与えたい1)
- 1643 / 1672 wants you to place the Lordvessel (王の器を置きたい)
- 1644 / 1673 gives the second task (使命を与えたい2)
- 1645 / 1674 pleased, task done (達成してくれて嬉しい)
- 1649 / 1678 lamenting, you chose the other serpent (嘆かわしい)
- 1647 / 1677 gone, disappointed or attacked (いなくなる)
- 1646 / 1675 hostile (set by the talk scripts, cleared by absolution)
- 1650 Frampt asleep (toggled now and then, outside his block)

### Alvina of the Darkroot Wood (1710-1712)
- 1710 starting state
- 1711 gone (どこにもいない)
- 1712 betrayed (裏切られた)

### Oolacile
- Hawkeye Gough 1820 starting, 1821 thanked you (感謝), 1822 hostile, 1823 dead
- Marvellous Chester 1840 starting, 1841 hostile, 1842 dead
- Lord's Blade Ciaran 1860 starting, 1861 wants to offer (供えたい, at Artorias' grave), 1862 thanked
  you (感謝), 1865 gone (どこにもいない), 1863 hostile, 1864 dead
- Elizabeth 1870 starting, 1872 dead (also dies when Dusk is dead: 宵闇が死んでたら死ぬ)
