# Elden Ring: NPC questline stages

What the questline flags of an NPC's block mean (blocks: `docs/event-flags.md`, Elden Ring NPCs).

## How the blocks work

- An NPC's block is 20 flags: base .. base + 4 its status (friendly, hostile, dead),
  base + 5 .. base + 18 its questline, base + 19 set on its own. Stage events clear the
  questline range and set one flag, so one is set (checked on the 5 test characters, all
  55 NPCs). Exceptions: the merchants keep three groups (+5..+9, +10..+14, +15..+18: wares
  and sales), Boc and Corhyn a few side flags (+14..+17) next to their stage; the app takes
  the first set flag that has a text.
- Two kinds of events set stages: one moves an NPC from its previous stage when a boss, NPC,
  item, line of dialogue or another NPC's stage condition is met; the other runs on load and
  picks the stage from world flags with SKIP IF Event Flag chains (e.g. Hewg: +6 once
  flag 110, the Erdtree burned at the Forge of the Giants, is set).
- ER ships no developer event names, so the texts were written from those triggers.
  Global flags are named by what sets them: 9130 / 9412 Radahn's death, 9116 Maliketh's,
  110 the Forge of the Giants, 105 reaching Roundtable Hold, 9410 / 9411 the Radahn
  festival; flags set when a named character dies read "X dead". 76xx are set by the
  shared NPC fight event (common_func 90005796 / 90005797 / 90005799) in a map.
- Flags marked "-" were reviewed and have no text: the trigger is a world flag nothing
  names, or the flag is bookkeeping (every NPC's +18, the merchants' wares and sales).
  The table is `NPC_STAGES` in `tools/er_flags/gen_npcs.py`; the generator fails when the
  scripts set a flag the table doesn't list or the table lists one no script sets.

## Progress steps and endings

The World Progress tab lists checklist steps under the main questline NPCs and the endings
(`NPC_STEPS` and `ENDINGS` in `tools/er_flags/gen_npcs.py`, with the flag of each step).
Stages replace each other; steps use flags that stay set:

- Item lot flags (ItemLotParam_map / _enemy `getItemFlagId`, +0x80): set once the lot is
  received. The items NPCs give or leave behind are in block 400 (400xxx), e.g. 400391
  Carian Inverted Statue, 400393 Dark Moon Greatsword, 400073 / 400074 Letter from Volcano
  Manor, 400310 Unalloyed Gold Needle; the Mending Runes are 9500 (Perfect Order), 9502
  (Death-Prince), 9504 (Fell Curse); picked-up items are map flags (12027080 Fingerslayer
  Blade in Nokron). Block 400's position was found on the test saves (`docs/event-flags.md`).
- Boss flags of `BOSSES` (ER-Save-Editor), and 9130, set when Starscourge Radahn dies.
- Stages (`+k` above) where a step has no lasting flag: the step is done at that stage or
  any the scripts only reach from it. Default stages (`+1 AND 3000`, set when the NPC's
  map first loads, e.g. Millicent's "Church of the Plague") don't count as met.
- World flags several NPCs' stage events agree on: 1042369411 Ranni met at the Church of
  Elleh (her and Kalé's stages), 1034509410 Ranni met at her Rise (Seluvis, Blaidd and Iji
  move to their Ranni's Rise stages), 1034509416 her service accepted (Ranni "You serve
  her", Blaidd "serving Ranni").

Endings (m19_00_00): Marika's talk script t607001900 offers "Mend the Elden Ring" and,
once their Mending Rune flag is set, "Use Mending Rune of Perfect Order / of the
Death-Prince / of the Fell Curse"; the choices set 9400 (Age of Fracture), 9401 (Age of
Order), 9402 (Age of the Duskborn), 9403 (Blessing of Despair). Event 19000110 (Age of the
Stars) needs 1034509406, Ranni's questline done (it also moves her to her last stage), and
sets 9404 or 9405 (two cutscenes); event 19000120 (Lord of Frenzied Flame) needs 108 (set
at the Three Fingers, m35_00_00) without 116 (the frenzy cured, common event 920) and sets
9406 or 9407. All endings set 120.

Checked on the 5 test characters: the four late-game characters have the Ranni, Volcano
Manor, Varré, Rogier and Millicent steps that their stages imply; one reached Age of the
Stars, one Lord of Frenzied Flame, one Age of Fracture; the early character has only the
first meetings.

## Stages

Triggers are the setting event's conditions (`+k` = that NPC's flag base + k), with its
script; "X +1 AND 3000" is the default a load event falls back to.

### Tanith, Volcano Manor Proprietress (3100-3119)

| flag | stage | trigger |
|---|---|---|
| 3105 (+5) | Volcano Manor | +1 AND 3000 (common) |
| 3106 (+6) | Volcano Manor: you joined | 400073 (common) |
| 3107 (+7) | - | - |
| 3108 (+8) | After Rykard | boss God-Devouring Serpent / Rykard, Lord of Blasphemy AND talk flag 16009208 (common) |
| 3109 (+9) | After Rykard | boss God-Devouring Serpent / Rykard, Lord of Blasphemy AND NOT talk flag 16009208 AND talked: "Well. That is unfortunate." (common) |
| 3110 (+10) | Devouring Rykard | 16009265 (common) |
| 3111 (+11) | Her knight is dead | Tanith's Knight dead (common) |
| 3118 (+18) | - | 400073 (m16_00_00) |

### White Mask Varré (3180-3199)

| flag | stage | trigger |
|---|---|---|
| 3185 (+5) | The First Step | +1 AND 3000 (common) |
| 3187 (+7) | After Roundtable Hold | 105 (common) |
| 3188 (+8) | After meeting the Finger Reader | talked: "Are you that new Tarnished?" (common) |
| 3189 (+9) | Rose Church | 1035449235 AND 1035449207 AND 7612 (common) |
| 3190 (+10) | Rose Church: gave you his task | 400033 (common) |
| 3191 (+11) | - | 181 (common) |
| 3192 (+12) | Rose Church: task done | 1035449226 (common) |
| 3193 (+13) | Mohgwyn Palace | 7601 (common) |
| 3198 (+18) | - | 7700 (common); OT +5 AND NOT +7 (m60_42_36) |

### Smithing Master Hewg (3220-3239)

