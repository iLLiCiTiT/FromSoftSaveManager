# Dark Souls III: NPC questline stages

What each questline flag of an NPC's block means (blocks: `docs/event-flags.md`, DS3 NPCs).

## How the blocks work

- An NPC's block is 20 flags: base .. base + 14 are questline stages, base + 15 .. 19 the
  status (friendly, hostile, dead or gone). Each NPC's map script event clears the stage
  range and sets one flag, so exactly one stage flag is set (checked on the 6 test
  characters).
- DS3 ships no developer event names (the `.emeld` files are not in the archives), so the
  texts were written from what each setting event waits for: the NPC's previous stage plus
  a boss or NPC defeated (global flags 9301-9326 and 8200-8263, named by the character
  whose death sets them), a talk flag the NPC's talk script sets with a line of dialogue
  (talk scripts call shared talk functions, bank 6, with the TalkParam id and the flag; the
  TalkParam row's +0 is the `会話` text id), an item, a bonfire, or another NPC's stage.
- Flags 130-149 are a progress ladder the game sets one after another as the game goes on;
  stages that wait for them are "later".
- The texts are in `NPC_STAGES` in `tools/ds3_flags/gen_flags.py`; the generator fails when
  the scripts set a stage flag without a text or a text names a flag no script sets.
- Some texts are less certain than others: where the trigger is only a talk flag or the
  progress ladder, the text says where the NPC is or that you talked, not more.

## Stages

Triggers are the conditions of the setting event (`+k` = that NPC's stage k, `+15`..`+19`
its status), with the map script it is in.

### Fire Keeper (1000-1019)

| flag | stage | trigger |
|---|---|---|
| 1000 (+0) | Firelink Shrine | +18 AND chr alive (m40_00) |
| 1001 (+1) | Lords of Cinder on their thrones | +0 AND 9210 (m40_00) |
| 1002 (+2) | Lords of Cinder on their thrones, later | +1 AND 145 (m40_00) |

### Ludleth of Courland (1020-1039)

| flag | stage | trigger |
|---|---|---|
| 1020 (+0) | Firelink Shrine | starting state |
| 1021 (+1) | After the Kiln warp | +0 AND 14000110 (m40_00) |

### Yuria of Londor (1040-1059)

| flag | stage | trigger |
|---|---|---|
| 1040 (+0) | Not met yet | +16 AND 70000054 (m37_00) |
| 1041 (+1) | Firelink Shrine: came after Yoel's death | 700 AND +0 AND Yoel levels left <= 0 AND NOT talk flag 74000124 (m37_00); +0 AND Yoel levels left <= 0 AND NOT talk flag 74000124 (m38_00) |
| 1042 (+2) | Firelink Shrine: met | +1 AND 50006193 (m37_00) |
| 1043 (+3) | Firelink Shrine: later | any of +1 .. +2 AND Old Woman of Londor +1 AND Old Woman of Londor +18 (m37_00); +15 AND any of +1 .. +2 AND talk flag 74000124 (m40_00) |
| 1044 (+4) | Left | +1 AND any of Anri of Astora +0 .. Anri of Astora +7 AND Anri of Astora +18 (m37_00); +3 AND (talked: "Ah, what have we here! A pyromancy tome from Izalith!" OR NOT 74000600) (m37_00) |
| 1045 (+5) | Kiln: after the Soul of Cinder | +2 AND 74000640 (m40_00) |

### Yoel of Londor (1060-1079)

| flag | stage | trigger |
|---|---|---|
| 1060 (+0) | Not met yet | +16 AND 70000055 (m31_00) |
| 1061 (+1) | Firelink Shrine: serving you | +0 AND talked: "Would I serve you?" (m31_00); talk flag 73100180 AND +15 AND +0 AND HP of (m31_00) |
| 1062 (+2) | Drew out all your strength | +1 AND Yoel levels left <= 0 (m37_00) |
| 1063 (+3) | Not recruited, gone | +0 AND 138 (m31_00); +0 AND 136 (m37_00) |
| 1064 (+4) | Died at Firelink Shrine | +1 AND 136 (m37_00); +1 AND 138 (m40_00) |

### Company Captain Yorshka (1080-1099)

| flag | stage | trigger |
|---|---|---|
| 1080 (+0) | Anor Londo | starting state |

### Hawkwood the Deserter (1100-1119)

| flag | stage | trigger |
|---|---|---|
| 1100 (+0) | Firelink Shrine | +16 AND 70000057 (m40_00) |
| 1101 (+1) | Firelink Shrine: after the Greatwood or Crystal Sage | +0 AND (Curse-rotted Greatwood defeated OR Crystal Sage defeated) (m40_00) |
| 1102 (+2) | Firelink Shrine: after the Abyss Watchers | +1 AND Abyss Watchers defeated (m40_00) |
| 1103 (+3) | Left Firelink Shrine | +2 AND 74000705 (m33_00); +2 AND talked: "This place is a cesspool of doddering oldfolk and degenerates." (m40_00) |
| 1104 (+4) | Farron Keep: waiting for a duel | +3 AND has item Twinkling Dragon Torso Stone AND 53200900 (m33_00) |

### Sirris of the Sunless Realms (1120-1139)

| flag | stage | trigger |
|---|---|---|
| 1120 (+0) | Not met yet | +16 AND 70000058 (m40_00) |
| 1121 (+1) | Firelink Shrine | +0 AND 133 (m40_00) |
| 1122 (+2) | Firelink Shrine: met | +1 AND talked: "Mm, you're an Unkindled, aren't you." (m40_00) |
| 1123 (+3) | Firelink Shrine: talked again | +2 AND talked: "No! You should have waited!" (m40_00) |
| 1124 (+4) | Irithyll: asks for help against Creighton | +3 AND talked: "Hello again." (m40_00) |
| 1125 (+5) | Creighton defeated | +4 AND Creighton the Wanderer defeated (m40_00) |
| 1126 (+6) | Thanked you for the rescue | +5 AND talked: "Mm, I have not thanked you, for your generous rescue." (m40_00) |
| 1127 (+7) | Holy Knight Hodrick defeated | (+4 OR +6) AND Holy Knight Hodrick defeated (m40_00) |
| 1128 (+8) | Thanked you again | +7 AND talked: "Oh, I am most grateful." (m40_00) |
| 1129 (+9) | After the Twin Princes | +8 AND Lothric, Younger Prince defeated (m40_00) |
| 1130 (+10) | Undead Settlement: her last duel | +7 AND 74000760 AND talk flag 74000756 (m31_00) |
| 1133 (+13) | You joined Rosaria's Fingers | any of +2 .. +8 AND 73500150 (m40_00) |
| 1134 (+14) | Turned against you, a Finger of Rosaria | (+0 OR +1) AND 73500150 (m40_00); +13 AND talked: "Mm, I see that you are now a Finger of Rosaria." (m40_00) |

### Ringfinger Leonhard (1140-1159)

| flag | stage | trigger |
|---|---|---|
| 1140 (+0) | Firelink Shrine | +16 AND 70000059 (m35_00) |
| 1141 (+1) | Firelink Shrine: after the High Wall | +0 AND (bonfire Tower on the Wall OR Vordt of the Boreal Valley defeated) (m40_00) |
| 1142 (+2) | Firelink Shrine: gave you red eye orbs | +1 AND talked: "Use them to pillage embers, and briefly heighten your strength, for your duty." (m40_00) |
| 1143 (+3) | Firelink Shrine: you have a Pale Tongue | +2 AND has item Pale Tongue AND (bonfire Cliff Underside OR Curse-rotted Greatwood defeated) (m40_00) |
| 1144 (+4) | Firelink Shrine: talked about invading | +3 AND talked: "I can see it in your eyes. If you didn't invade, didn't pillage, whatever would you do?" (m40_00) |
| 1145 (+5) | Firelink Shrine: later | +4 AND 13000380 (m40_00) |
| 1146 (+6) | Cathedral of the Deep: you joined Rosaria's Fingers | +5 AND +15 AND talked: "Now invade and pillage all you like." AND 73500150 (m35_00) |
| 1151 (+11) | Cathedral of the Deep: asked about Horace | +6 AND talked: "If you happen upon Horace, please tell him." (m35_00) |
| 1152 (+12) | Took Rosaria's soul | +11 AND Rosaria, Mother of Rebirth +0 AND (bonfire Profaned Capital OR Yhorm the Giant defeated) AND HP of Rosaria, Mother of Rebirth (m35_00) |
| 1153 (+13) | Defeated in Anor Londo | defeated (m37_00) |

### Blacksmith Andre (1160-1179)

| flag | stage | trigger |
|---|---|---|
| 1160 (+0) | Firelink Shrine | +18 AND chr alive (m40_00) |

### Shrine Handmaid (1180-1199)

| flag | stage | trigger |
|---|---|---|
| 1180 (+0) | Firelink Shrine | +18 AND chr alive (m40_00) |

### Greirat of the Undead Settlement (1200-1219)

| flag | stage | trigger |
|---|---|---|
| 1200 (+0) | High Wall: in his cell | +16 AND 70000062 (m30_00) |
| 1201 (+1) | Freed | talked: "Oh, dear brother..." AND +15 AND +0 AND HP of (m30_00) |
| 1202 (+2) | Firelink Shrine: sells his wares | +1 AND talked: "Thank you. You will not be disappointed." (m40_00) |
| 1203 (+3) | Out looting the Undead Settlement | +2 AND 74000316 (m40_00) |
| 1204 (+4) | Back from looting | +3 AND talked: "Mm, a fine choice!" (m37_00) |
| 1205 (+5) | Out looting Irithyll alone | +4 AND 74000318 AND NOT Siegward of Catarina +5 AND NOT Unbreakable Patches +6 (m37_00) |
| 1206 (+6) | Back from Irithyll | +4 AND 74000318 AND (Siegward of Catarina +5 OR Unbreakable Patches +6) (m40_00) |
| 1207 (+7) | Out looting Lothric Castle | +6 AND talked: "Thank you, for placing your trust in me." (m34_01) |

### Orbeck of Vinheim (1220-1239)

| flag | stage | trigger |
|---|---|---|
| 1220 (+0) | Not met yet | +16 AND 70000063 (m33_00) |
| 1221 (+1) | Road of Sacrifices: met | talked: "Ahh, Unkindled One." AND +15 AND +0 AND HP of (m33_00) |
| 1222 (+2) | Firelink Shrine: teaches sorceries | +1 AND 74000553 (m40_00) |
| 1223 (+3) | Firelink Shrine: you brought him scrolls | +1 AND 74000583 (m40_00) |
| 1224 (+4) | After the Twin Princes | +3 AND Lothric, Younger Prince defeated (m34_01) |

### Cornyx of the Great Swamp (1240-1259)

| flag | stage | trigger |
|---|---|---|
| 1240 (+0) | Undead Settlement: in a cage | 70000064 AND +16 (m31_00); +16 AND 70000064 (m40_00) |
| 1241 (+1) | Firelink Shrine: freed | talk flag 73100212 AND +15 AND +0 AND HP of (m31_00) |

### Karla (1260-1279)

| flag | stage | trigger |
|---|---|---|
| 1260 (+0) | Irithyll Dungeon: in a cell | +16 AND 70000065 (m39_00) |
| 1261 (+1) | Firelink Shrine: freed | talked: "Oh, really. You are no ordinary man." AND +15 AND +0 AND HP of (m39_00) |

### Irina of Carim (1280-1299)

| flag | stage | trigger |
|---|---|---|
| 1280 (+0) | Undead Settlement: in a cell | +16 AND 70000066 (m31_00) |
| 1281 (+1) | Firelink Shrine: freed | talked: "Oh, thank you, sweet Champion." AND +15 AND +0 AND HP of (m31_00) |
| 1282 (+2) | Firelink Shrine: given a dark tome | +1 AND (73101710 OR 73101720 OR 73101730 OR 73101740 OR 73101750) (m40_00); talk flag 74000381 AND +15 AND +4 AND HP of (m40_00) |
| 1283 (+3) | Firelink Shrine: given every dark tome | +2 AND talked: "Ahh, ahh, sweet Champion. Where have you been?" AND 73101710 AND 73101720 AND 73101730 AND 73101740 AND 73101750 (m40_00); talk flag 74000382 AND +15 AND +5 AND HP of (m40_00) |
| 1284 (+4) | Firelink Shrine: gone dark | any of Eygon of Carim +1 .. Eygon of Carim +3 AND Eygon of Carim +15 AND any of +2 .. +3 AND +15 AND talked: "Ahh, ahh, sweet Champion. Where have you been?" (m40_00) |
| 1285 (+5) | Firelink Shrine: gone dark | any of Eygon of Carim +1 .. Eygon of Carim +3 AND Eygon of Carim +15 AND any of +2 .. +3 AND +15 AND talked: "Ahh, ahh, sweet Champion. Where have you been?" (m40_00) |
| 1286 (+6) | Firelink Shrine: given no dark tome | +1 AND 74000380 AND NOT 73101710 AND NOT 73101720 AND NOT 73101730 AND NOT 73101740 AND NOT 73101750 (m40_00) |

### Eygon of Carim (1300-1319)

| flag | stage | trigger |
|---|---|---|
| 1300 (+0) | Undead Settlement | +16 AND 70000067 (m31_00) |
| 1301 (+1) | Undead Settlement: met | +0 AND talk flag 73100304 (m31_00) |
| 1302 (+2) | Firelink Shrine: after the Abyss Watchers | +1 AND Abyss Watchers defeated (m40_00); any of +1 .. +3 AND Irina of Carim +18 (m40_00) |
| 1303 (+3) | Firelink Shrine: talked | +2 AND talk flag 74000650 (m31_00) |
| 1304 (+4) | Firelink Shrine: Irina gone dark | any of +1 .. +3 AND +15 AND any of Irina of Carim +2 .. Irina of Carim +3 AND Irina of Carim +15 AND talked: "Ahh, ahh, sweet Champion. Where have you been?" (m40_00) |
| 1305 (+5) | After the Dragonslayer Armour | +3 AND Dragonslayer Armour defeated AND NOT 73101740 AND NOT 73101750 AND NOT 73101760 AND NOT 73101770 AND NOT 73101780 (m31_00); +3 AND Dragonslayer Armour defeated AND NOT 73101710 AND NOT 73101720 AND NOT 73101730 AND NOT 73101740 AND NOT 73101750 (m40_00) |

### Anri of Astora (1340-1359)

| flag | stage | trigger |
|---|---|---|
| 1340 (+0) | Not met yet | +16 AND 70000069 (m33_00) |
| 1341 (+1) | Road of Sacrifices: met | +0 AND +15 AND Horace the Hushed +0 AND Horace the Hushed +15 AND talked: "You! You!" (m33_00); +0 AND talked: "You! You!" (m33_00) |
| 1342 (+2) | Firelink Shrine: after the Deacons | +1 AND Deacons of the Deep defeated (m40_00) |
| 1344 (+4) | Catacombs of Carthus | +2 AND 74000700 (m38_00); +3 AND +15 AND Horace the Hushed +3 AND Horace the Hushed +15 AND 136 (m38_00) |
| 1345 (+5) | Catacombs of Carthus: met again | +15 AND +4 AND (talked: "Yes, I see." OR 73800134) (m38_00); +4 AND (talked: "Yes, I see." OR 73800134) (m38_00) |
| 1346 (+6) | After High Lord Wolnir, Horace hollowed | +15 AND +5 AND talk flag 73800101 AND talked: "Goodness me." AND NOT Horace the Hushed +18 AND High Lord Wolnir defeated (m38_00); +5 AND talk flag 73800101 AND talked: "Goodness me." AND Horace the Hushed +17 AND High Lord Wolnir defeated (m38_00) |
| 1347 (+7) | After High Lord Wolnir | +15 AND +5 AND talk flag 73800101 AND NOT talked: "Goodness me." AND High Lord Wolnir defeated (m38_00); +15 AND +5 AND talk flag 73800101 AND talked: "Goodness me." AND Horace the Hushed +18 AND High Lord Wolnir defeated (m38_00) |
| 1348 (+8) | Irithyll: met again | +15 AND +7 AND talked: "Oh, I thought it might be you. Good to see you." AND Old Woman of Londor +2 AND Old Woman of Londor +15 AND any of Yuria of Londor +1 .. Yuria of Londor +3 (m37_00); +7 AND Old Woman of Londor +2 AND any of Yuria of Londor +1 .. Yuria of Londor +3 (m37_00) |
| 1349 (+9) | Irithyll: wedding | Old Woman of Londor +4 AND has item Sword of Avowal (m37_00) |
| 1350 (+10) | Irithyll: the Old Woman is dead | +15 AND +7 AND talked: "Oh, I thought it might be you. Good to see you." AND Old Woman of Londor +1 AND Old Woman of Londor +18 (m37_00); +7 AND Old Woman of Londor +1 AND Old Woman of Londor +18 (m37_00) |
| 1351 (+11) | After Aldrich | +10 AND Aldrich, Devourer of Gods defeated (m40_00) |
| 1352 (+12) | After Aldrich: left | +11 AND 50006030 AND NOT 8263 (m35_00) |
| 1353 (+13) | After Aldrich: left (Smouldering Lake) | +11 AND 50006030 AND 8263 (m38_00) |

### Unbreakable Patches (1360-1379)

| flag | stage | trigger |
|---|---|---|
| 1360 (+0) | Not met yet | +16 AND 70000070 (m35_00) |
| 1361 (+1) | Cathedral of the Deep: met | +0 AND (Siegward of Catarina +0 OR Siegward of Catarina +2 OR Siegward of Catarina +14) AND Siegward of Catarina +15 AND (talked: "Well, what's happened here?" OR talked: "Beware, the shackles of the Gods are fragile." OR talked: "Right, onto the pile with you. You're family now...") AND 63500201 (m35_00); +0 AND +15 AND (Siegward of Catarina +0 OR Siegward of Catarina +2 OR Siegward of Catarina +14) AND Siegward of Catarina +15 AND (talked: "Well, what's happened here?" OR talked: "Beware, the shackles of the Gods are fragile." OR talked: "Right, onto the pile with you. You're family now...") AND 63500201 (m35_00) |
| 1362 (+2) | Cathedral of the Deep: trapped you | ( OR  OR ) AND +1 AND +15 AND 13500783 (m35_00); +1 AND 13500783 (m35_00) |
| 1363 (+3) | Cathedral of the Deep: trap avoided | OT 13500783 AND (+0 OR +1) AND +15 (m35_00); +1 AND 63500202 (m35_00) |
| 1364 (+4) | Firelink Shrine: arrived | any of +0 .. +3 AND +15 AND (+3 OR talked: " " OR talked: "Of course not, we have just met, after all." OR 138 AND Deacons of the Deep defeated) (m40_00) |
| 1365 (+5) | Firelink Shrine: sells his wares | +4 AND +15 (m40_00); +6 AND Greirat of the Undead Settlement +6 (m40_00) |
| 1366 (+6) | Firelink Shrine: Greirat went to Irithyll | +5 AND Greirat of the Undead Settlement +4 AND talked: "What? Gone all the way to Irithyll, has he?" (m40_00) |
| 1367 (+7) | Firelink Shrine: Greirat went to Lothric | +5 AND Greirat of the Undead Settlement +7 AND talked: "Hmph. Off to Lothric, the castle of no return..." (m40_00) |
| 1370 (+10) | Greirat is dead | chr Greirat of the Undead Settlement dead (m40_00) |

### Siegward of Catarina (1380-1399)

| flag | stage | trigger |
|---|---|---|
| 1380 (+0) | Not met yet | +16 AND 70000071 (m31_00); +15 AND talked: "Well, I'm going to have myself a little nap." (m39_00) |
| 1382 (+2) | Undead Settlement: on the tower | ( OR ) (m31_00) |
| 1383 (+3) | Cathedral of the Deep: met again | (+0 OR +2 OR +14) AND (talked: "Well, what's happened here?" OR talked: "Beware, the shackles of the Gods are fragile." OR talked: "Right, onto the pile with you. You're family now...") AND NOT Unbreakable Patches +18 AND 63500201 (m35_00); Unbreakable Patches +0 AND Unbreakable Patches +15 AND (+0 OR +2 OR +14) AND +15 AND (talked: "Well, what's happened here?" OR talked: "Beware, the shackles of the Gods are fragile." OR talked: "Right, onto the pile with you. You're family now...") AND 63500201 (m35_00) |
| 1384 (+4) | Firelink Shrine | +3 AND talk flag 73500105 (m40_00) |
| 1385 (+5) | Cathedral of the Deep: lost his armour | +3 AND talked: "Ah, oh! My armour!" (m35_00); (+0 OR +2 OR +14) AND (talked: "Well, what's happened here?" OR talked: "Beware, the shackles of the Gods are fragile." OR talked: "Right, onto the pile with you. You're family now...") AND Unbreakable Patches +18 (m37_00) |
| 1386 (+6) | Irithyll: dinner by the fire | (+3 AND talk flag 73500105 OR +5) AND +15 AND 140 AND talked: "Ah, ohh!" (m39_00); (+4 OR +5) AND 140 AND talked: "Ah, ohh!" (m39_00) |
| 1388 (+8) | Irithyll Dungeon: freed | (+6 AND talked: "Ah! You are a saint!" OR +7) AND talked: "Right, onto the pile with you. You're family now..." AND talked: "I know. Won't you join me for dinner?" AND talk flag 73500105 AND NOT Yhorm the Giant defeated (m39_00); any of +16 .. +18 AND +6 AND talked: "Ah! You are a saint!" AND talked: "Right, onto the pile with you. You're family now..." AND talked: "I know. Won't you join me for dinner?" AND talk flag 73500105 (m39_00) |
| 1389 (+9) | Fought Yhorm with you | +8 AND Yhorm the Giant defeated (m39_00); talked: "Well, I'm going to have myself a little nap." AND HP of AND +15 (m39_00) |
| 1390 (+10) | Yhorm defeated | +6 AND Yhorm the Giant defeated AND talked: "Ah! You are a saint!" (m39_00) |
| 1394 (+14) | Undead Settlement: Fire Demon defeated | +16 AND 70000071 AND 13100500 (m31_00); +2 AND talked: "Right, onto the pile with you. You're family now..." (m31_00) |

### High Priestess Emma (1420-1439)

| flag | stage | trigger |
|---|---|---|
| 1420 (+0) | High Wall | +15 AND 50006230 (m30_00) |
| 1421 (+1) | After the Dancer | 711 AND +15 (m30_00) |

### Horace the Hushed (1480-1499)

| flag | stage | trigger |
|---|---|---|
| 1480 (+0) | Not met yet | +16 AND 70000076 (m33_00) |
| 1481 (+1) | Road of Sacrifices: met | Anri of Astora +0 AND Anri of Astora +15 AND +0 AND +15 AND talked: "You! You!" (m33_00); +0 AND talked: "You! You!" (m33_00) |
| 1482 (+2) | Firelink Shrine: after the Deacons | +1 AND Deacons of the Deep defeated (m40_00) |
| 1484 (+4) | Catacombs of Carthus | Anri of Astora +3 AND Anri of Astora +15 AND +3 AND +15 AND 136 (m38_00); +2 AND 74000700 (m38_00) |
| 1485 (+5) | Smouldering Lake: hollowed | Anri of Astora +15 AND Anri of Astora +5 AND talk flag 73800101 AND talked: "Goodness me." AND +18 AND High Lord Wolnir defeated (m38_00); +4 AND +18 AND (Anri of Astora +7 OR Anri of Astora +5 AND (Anri of Astora +15 OR Anri of Astora +16 AND 70000069)) AND talk flag 73800101 AND talked: "Goodness me." AND High Lord Wolnir defeated AND 8263 (m38_00) |

### Rosaria, Mother of Rebirth (1500-1519)

| flag | stage | trigger |
|---|---|---|
| 1500 (+0) | Cathedral of the Deep | OT +1 AND +18 (m35_00) |
| 1501 (+1) | Soul taken by Leonhard | Ringfinger Leonhard +11 AND +0 AND (bonfire Profaned Capital OR Yhorm the Giant defeated) AND HP of (m35_00) |
| 1502 (+2) | Restored | talk flag 73500160 (m35_00) |

### Old Woman of Londor (1560-1579)

| flag | stage | trigger |
|---|---|---|
| 1560 (+0) | Firelink Shrine | starting state |
| 1561 (+1) | After High Lord Wolnir | Anri of Astora +15 AND Anri of Astora +5 AND talk flag 73800101 AND NOT talked: "Goodness me." AND High Lord Wolnir defeated (m38_00); Anri of Astora +15 AND Anri of Astora +5 AND talk flag 73800101 AND talked: "Goodness me." AND Horace the Hushed +18 AND High Lord Wolnir defeated (m38_00) |
| 1562 (+2) | Talked | +1 AND +15 (m37_00) |
| 1563 (+3) | Anri in Irithyll | Anri of Astora +15 AND Anri of Astora +7 AND talked: "Oh, I thought it might be you. Good to see you." AND +2 AND +15 AND any of Yuria of Londor +1 .. Yuria of Londor +3 (m37_00); Anri of Astora +7 AND +2 AND any of Yuria of Londor +1 .. Yuria of Londor +3 (m37_00) |
| 1564 (+4) | Told you of the Lord of Hollows | +3 AND talked: "Heed my words, Unkindled One." (m37_00) |
| 1565 (+5) | Wedding with Anri | +4 AND has item Sword of Avowal (m37_00) |
| 1566 (+6) | Anri in Irithyll | Anri of Astora +15 AND Anri of Astora +7 AND talked: "Oh, I thought it might be you. Good to see you." AND +2 AND +15 AND any of Yuria of Londor +1 .. Yuria of Londor +3 (m37_00); Anri of Astora +7 AND +2 AND any of Yuria of Londor +1 .. Yuria of Londor +3 (m37_00) |
| 1567 (+7) | Anri is dead | any of +1 .. +2 AND Anri of Astora +7 AND Anri of Astora +18 (m37_00) |

### Holy Knight Hodrick (1580-1599)

| flag | stage | trigger |
|---|---|---|
| 1580 (+0) | Undead Settlement | OT +18 AND 13100820 (m31_00) |

### Shrine Handmaid (2) (1600-1619)

| flag | stage | trigger |
|---|---|---|
| 1600 (+0) | Firelink Shrine | starting state |

### Slave Knight Gael (1620-1639)

| flag | stage | trigger |
|---|---|---|
| 1620 (+0) | Cathedral of the Deep | +16 AND 70001051 (m35_00) |
| 1621 (+1) | Cathedral of the Deep: met | +0 AND 73500300 (m35_00) |
| 1622 (+2) | Cathedral of the Deep: talked | +1 AND 73500304 (m35_00) |
| 1623 (+3) | Later in the game | +1 AND 6952 AND 148 (m35_00) |
| 1624 (+4) | Later in the game, talked | +3 AND 73500304 (m35_00) |
| 1625 (+5) | The Painted World | any of +1 .. +2 AND 6952 AND 148 (m45_00) |

### Sir Vilhelm (1640-1659)

| flag | stage | trigger |
|---|---|---|
| 1640 (+0) | Painted World of Ariandel | +16 AND 70001052 (m45_00) |
| 1641 (+1) | Painted World of Ariandel: met | starting state |

### Amnesiac Lapp (1800-1819)

| flag | stage | trigger |
|---|---|---|
| 1800 (+0) | Not met yet | +16 AND 70001073 (m50_00) |
| 1801 (+1) | Dreg Heap: met | +0 AND 75000101 (m50_00) |
| 1802 (+2) | Dreg Heap: remembered | +1 AND 75000131 AND NOT 55000450 AND Stone-humped Hag +15 (m50_00) |
| 1803 (+3) | Dreg Heap: summon sign | +15 AND +1 AND NOT 9324 (m50_00) |
| 1804 (+4) | Dreg Heap: summon sign | +15 AND +2 AND NOT 9324 (m50_00) |
| 1805 (+5) | After the Demon Prince | +1 AND 9324 (m50_00); 9324 AND (+1 OR +3) (m51_00) |
| 1806 (+6) | After the Demon Prince | +2 AND 9324 (m50_00); +5 AND 75000131 AND NOT 55000450 AND Stone-humped Hag +15 (m50_00) |
| 1807 (+7) | Feast with you | any of +5 .. +6 AND 75000104 (m50_00) |
| 1808 (+8) | Off to the Purging Monument | +15 AND +7 AND talked: "Now, I'm off in search of the Purging Monument." (m51_00); +7 AND talked: "Now, I'm off in search of the Purging Monument." (m51_00) |
| 1809 (+9) | Asked for help | +8 AND NOT talked: "Are you certain of that?" AND talked: "...Ahh, please, someone tell me..." AND 15110801 (m51_00) |
| 1810 (+10) | Thanked you | +15 AND +8 AND talked: "Ahh, thank you, most sincerely." (m51_00); +8 AND talked: "Ahh, thank you, most sincerely." (m51_00) |
| 1811 (+11) | Purging Monument | +15 AND +10 (m51_00) |

### Shira, Knight of Filianore (1820-1839)

| flag | stage | trigger |
|---|---|---|
| 1820 (+0) | Ringed City | +16 AND 70001074 (m51_00) |
| 1821 (+1) | Ringed City: gave you a token | +15 AND +0 AND talked: "I have asked a thing most terrible of thee. May this token of thanks be of aid." AND 15100200 AND NOT 9326 (m51_00) |
| 1822 (+2) | After Halflight | +15 AND +0 AND 9326 (m51_00); any of +0 .. +1 AND 9326 (m51_00) |
| 1823 (+3) | Ringed City: later | any of +0 .. +2 AND (15100500 OR 149) (m51_00); +16 AND 70001074 (m51_01) |

### Stone-humped Hag (1860-1879)

| flag | stage | trigger |
|---|---|---|
| 1860 (+0) | Ringed City | +16 AND 70001076 (m50_00) |
| 1861 (+1) | Ringed City: dead | +0 AND +18 (m50_00) |
| 1862 (+2) | Ringed City: later | +0 AND +15 AND 15100500 (m50_00) |