| flag | stage | trigger |
|---|---|---|
| 3225 (+5) | Roundtable Hold | +1 AND 3000 (common) |
| 3226 (+6) | Roundtable Hold: after the Erdtree burned | 110 (common) |
| 3227 (+7) | - | DLC dummy dead (common) |
| 3228 (+8) | - | talked: "Who are you?" (common) |
| 3238 (+18) | - | - |

### Gatekeeper Gostoc (3260-3279)

| flag | stage | trigger |
|---|---|---|
| 3265 (+5) | Stormveil Castle: at the gate | +1 AND 3000 (common) |
| 3266 (+6) | Stormveil Castle: met | talked: "Oh, hello... Glad to see you safe." AND NOT talked: "Oh, you..." (common) |
| 3268 (+8) | Stormveil Castle: met again | talked: "Oh, hello... Glad to see you safe." AND talked: "Oh, you..." (common) |
| 3269 (+9) | After Godrick | boss Godrick the Grafted (common) |
| 3270 (+10) | Stormveil Castle: Nepheli its lord | Nepheli Loux, Warrior +9 (common) |
| 3278 (+18) | - | 10002732 (m10_00_00); 10002733 (m10_00_00) |

### Witch-Hunter Jerren (3360-3379)

| flag | stage | trigger |
|---|---|---|
| 3365 (+5) | Not met yet | +1 AND 3000 (common) |
| 3366 (+6) | Redmane Castle: festival announced | +5 AND 9410 (common) |
| 3367 (+7) | After Radahn | +6 AND 9130 (common) |
| 3368 (+8) | Left Redmane Castle | +7 AND (talked: "It's time I went on my way." OR 1051369204) (common) |
| 3369 (+9) | Witchbane Ruins | +8 AND 1044369231 (common) |
| 3370 (+10) | After Rennala | +9 AND 1034509256 AND Rennala, Queen of the Full Moon dead (common) |
| 3371 (+11) | Raya Lucaria: fought Sellen | +10 AND 7608 (common) |
| 3372 (+12) | Thanked you | +11 AND talked: "My thanks, for the aid given." AND Sorceress Sellen +3 AND any ON +8 .. +10 (common) |
| 3378 (+18) | - | 1051369204 (common) |

### Lightseeker Hyetta (3380-3399)

| flag | stage | trigger |
|---|---|---|
| 3385 (+5) | Not met yet | - |
| 3386 (+6) | Met | +5 AND talked: "I understand." AND (1043319206 OR Castellan Edgar +3 OR 3062 OR DLC dummy dead) (common) |
| 3387 (+7) | Waiting for a grape | +6 (common) |
| 3388 (+8) | Given a grape | +7 AND talked: "You are most kind, indeed." (common) |
| 3389 (+9) | Given another grape | +8 AND talked: "You are most kind, indeed." (common) |
| 3390 (+10) | Given a Shabriri Grape | +9 AND talked: "N-No... That's not possible..." (common) |
| 3391 (+11) | Moved on | +10 AND talked: "I've gleaned something very important indeed, thanks to you." (common) |
| 3392 (+12) | Thanked you | +11 AND talked: "Thank you from the bottom of my heart." (common) |
| 3393 (+13) | Before the Three Fingers | +12 AND talked: "..." (common) |
| 3398 (+18) | - | any ON 1041389320 .. 1041389322 AND NOT 1041382729 AND NOT 1038412709 (m60_38_43); Castellan Edgar +5 AND (1043319206 OR Castellan Edgar +3) (m60_43_31) |

### Castellan Edgar (3400-3419)

| flag | stage | trigger |
|---|---|---|
| 3405 (+5) | Castle Morne | +2 AND NOT 1045349256 (common) |
| 3406 (+6) | Castle Morne: met | +5 AND 1043319206 (common) |
| 3407 (+7) | After the Leonine Misbegotten | +6 AND boss Leonine Misbegotten (Morne Moangrave) (common); +8 AND NOT 1045349256 (common) |
| 3408 (+8) | Grieving Irina | +7 AND Lightseeker Hyetta +5 AND Lightseeker Hyetta +3 AND 1043319207 (common); +7 AND Lightseeker Hyetta +5 AND Lightseeker Hyetta +3 AND 1045342719 (common) |
| 3409 (+9) | Left | +8 AND 1045349255 (common); any ON +5 .. +8 AND talked: "You are most kind, indeed." (common) |
| 3418 (+18) | - | talked: "You are most kind, indeed." (m60_39_40); 1043319206 AND +5 (m60_43_31) |

### Zorayas the Scout (3420-3439)

| flag | stage | trigger |
|---|---|---|
| 3425 (+5) | Volcano Manor | +1 AND 3000 (common) |
| 3426 (+6) | Volcano Manor: met | talked: "Yes, I see. Most unfortunate, but I'll have to make do." AND NOT talked: "..." AND NOT talk flag 16009208 (common) |
| 3427 (+7) | Volcano Manor: talked | talked: "..." AND talk flag 16009208 (common) |
| 3428 (+8) | Volcano Manor: told you of the ruins | talked: "Seek the old ruins in the cliff." (common) |
| 3429 (+9) | Volcano Manor: told you of the ruins | talked: "Hmm, what is this?" (common) |
| 3430 (+10) | After Rykard | talked: "Hmm, what is this?" AND (boss God-Devouring Serpent / Rykard, Lord of Blasphemy AND NOT talk flag 16009208 OR talked: "Well. That is unfortunate.") AND NOT Patches +9 AND NOT Potentate Diallos +8 AND NOT Potentate Diallos +9 AND NOT Knight Bernahl +6 (common) |
| 3431 (+11) | After Rykard: gave up | talked: "I suppose I knew in my heart of hearts." AND NOT talked: "Hmm, what is this?" AND any ON +5 .. +8 AND NOT talked: "Hmm, what is this?" AND boss God-Devouring Serpent / Rykard, Lord of Blasphemy AND talked: "Oh, it's you." (common) |
| 3438 (+18) | - | OT Tanith, Volcano Manor Proprietress +5 AND NOT Tanith, Volcano Manor Proprietress +6 AND NOT Tanith, Volcano Manor Proprietress +7 AND NOT Tanith, Volcano Manor Proprietress +8 AND NOT Patches +9 AND NOT Potentate Diallos +8 AND NOT Potentate Diallos +9 AND NOT Knight Bernahl +6 (m16_00_00); talk  |

### Potentate Diallos (3440-3459)

| flag | stage | trigger |
|---|---|---|
| 3445 (+5) | Not met yet | +1 AND 3000 (common) |
| 3446 (+6) | - | 1037442701 AND 1037449206 AND 110 (common) |
| 3447 (+7) | - | talked: "They laid hands upon my servant Lanya, and I refuse to let the insult stand." (common) |
| 3448 (+8) | - | talked: "Ah, you again, is it?" (common) |
| 3449 (+9) | Volcano Manor: met again | talked: "Ah. Oh... I-it's you, is it?" AND NOT talked: "It's on Mt. Gelmir, found off the old road that leads west from the town of wind" AND DLC dummy dead (common) |
| 3450 (+10) | Jarburg | +16 AND 1039449263 (common) |
| 3451 (+11) | Jarburg | Jar-Bairn +7 (common) |
| 3456 (+16) | - | talked: "It's on Mt. Gelmir, found off the old road that leads west from the town of wind" AND DLC dummy dead AND talked: "It's just as my noble brother says." (common) |
| 3457 (+17) | - | any ON +5 .. +7 AND DLC dummy dead (common) |
| 3458 (+18) | - | - |

### Sorceress Sellen (3460-3479)

| flag | stage | trigger |
|---|---|---|
| 3465 (+5) | Waypoint Ruins | any ON +0 .. +3 (common) |
| 3466 (+6) | Jerren at Witchbane Ruins | +5 AND Witch-Hunter Jerren +9 (common) |
| 3467 (+7) | After Rennala | +6 AND 1034509256 AND Rennala, Queen of the Full Moon dead (common) |
| 3468 (+8) | After Rennala | (Witch-Hunter Jerren +3 OR 7609) AND Rennala, Queen of the Full Moon dead AND 1034509256 (common) |
| 3469 (+9) | Raya Lucaria: lessons again | +8 AND talked: "My apprentice, shall we commence the lesson?" AND (1044369228 OR talked: "Who could've guessed. What a place to find Master Lusat...") (common) |
| 3478 (+18) | - | - |

### Finger Reader Enia (3480-3499)

| flag | stage | trigger |
|---|---|---|
| 3485 (+5) | Roundtable Hold | +1 AND 3000 (common) |
| 3486 (+6) | - | 182 (common) |
| 3487 (+7) | - | 11000500 (common) |
| 3488 (+8) | Roundtable Hold: after the Erdtree burned | 110 (common) |
| 3489 (+9) | - | 118 (common) |
| 3498 (+18) | - | - |

### Old Albus (3540-3559)

| flag | stage | trigger |
|---|---|---|
| 3545 (+5) | Village of the Albinaurics | +1 AND 3000 (common) |
| 3546 (+6) | Gave you the medallion half | 1035429210 (common) |
| 3558 (+18) | - | - |

### Preceptor Seluvis (3560-3579)

| flag | stage | trigger |
|---|---|---|
| 3565 (+5) | Not met yet | - |
| 3566 (+6) | Ranni's Rise: met | +5 AND 1034509410 (common) |
| 3567 (+7) | - | +6 AND 1034509416 (common) |
| 3568 (+8) | - | +7 AND 1034509315 (common) |
| 3569 (+9) | Gone | any ON +5 .. +8 AND 1034509424 (common); any ON +5 .. +8 AND 1034509433 (common) |
| 3578 (+18) | - | Ranni the Witch +8 AND 1034509421 (common); 1034509410 (m60_34_50) |

### Kenneth Haight, Limgrave Heir (3580-3599)

| flag | stage | trigger |
|---|---|---|
| 3585 (+5) | Limgrave | +1 AND 3000 (common) |
| 3586 (+6) | Asked you to take Fort Haight | +5 AND talked: "What are you waiting for? A kiss goodbye?" (common) |
| 3587 (+7) | Stormveil Castle: Nepheli its lord | Nepheli Loux, Warrior +9 AND talked: "I am sorry to have lifted your hopes..." (common) |
| 3598 (+18) | - | - |

### Blaidd the Half-Wolf (3600-3619)

| flag | stage | trigger |
|---|---|---|
| 3605 (+5) | Mistwood: met | +11 AND NOT talked: "Traitorous wretch..." AND NOT talked: "Hmm..." (common); +6 AND NOT boss Bloodhound Knight Darriwil (Forlorn Hound Evergaol) (common) |
| 3606 (+6) | Told you of Darriwil | +5 AND talked: "Darriwil is nothing but a traitor." AND 1044352717 (common) |
| 3607 (+7) | Darriwil defeated | any ON +5 .. +6 AND talked: "Darriwil is nothing but a traitor." AND boss Bloodhound Knight Darriwil (Forlorn Hound Evergaol) (common) |
| 3608 (+8) | Met again | +7 AND talked: "...Right. There you are." (common) |
| 3609 (+9) | Ranni's Rise | any ON +5 .. +8 AND 1034509410 (common) |
| 3610 (+10) | Ranni's Rise: serving Ranni | +9 AND 1034509416 (common) |
| 3611 (+11) | Redmane Castle: festival announced | +12 AND NOT 9411 (common); any ON +5 .. +10 AND 9410 AND (talked: "Hmm..." OR 1051362739) (common) |
| 3612 (+12) | Redmane Castle | +11 AND 9411 (common) |
| 3613 (+13) | After Radahn | +12 AND 9412 (common) |
| 3614 (+14) | Ranni's Rise: waiting | +13 AND (talked: "Ah, good. I was waiting for you." OR 1034499238 OR 12027080) (common); any ON +5 .. +10 AND 9412 AND NOT 1051369357 (common) |
| 3615 (+15) | Ranni's Rise | +14 AND boss Bloodhound Knight Darriwil (Forlorn Hound Evergaol) AND 1034509410 (common) |
| 3616 (+16) | Going to see Ranni | +15 AND talked: "I'm going to see mistress Ranni, now." (common) |
| 3617 (+17) | Going to see Ranni | +15 AND talked: "I'm going to see mistress Ranni, now." (common) |
| 3618 (+18) | - | (talked: "Well, well..." OR 1034499224 OR 3063) AND talked: "Well, I haven't heard that name in an age." (common); 1034509410 (m60_34_50) |

### Yura, Hunter of Bloody Fingers (3620-3639)

| flag | stage | trigger |
|---|---|---|
| 3625 (+5) | Murkwater: met | Bloody Finger Nerijus dead AND NOT talked: "..." (common) |
| 3626 (+6) | Met again | Bloody Finger Nerijus dead AND talked: "..." AND NOT Bloody Finger Nerijus dead (common); +1 AND 3000 (common) |
| 3627 (+7) | - | 7607 (common) |
| 3630 (+10) | - | talked: "Be on your way." (common) |
| 3631 (+11) | - | 1039529206 AND 1049539210 (common); +3 AND NOT +11 (common) |
| 3638 (+18) | - | OT Blaidd the Half-Wolf +17 AND Blaidd the Half-Wolf +3 (m60_37_46); Bloody Finger Nerijus dead (m60_43_37) |

### Gurranq, Beast Clergyman (3640-3659)

| flag | stage | trigger |
|---|---|---|
| 3645 (+5) | Bestial Sanctum | - |
| 3646 (+6) | Given Deathroot | +5 (common) |
| 3647 (+7) | Turned feral | +6 AND +0 (common) |
| 3648 (+8) | Given more Deathroot | +7 (common) |
| 3658 (+18) | - | OT Blaidd the Half-Wolf +17 AND Blaidd the Half-Wolf +3 (m60_37_46) |

### Alexander, Warrior Jar (3660-3679)

| flag | stage | trigger |
|---|---|---|
| 3665 (+5) | Stuck in the ground | +1 AND 3000 (common) |
| 3666 (+6) | Freed, heading east | +5 AND (talked: "I see. More's the pity." OR talked: "I'm heading eastward.") (common) |
| 3667 (+7) | Redmane Castle: festival announced | 9410 AND (+5 OR +6) (common) |
| 3668 (+8) | After Radahn | +7 AND 9130 (common) |
| 3669 (+9) | Mt. Gelmir: met again | +8 AND talked: "Ah, you came, how delightful!" (common) |
| 3670 (+10) | Mt. Gelmir: freed again | +9 AND talked: "Ah, you came, how delightful!" AND 1035539204 (common) |
| 3671 (+11) | After the Fire Giant | +10 AND boss Fire Giant AND talked: "Alright lads, I'm going to take you on a little journey." (common) |
| 3678 (+18) | - | OT Blaidd the Half-Wolf +17 AND Blaidd the Half-Wolf +3 (m60_37_46) |

### Patches (3680-3699)

| flag | stage | trigger |
|---|---|---|
| 3685 (+5) | - | +11 AND +12 AND +14 (common) |
| 3686 (+6) | - | talked: "Ahhh..." (common) |
| 3687 (+7) | Murkwater Cave: spared | talked: "Ahhh..." AND 1038419254 AND NOT talk flag 16009208 (common) |
| 3688 (+8) | - | talked: "Ahhh..." AND 1037549211 AND NOT talk flag 16009208 AND NOT 1037549210 (common) |
| 3689 (+9) | Volcano Manor | talked: "Ahhh..." AND talk flag 16009208 (common) |
| 3690 (+10) | Volcano Manor | +17 (common); talked: "What is it?" (common) |
| 3691 (+11) | Met again | (talked: "Ah, oh, you again..." OR +16) AND NOT 31002715 AND NOT 9000 (common) |
| 3692 (+12) | - | talked: "Oh, it's nothing, it's just... Makes me sick to see her all bent out of shape." (common) |
| 3693 (+13) | - | talked: "Ahhh..." AND 1037549211 AND NOT talk flag 16009208 AND 1037549210 (common) |
| 3694 (+14) | - | dead AND NOT talked: "Oh, it's nothing, it's just... Makes me sick to see her all bent out of shape." (common) |
| 3696 (+16) | Met again | talked: "Ah, oh, you again..." (common) |
| 3698 (+18) | - | OT +5 AND NOT +6 AND NOT +7 (m60_37_42); OT Blaidd the Half-Wolf +17 AND Blaidd the Half-Wolf +3 (m60_37_46) |

### Roderika, Spirit Tuner (3700-3719)

| flag | stage | trigger |
|---|---|---|
| 3705 (+5) | Stormhill Shack | +1 AND 3000 (common) |
| 3707 (+7) | Roundtable Hold: spirit tuner | +5 AND talked: "They all...believed in me." AND 3062 (common) |
| 3708 (+8) | Roundtable Hold: Hewg spoke of her | +7 AND talked: "The girl? What about her?" (common) |
| 3709 (+9) | Roundtable Hold: after the Erdtree burned | 110 (common) |
| 3718 (+18) | - | - |

### Miriel, Pastor of Vows (3720-3739)

| flag | stage | trigger |
|---|---|---|
| 3725 (+5) | Church of Vows | - |
| 3738 (+18) | - | - |

### Ranni the Witch (3740-3759)

| flag | stage | trigger |
|---|---|---|
| 3745 (+5) | Ranni's Rise | +6 AND NOT 1042369410 AND NOT 1042369407 (common); +8 AND 1034509425 (common) |
| 3746 (+6) | Church of Elleh: met | +5 AND 1042369411 (common) |
| 3747 (+7) | Offered you her service | +6 AND (1042369410 OR 1042369407) (common); any ON +5 .. +6 AND (1034509419 OR 3062) (common) |
| 3748 (+8) | You serve her | +7 AND 1034509416 (common) |
| 3749 (+9) | - | +8 AND 1034509421 (common) |
| 3750 (+10) | - | any ON +5 .. +9 AND 1035420150 (common) |
| 3751 (+11) | - | +10 AND 1034509406 (common) |
| 3758 (+18) | - | 1035420150 (m60_35_42); (1042362732 OR 1042369401) AND NOT 1042362733 (m60_42_36) |

### Smithing Master Iji (3760-3779)

| flag | stage | trigger |
|---|---|---|
| 3765 (+5) | Near Ranni's Rise | - |
| 3766 (+6) | - | +5 AND 1034509410 (common) |
| 3767 (+7) | - | +6 AND 1034509416 (common) |
| 3768 (+8) | - | any ON +5 .. +7 AND 1034499242 (common) |
| 3778 (+18) | - | 1034509410 (m60_34_50); 1034509416 (m60_34_50) |

### Sorcerer Thops (3800-3819)

| flag | stage | trigger |
|---|---|---|
| 3805 (+5) | Church of Irith | +1 AND 3000 (common) |
| 3806 (+6) | Given the Academy key | +5 AND 1039399220 (common) |
| 3818 (+18) | - | - |

### Jar-Bairn (3820-3839)

| flag | stage | trigger |
|---|---|---|
| 3825 (+5) | Jarburg | +1 AND 3000 (common) |
| 3826 (+6) | Jarburg: met | +5 AND 1039449258 AND (talked: "Ah, if it isn't a Tarnished warrior." OR Alexander, Warrior Jar +3) (common) |
| 3827 (+7) | Jarburg | +6 AND 1039449278 (common) |
| 3828 (+8) | Jarburg: Diallos is dead | +7 AND Potentate Diallos +3 AND 1039449270 (common) |
| 3829 (+9) | - | 1039449291 (common) |
| 3838 (+18) | - | - |

### Knight Bernahl (3880-3899)

| flag | stage | trigger |
|---|---|---|
| 3885 (+5) | Volcano Manor | +1 AND 3000 (common) |
| 3886 (+6) | Volcano Manor: you joined | talk flag 16009208 (common) |
| 3887 (+7) | After Rykard | talked: "So. You killed Rykard?" (common) |
| 3888 (+8) | After Rykard | OT talk flag 16009208 AND boss God-Devouring Serpent / Rykard, Lord of Blasphemy (common) |
| 3898 (+18) | - | talk flag 16009208 (m16_00_00) |

### Sorcerer Rogier (3900-3919)

| flag | stage | trigger |
|---|---|---|
| 3905 (+5) | Stormveil Castle | - |
| 3906 (+6) | Roundtable Hold | +5 AND (DLC dummy dead OR 10009610) (common) |
| 3907 (+7) | Roundtable Hold: researching | +6 AND DLC dummy dead (common) |
| 3908 (+8) | Told you of the Black Knives | any ON +5 .. +7 AND (talked: "Hmm. Maybe I should tell you." OR 1034509421) (common); any ON +5 .. +7 AND 110 AND (NOT talked: "Ahh, hello. I was hoping to see you." OR 1034509410 OR 1034509431) (common) |
| 3909 (+9) | - | +8 (common) |
| 3918 (+18) | - | OT 10002164 AND (talked: "<sleeping>" OR any ON +7 .. +17) (m10_00_00); DLC dummy dead (m10_00_00) |

### Boc the Seamster (3940-3959)

| flag | stage | trigger |
|---|---|---|
| 3945 (+5) | Limgrave | +1 AND 3000 (common) |
| 3946 (+6) | Met | +5 AND talked: "Oh, what a shame." (common) |
| 3947 (+7) | Following you | +6 AND talk flag 31159206 (common) |
| 3948 (+8) | After Rennala | +7 AND NOT talked: "Did I just hear my mum speaking?" AND talked: "M'lord, what's this?" AND Rennala, Queen of the Full Moon dead (common) |
| 3949 (+9) | After Rennala | +7 AND NOT talked: "Did I just hear my mum speaking?" AND talked: "M'lord." AND Rennala, Queen of the Full Moon dead (common) |
| 3954 (+14) | - | +0 AND talked: "Oh..." (m60_39_40) |
| 3955 (+15) | - | +0 AND talked: "Oh..." (m60_36_48) |
| 3956 (+16) | - | +0 AND talked: "Oh..." (m60_39_51) |
| 3957 (+17) | - | +7 (m11_00_00) |
| 3958 (+18) | - | - |

### Sir Gideon Ofnir, the All-Knowing (3960-3979)

| flag | stage | trigger |
|---|---|---|
| 3965 (+5) | Roundtable Hold | +1 AND 3000 (common) |
| 3966 (+6) | Roundtable Hold: talked | +5 AND talked: "Oh, this is a rare occasion." AND 181 AND talked: "Oh, this is a rare occasion." (common) |
| 3967 (+7) | Roundtable Hold: talked again | 110 AND +6 AND talked: "Are you that new Tarnished?" AND talked: "What do you need? I have little time to spare." AND 181 AND talked: "Oh, this is a rare occasion." (common) |
| 3968 (+8) | - | DLC dummy dead (common) |
| 3978 (+18) | - | - |

### D, Hunter of the Dead (4040-4059)

| flag | stage | trigger |
|---|---|---|
| 4045 (+5) | Limgrave: met | - |
| 4046 (+6) | After the Tibia Mariner | +5 AND boss Tibia Mariner (Summonwater Village) (common) |
| 4047 (+7) | Roundtable Hold | +6 AND (talked: "Very well. Show me your map." OR 11109609) (common); any ON +5 .. +6 AND (132 OR 3064 OR 3063 OR 1051439205) (common) |
| 4048 (+8) | Roundtable Hold: watching Fia | OT +3 (common); Fia, Deathbed Companion +5 AND +7 AND talked: "Well, what have we here?" (common) |
| 4058 (+18) | - | Fia, Deathbed Companion +5 AND +7 AND talked: "Well, what have we here?" (common); any ON Fia, Deathbed Companion +5 .. Fia, Deathbed Companion +6 AND 110 (common) |

### D, Beholder of Death (4060-4079)

| flag | stage | trigger |
|---|---|---|
| 4065 (+5) | Deeproot Depths | - |
| 4066 (+6) | After Fia's quest | +17 AND 9502 AND Fia, Deathbed Companion +11 (common) |
| 4067 (+7) | No regrets | +17 AND Fia, Deathbed Companion +3 AND any ON Fia, Deathbed Companion +8 .. Fia, Deathbed Companion +9 (common); +6 AND talked: "Darian, now I have no regrets." (common) |
| 4077 (+17) | - | +5 AND talked: "Ah, aah..." AND D, Hunter of the Dead +8 (common) |
| 4078 (+18) | - | +5 AND talked: "Ah, aah..." AND D, Hunter of the Dead +8 (m12_02_00) |

### Latenna the Albinauric (4100-4119)

| flag | stage | trigger |
|---|---|---|
| 4105 (+5) | Lakeside Crystal Cave | +1 AND 3000 (common) |
| 4106 (+6) | - | 1036419209 (common) |
| 4118 (+18) | - | - |

### Fia, Deathbed Companion (4120-4139)

| flag | stage | trigger |
|---|---|---|
| 4125 (+5) | Roundtable Hold | - |
| 4126 (+6) | Roundtable Hold: D watching her | +5 AND D, Hunter of the Dead +7 AND talked: "Well, what have we here?" (common) |
| 4127 (+7) | Deeproot Depths | +5 AND D, Hunter of the Dead +3 AND (3063 OR 9410) (common); +6 AND talked: "Greetings, great champion called by grace." (common) |
| 4128 (+8) | After Fia's Champions | +7 AND boss Fia's Champions (common) |
| 4129 (+9) | Lying with Godwyn | +8 AND talked: "I will soon lay with Godwyn." (common) |
| 4130 (+10) | Waiting for Fortissax | +17 AND NOT boss Lichdragon Fortissax (common); +9 AND talked: "Have you ever seen a hallowbrand?" (common) |
| 4131 (+11) | After Lichdragon Fortissax | +10 AND boss Lichdragon Fortissax (common); +17 AND boss Lichdragon Fortissax (common) |
| 4132 (+12) | - | D, Beholder of Death +17 AND 9502 AND +11 (common) |
| 4137 (+17) | - | +10 AND 12032870 (common) |
| 4138 (+18) | - | OT 12039179 AND 12039179 AND NOT 12039179 (m12_03_00); 12032870 AND +10 (m12_03_00) |

### Blackguard (4140-4159)

| flag | stage | trigger |
|---|---|---|
| 4145 (+5) | Liurnia | +1 AND 3000 (common) |
| 4146 (+6) | - | (NOT 35009316 OR 1045520180) AND 1036439213 AND 3063 AND NOT 1044522701 (common) |
| 4147 (+7) | - | 1044529259 (common) |
| 4158 (+18) | - | - |

### Sage Gowry (4160-4179)

| flag | stage | trigger |
|---|---|---|
| 4165 (+5) | Gowry's Shack | +1 AND 3000 (common) |
| 4166 (+6) | - | +5 AND 1050389207 (common) |
| 4167 (+7) | Gowry's Shack: Millicent cured | +6 AND talked: "Please, let me pass alone." (common) |
| 4168 (+8) | Millicent at the Haligtree | +7 AND Millicent +13 (common) |
| 4169 (+9) | Millicent's choice made | any ON Millicent +11 .. Millicent +12 (common) |
| 4170 (+10) | Millicent's choice made | any ON Millicent +11 .. Millicent +12 AND Millicent +3 (common) |
| 4171 (+11) | - | 7611 (common) |
| 4178 (+18) | - | - |

### Millicent (4180-4199)

| flag | stage | trigger |
|---|---|---|
| 4185 (+5) | Church of the Plague | +1 AND 3000 (common) |
| 4186 (+6) | Cured | +5 AND talked: "Please, let me pass alone." (common) |
| 4187 (+7) | Gowry's Shack | +6 AND talked: "If you are wise, you will leave, immediately." (common) |
| 4188 (+8) | After the Godskin Apostle | +7 AND boss Godskin Apostle (Dominula, Windmill Village) AND talked: "If the arm serves well enough," (common) |
| 4189 (+9) | Met again | +8 AND talked: "Ah, we meet yet again." (common) |
| 4190 (+10) | Met again | +9 AND talked: "To think we'd meet in such a place..." (common) |
| 4191 (+11) | - | +13 AND 7610 (common) |
| 4192 (+12) | Met again | +11 AND talked: "Oh, hello." (common) |
| 4193 (+13) | Haligtree | +10 AND talked: "Again we meet." AND 15000398 AND (1050389227 OR 15009213) (common) |
| 4194 (+14) | - | +13 AND 7611 (common) |
| 4197 (+17) | - | talked: "I'm considering leaving. On a journey." (common) |
| 4198 (+18) | - | - |

### Scribe Corhyn (4200-4219)

| flag | stage | trigger |
|---|---|---|
| 4205 (+5) | Roundtable Hold | +1 AND 3000 (common) |
| 4206 (+6) | Roundtable Hold: talked | +5 AND talked: "Ah, you appear to be doing well. Very good." (common) |
| 4207 (+7) | Looking for Goldmask | +6 AND talked: "I'm yet to find the noble Goldmask." (common) |
| 4208 (+8) | Leaving to join Goldmask | +7 AND talked: "I will soon leave to join the noble Goldmask." (common) |
| 4209 (+9) | With Goldmask | +8 AND talk flag 11009555 (common) |
| 4210 (+10) | Goldmask's path done | +9 AND DLC dummy dead AND NOT talked: "I've…been gripped by a terrifying thought." (common) |
| 4211 (+11) | Goldmask's path done, terrified | +9 AND DLC dummy dead AND talked: "I've…been gripped by a terrifying thought." (common) |
| 4217 (+17) | - | OT talk flag 11009555 AND 110 (common) |
| 4218 (+18) | - | - |

### Nepheli Loux, Warrior (4220-4239)

| flag | stage | trigger |
|---|---|---|
| 4225 (+5) | Stormveil Castle | +1 AND 3000 (common) |
| 4226 (+6) | Roundtable Hold | +5 AND (DLC dummy dead OR 1034509305) (common) |
| 4227 (+7) | Met again | +6 AND talked: "You again..." (common) |
| 4228 (+8) | After the Omenkiller | +7 AND boss Omenkiller (Village of the Albinaurics) AND talked: "It's about time I headed off." (common) |
| 4229 (+9) | Lord of Stormveil Castle | +8 AND DLC dummy dead AND 10009722 (common) |
| 4230 (+10) | - | +8 AND talked: "...So you know already, do you?" (common) |
| 4238 (+18) | - | - |

### Dung Eater (4240-4259)

| flag | stage | trigger |
|---|---|---|
| 4245 (+5) | Roundtable Hold | +1 AND 3000 (common) |
| 4246 (+6) | Roundtable Hold: talked | +5 AND talked: "Have you ever felt the curse, with your whole being?" (common) |
| 4247 (+7) | - | +6 AND talked: " " (common) |
| 4248 (+8) | - | +7 AND 1045520180 (common) |
| 4249 (+9) | - | +8 AND talked: "There you are." (common) |
| 4251 (+11) | - | +9 AND talk flag 35009326 (common) |
| 4258 (+18) | - | - |

### Igon (4260-4279)

| flag | stage | trigger |
|---|---|---|
| 4265 (+5) | Not met yet | +1 AND 3000 (common) |
| 4267 (+7) | - | +5 AND 2048429205 (common) |
| 4268 (+8) | - | +7 AND 2052409205 (common) |
| 4278 (+18) | - | DLC dummy dead (m61_52_40) |

### St. Trina (4280-4299)

| flag | stage | trigger |
|---|---|---|
| 4285 (+5) | Not met yet | - |
| 4286 (+6) | - | 22009275 (common) |
| 4298 (+18) | - | DLC dummy dead (m22_00_00) |

### Fire Knight Queelign (4340-4359)

| flag | stage | trigger |
|---|---|---|
| 4345 (+5) | Not met yet | - |
| 4346 (+6) | - | +5 AND 4865 (common) |
| 4358 (+18) | - | (4860) (common); 4860 (m21_00_00) |

### Hornsent (4360-4379)

| flag | stage | trigger |
|---|---|---|
| 4365 (+5) | Met | +2 AND +3 (common) |
| 4366 (+6) | - | +5 AND 2046429214 (common) |
| 4368 (+8) | - | +6 AND 2048459225 (common) |
| 4369 (+9) | - | +8 AND 21019220 (common) |
| 4370 (+10) | - | +8 AND 21019222 (common) |
| 4378 (+18) | - | (4925 OR 4927) (common); talked: "We meet again I see, comrade-in-arms." AND NOT 21019222 (common) |

### Sir Ansbach (4400-4419)

| flag | stage | trigger |
|---|---|---|
| 4405 (+5) | Not met yet | +1 AND 3000 (common) |
| 4406 (+6) | Met | +5 AND 2045429225 AND NOT 4830 (common) |
| 4407 (+7) | - | +6 AND 20019223 (common) |
| 4408 (+8) | - | +6 AND 20019228 (common); +7 AND 20019225 AND DLC dummy dead (common) |
| 4418 (+18) | - | 20019224 AND NOT 20019223 (common); talk flag 4902 AND NOT 20019257 (m20_01_00) |

### Redmane Freyja (4420-4439)

| flag | stage | trigger |
|---|---|---|
| 4425 (+5) | Not met yet | +1 AND 3000 (common) |
| 4426 (+6) | Talked of Leda | +5 AND talked: "Ah, Lady Leda spoke of you." (common) |
| 4427 (+7) | - | +6 AND 21019375 (common) |
| 4438 (+18) | - | talked: "There is no need for concern." AND NOT 21019375 (common); talk flag 4902 AND NOT 20019257 (m20_01_00) |

### Needle Knight Leda (4440-4459)

| flag | stage | trigger |
|---|---|---|
| 4445 (+5) | Mohgwyn Palace: met | - |
| 4446 (+6) | Realm of Shadow | +5 AND 12059260 (common) |
| 4447 (+7) | Said farewell | +6 AND talked: "I'm afraid this is our farewell, comrade." (common) |
| 4448 (+8) | - | +7 AND 20019256 (common) |
| 4458 (+18) | - | OT 4896 (common); 20019256 (common) |

### Thiollier (4460-4479)

| flag | stage | trigger |
|---|---|---|
| 4465 (+5) | Not met yet | - |
| 4466 (+6) | Met | +5 AND talked: "Umm, oh..." (common) |
| 4467 (+7) | - | +6 AND 22009205 (common) |
| 4468 (+8) | - | +7 AND 20019305 (common) |
| 4469 (+9) | - | 22009241 (common); +8 AND 20019310 (common) |
| 4478 (+18) | - | 4894 AND NOT 20019305 (common); talk flag 4902 AND NOT 20019257 (m20_01_00) |

### Hornsent Grandam (4480-4499)

| flag | stage | trigger |
|---|---|---|
| 4485 (+5) | - | +2 AND +3 (common) |
| 4498 (+18) | - | - |

### Count Ymir, Mother of Fingers (4500-4519)

| flag | stage | trigger |
|---|---|---|
| 4505 (+5) | - | +3 AND DLC dummy dead AND NOT dead (common) |
| 4506 (+6) | - | +3 AND DLC dummy dead AND NOT dead (common); +5 AND 2051459211 (common) |
| 4518 (+18) | - | - |

### Merchant Kalé (4700-4719)

| flag | stage | trigger |
|---|---|---|
| 4705 (+5) | Church of Elleh | +17 (common); +1 AND 3000 (common) |
| 4717 (+17) | - | +5 AND 1042369414 (common) |
| 4718 (+18) | - | Ranni the Witch +5 AND 1042369411 (common); (talked: "Good-bye. Nice to do business." OR talked: "Wait, what's that?") (m35_00_00) |

### Nomadic Merchant (1) (4720-4739)

| flag | stage | trigger |
|---|---|---|
| 4725 (+5) | - | +6 (m60_37_46) |
| 4726 (+6) | - | OT +6 AND +5 AND ((+6) OR HP of DLC dummy AND (1045369307 OR  OR (+6 OR +7) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_45_36); OT +6 AND +5 AND ((+6) OR HP of Nomadic Merchant AND (1045369306 OR  OR (+6 |
| 4727 (+7) | - | OT +6 AND +5 AND ((+6) OR HP of DLC dummy AND (1045369307 OR  OR (+6 OR +7) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_45_36); OT +6 AND +5 AND ((+6) OR HP of Nomadic Merchant AND (1045369306 OR  OR (+6 |
| 4728 (+8) | - | OT +8 AND Nomadic Merchant dead (m60_45_36) |
| 4730 (+10) | - | +11 (m60_37_46) |
| 4731 (+11) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1041369207 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_41_36); OT +11 AND +10 AND ((+11) OR HP of Nomadic Merchant AND (1041369206 OR |
| 4732 (+12) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1041369207 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_41_36); OT +11 AND +10 AND ((+11) OR HP of Nomadic Merchant AND (1041369206 OR |
| 4733 (+13) | - | OT +13 AND Nomadic Merchant dead (m60_41_36) |
| 4735 (+15) | - | +16 (m60_37_46) |
| 4736 (+16) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1044339207 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_44_33); OT +16 AND +15 AND ((+16) OR HP of Nomadic Merchant AND (1044339206 OR |
| 4737 (+17) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1044339207 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_44_33); OT +16 AND +15 AND ((+16) OR HP of Nomadic Merchant AND (1044339206 OR |
| 4738 (+18) | - | OT +18 AND Nomadic Merchant dead (m60_44_33) |

### Nomadic Merchant (2) (4740-4759)

| flag | stage | trigger |
|---|---|---|
| 4745 (+5) | - | +1 AND 3000 (common); +6 (m60_37_46) |
| 4746 (+6) | - | 1041369201 AND NOT 1041369202 (common); OT +6 AND +5 AND ((+6) OR HP of DLC dummy AND (1041329207 OR  OR (+6 OR +7) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_41_32) |
| 4747 (+7) | - | 1041369201 AND 1041369202 (common); OT +6 AND +5 AND ((+6) OR HP of DLC dummy AND (1041329207 OR  OR (+6 OR +7) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_41_32) |
| 4748 (+8) | - | OT +8 AND Isolated Merchant dead (m60_41_32) |
| 4750 (+10) | - | +11 (m60_37_46) |
| 4751 (+11) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1035459207 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_35_45); OT +11 AND +10 AND ((+11) OR HP of Isolated Merchant AND (1035459206 O |
| 4752 (+12) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1035459207 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_35_45); OT +11 AND +10 AND ((+11) OR HP of Isolated Merchant AND (1035459206 O |
| 4753 (+13) | - | OT +13 AND Isolated Merchant dead (m60_35_45) |
| 4755 (+15) | - | +16 (m60_37_46) |
| 4756 (+16) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1036499257 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_36_49); OT +16 AND +15 AND ((+16) OR HP of Nomadic Merchant AND (1036499256 OR |
| 4757 (+17) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1036499257 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_36_49); OT +16 AND +15 AND ((+16) OR HP of Nomadic Merchant AND (1036499256 OR |
| 4758 (+18) | - | OT +18 AND Nomadic Merchant dead (m60_36_49) |

### Hermit Merchant (4760-4779)

| flag | stage | trigger |
|---|---|---|
| 4765 (+5) | - | +6 (m60_37_46) |
| 4766 (+6) | - | OT +6 AND +5 AND ((+6) OR HP of Nomadic Merchant AND (talked: "Ah, oh, it's you. Thanks for your help there." OR  OR (+6 OR +7) OR HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND ( OR  O |
| 4767 (+7) | - | OT +6 AND +5 AND ((+6) OR HP of Nomadic Merchant AND (talked: "Ah, oh, it's you. Thanks for your help there." OR  OR (+6 OR +7) OR HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND HP of Nomadic Merchant AND ( OR  O |
| 4768 (+8) | - | OT +8 AND Nomadic Merchant dead (m60_40_52) |
| 4770 (+10) | - | +11 (m60_37_46) |
| 4771 (+11) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1037549257 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_37_54); OT +11 AND +10 AND ((+11) OR HP of Nomadic Merchant AND (1037549256 OR |
| 4772 (+12) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1037549257 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_37_54); OT +11 AND +10 AND ((+11) OR HP of Nomadic Merchant AND (1037549256 OR |
| 4773 (+13) | - | OT +13 AND Nomadic Merchant dead (m60_37_54) |
| 4775 (+15) | - | +16 (m60_37_46) |
| 4776 (+16) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1048389207 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_48_38); OT +16 AND +15 AND ((+16) OR HP of Nomadic Merchant AND (1048389206 OR |
| 4777 (+17) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1048389207 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_48_38); OT +16 AND +15 AND ((+16) OR HP of Nomadic Merchant AND (1048389206 OR |
| 4778 (+18) | - | OT +18 AND Nomadic Merchant dead (m60_48_38) |

### Nomadic Merchant (3) (4780-4799)

| flag | stage | trigger |
|---|---|---|
| 4785 (+5) | - | +6 (m60_37_46) |
| 4790 (+10) | - | +11 (m60_37_46) |
| 4791 (+11) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1048410702 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_48_41); OT +11 AND +10 AND ((+11) OR HP of Isolated Merchant AND (1048410702 O |
| 4792 (+12) | - | OT +11 AND +10 AND ((+11) OR HP of DLC dummy AND (1048410702 OR  OR (+11 OR +12) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_48_41); OT +11 AND +10 AND ((+11) OR HP of Isolated Merchant AND (1048410702 O |
| 4793 (+13) | - | OT +13 AND Isolated Merchant dead (m60_48_41) |
| 4795 (+15) | - | +16 (m60_37_46) |
| 4796 (+16) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1051569407 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_51_56); OT +16 AND +15 AND ((+16) OR HP of Hermit Merchant AND (1051569406 OR  |
| 4797 (+17) | - | OT +16 AND +15 AND ((+16) OR HP of DLC dummy AND (1051569407 OR  OR (+16 OR +17) OR HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND HP of DLC dummy AND ( OR  OR ))) (m60_51_56); OT +16 AND +15 AND ((+16) OR HP of Hermit Merchant AND (1051569406 OR  |
| 4798 (+18) | - | OT +18 AND Hermit Merchant dead (m60_51_56) |

### Abandoned Merchant (4800-4819)

| flag | stage | trigger |
|---|---|---|
| 4805 (+5) | - | +6 (m60_37_46) |
| 4806 (+6) | - | OT +6 AND +5 AND ((+6) OR HP of Hermit Merchant AND (12019006 OR  OR (+6 OR +7) OR HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND ( OR  OR ))) (m12_01_00) |
| 4807 (+7) | - | OT +6 AND +5 AND ((+6) OR HP of Hermit Merchant AND (12019006 OR  OR (+6 OR +7) OR HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND HP of Hermit Merchant AND ( OR  OR ))) (m12_01_00) |
| 4808 (+8) | - | OT +8 AND Hermit Merchant dead (m12_01_00) |
| 4810 (+10) | - | +11 (m60_37_46) |
| 4811 (+11) | - | OT +11 AND +10 AND ((+11) OR HP of Imprisoned Merchant AND (12059106 OR  OR (+11 OR +12) OR HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND ( OR  OR ))) (m12_05_00) |
| 4812 (+12) | - | OT +11 AND +10 AND ((+11) OR HP of Imprisoned Merchant AND (12059106 OR  OR (+11 OR +12) OR HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND HP of Imprisoned Merchant AND ( OR  OR ))) (m12_05_00) |
| 4813 (+13) | - | OT +13 AND Imprisoned Merchant dead (m12_05_00) |
