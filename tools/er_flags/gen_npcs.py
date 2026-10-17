"""Generate the Elden Ring NPC table from the installed game data.

Usage:
    python gen_npcs.py "<...>/ELDEN RING/Game" <repo root>

Writes:
    src/parse/EldenRing/NpcProgress.h      table sizes
    src/parse/EldenRing/NpcProgress.cpp    NPCs

Every NPC owns a block of 20 global event flags in 3000-4999 (base % 20 == 0). The first
five (S = base .. base + 4) are its status: the scripts clear them (Batch Set Event Flags
S..S+3 or S..S+4 off) and set one, S friendly, S + 1 / S + 2 hostile, S + 3 dead, as in
Dark Souls III. The rest of the block holds the questline.

The scripts are the map event scripts (/event/<map>.emevd.dcx, EMEVD 0xCD) with the shared
events of common_func.emevd (Initialize Common Event 2000:06: slot, event id, args). The NPC
is the character the status events act on (IF Character Dead/Alive 4:00, IF Character HP
Ratio 4:02) -> map MSB enemy part (MSBE: type at +0xC, entity data at the offset at +0x60,
type data at the offset at +0x68; entity id at entity data +0, NpcParam id at type data
+0xC, talk id at +0x10) -> NpcParam +0xC name id -> NpcName.fmg (item, item_dlc01,
item_dlc02 msgbnd). Blocks whose events name no character are named by the talk script
that reads the block's flags most (t<talk id>.esd in /script/talk/<map>.talkesdbnd.dcx ->
the MSB parts with that talk id). Blocks named by neither are left out.

Requires pycryptodome and zstandard (see tools/er_items/erdata.py).
"""
import collections
import os
import re
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "er_items"))
import erdata  # noqa: E402

NPC_FLAG_MIN, NPC_FLAG_MAX = 3000, 5000
NPC_BLOCK = 20
NPC_STATUS = 5
NPC_STAGE_LAST = 18             # base + 5 .. base + 18 questline; base + 19 is set on its own
ENEMY_PART = 2
ITEM_MSG = ("/msg/engus/item.msgbnd.dcx", "/msg/engus/item_dlc01.msgbnd.dcx", "/msg/engus/item_dlc02.msgbnd.dcx")
# Characters NpcName calls the same; numbered in flag order, e.g. "Nomadic Merchant (2)".
NPC_NAME_PLACEHOLDERS = {"DLC dummy"}


# Questline stage of the flags the event scripts set in an NPC block, written from what the
# setting event waits for (docs/er-npc-stages.md); None where that doesn't say enough. The
# events either move one stage to the next (previous stage AND a boss, item, dialogue or
# another NPC's stage) or, on load, pick the stage from world flags (SKIP IF Event Flag).
NPC_STAGES = {
    # Tanith, Volcano Manor Proprietress
    3105: "Volcano Manor",
    3106: "Volcano Manor: you joined",
    3107: None,
    3108: "After Rykard",
    3109: "After Rykard",
    3110: "Devouring Rykard",
    3111: "Her knight is dead",
    3118: None,
    # White Mask Varré
    3185: "The First Step",
    3187: "After Roundtable Hold",
    3188: "After meeting the Finger Reader",
    3189: "Rose Church",
    3190: "Rose Church: gave you his task",
    3191: None,
    3192: "Rose Church: task done",
    3193: "Mohgwyn Palace",
    3198: None,
    # Smithing Master Hewg
    3225: "Roundtable Hold",
    3226: "Roundtable Hold: after the Erdtree burned",
    3227: None,
    3228: None,
    3238: None,
    # Gatekeeper Gostoc
    3265: "Stormveil Castle: at the gate",
    3266: "Stormveil Castle: met",
    3268: "Stormveil Castle: met again",
    3269: "After Godrick",
    3270: "Stormveil Castle: Nepheli its lord",
    3278: None,
    # Witch-Hunter Jerren
    3365: "Not met yet",
    3366: "Redmane Castle: festival announced",
    3367: "After Radahn",
    3368: "Left Redmane Castle",
    3369: "Witchbane Ruins",
    3370: "After Rennala",
    3371: "Raya Lucaria: fought Sellen",
    3372: "Thanked you",
    3378: None,
    # Lightseeker Hyetta
    3385: "Not met yet",
    3386: "Met",
    3387: "Waiting for a grape",
    3388: "Given a grape",
    3389: "Given another grape",
    3390: "Given a Shabriri Grape",
    3391: "Moved on",
    3392: "Thanked you",
    3393: "Before the Three Fingers",
    3398: None,
    # Castellan Edgar
    3405: "Castle Morne",
    3406: "Castle Morne: met",
    3407: "After the Leonine Misbegotten",
    3408: "Grieving Irina",
    3409: "Left",
    3418: None,
    # Zorayas the Scout
    3425: "Volcano Manor",
    3426: "Volcano Manor: met",
    3427: "Volcano Manor: talked",
    3428: "Volcano Manor: told you of the ruins",
    3429: "Volcano Manor: told you of the ruins",
    3430: "After Rykard",
    3431: "After Rykard: gave up",
    3438: None,
    # Potentate Diallos
    3445: "Not met yet",
    3446: None,
    3447: None,
    3448: None,
    3449: "Volcano Manor: met again",
    3450: "Jarburg",
    3451: "Jarburg",
    3456: None,
    3457: None,
    3458: None,
    # Sorceress Sellen
    3465: "Waypoint Ruins",
    3466: "Jerren at Witchbane Ruins",
    3467: "After Rennala",
    3468: "After Rennala",
    3469: "Raya Lucaria: lessons again",
    3478: None,
    # Finger Reader Enia
    3485: "Roundtable Hold",
    3486: None,
    3487: None,
    3488: "Roundtable Hold: after the Erdtree burned",
    3489: None,
    3498: None,
    # Old Albus
    3545: "Village of the Albinaurics",
    3546: "Gave you the medallion half",
    3558: None,
    # Preceptor Seluvis
    3565: "Not met yet",
    3566: "Ranni's Rise: met",
    3567: None,
    3568: None,
    3569: "Gone",
    3578: None,
    # Kenneth Haight, Limgrave Heir
    3585: "Limgrave",
    3586: "Asked you to take Fort Haight",
    3587: "Stormveil Castle: Nepheli its lord",
    3598: None,
    # Blaidd the Half-Wolf
    3605: "Mistwood: met",
    3606: "Told you of Darriwil",
    3607: "Darriwil defeated",
    3608: "Met again",
    3609: "Ranni's Rise",
    3610: "Ranni's Rise: serving Ranni",
    3611: "Redmane Castle: festival announced",
    3612: "Redmane Castle",
    3613: "After Radahn",
    3614: "Ranni's Rise: waiting",
    3615: "Ranni's Rise",
    3616: "Going to see Ranni",
    3617: "Going to see Ranni",
    3618: None,
    # Yura, Hunter of Bloody Fingers
    3625: "Murkwater: met",
    3626: "Met again",
    3627: None,
    3630: None,
    3631: None,
    3638: None,
    # Gurranq, Beast Clergyman
    3645: "Bestial Sanctum",
    3646: "Given Deathroot",
    3647: "Turned feral",
    3648: "Given more Deathroot",
    3658: None,
    # Alexander, Warrior Jar
    3665: "Stuck in the ground",
    3666: "Freed, heading east",
    3667: "Redmane Castle: festival announced",
    3668: "After Radahn",
    3669: "Mt. Gelmir: met again",
    3670: "Mt. Gelmir: freed again",
    3671: "After the Fire Giant",
    3678: None,
    # Patches
    3685: None,
    3686: None,
    3687: "Murkwater Cave: spared",
    3688: None,
    3689: "Volcano Manor",
    3690: "Volcano Manor",
    3691: "Met again",
    3692: None,
    3693: None,
    3694: None,
    3696: "Met again",
    3698: None,
    # Roderika, Spirit Tuner
    3705: "Stormhill Shack",
    3707: "Roundtable Hold: spirit tuner",
    3708: "Roundtable Hold: Hewg spoke of her",
    3709: "Roundtable Hold: after the Erdtree burned",
    3718: None,
    # Miriel, Pastor of Vows
    3725: "Church of Vows",
    3738: None,
    # Ranni the Witch
    3745: "Ranni's Rise",
    3746: "Church of Elleh: met",
    3747: "Offered you her service",
    3748: "You serve her",
    3749: None,
    3750: None,
    3751: None,
    3758: None,
    # Smithing Master Iji
    3765: "Near Ranni's Rise",
    3766: None,
    3767: None,
    3768: None,
    3778: None,
    # Sorcerer Thops
    3805: "Church of Irith",
    3806: "Given the Academy key",
    3818: None,
    # Jar-Bairn
    3825: "Jarburg",
    3826: "Jarburg: met",
    3827: "Jarburg",
    3828: "Jarburg: Diallos is dead",
    3829: None,
    3838: None,
    # Knight Bernahl
    3885: "Volcano Manor",
    3886: "Volcano Manor: you joined",
    3887: "After Rykard",
    3888: "After Rykard",
    3898: None,
    # Sorcerer Rogier
    3905: "Stormveil Castle",
    3906: "Roundtable Hold",
    3907: "Roundtable Hold: researching",
    3908: "Told you of the Black Knives",
    3909: None,
    3918: None,
    # Boc the Seamster
    3945: "Limgrave",
    3946: "Met",
    3947: "Following you",
    3948: "After Rennala",
    3949: "After Rennala",
    3954: None,
    3955: None,
    3956: None,
    3957: None,
    3958: None,
    # Sir Gideon Ofnir, the All-Knowing
    3965: "Roundtable Hold",
    3966: "Roundtable Hold: talked",
    3967: "Roundtable Hold: talked again",
    3968: None,
    3978: None,
    # D, Hunter of the Dead
    4045: "Limgrave: met",
    4046: "After the Tibia Mariner",
    4047: "Roundtable Hold",
    4048: "Roundtable Hold: watching Fia",
    4058: None,
    # D, Beholder of Death
    4065: "Deeproot Depths",
    4066: "After Fia's quest",
    4067: "No regrets",
    4077: None,
    4078: None,
    # Latenna the Albinauric
    4105: "Lakeside Crystal Cave",
    4106: None,
    4118: None,
    # Fia, Deathbed Companion
    4125: "Roundtable Hold",
    4126: "Roundtable Hold: D watching her",
    4127: "Deeproot Depths",
    4128: "After Fia's Champions",
    4129: "Lying with Godwyn",
    4130: "Waiting for Fortissax",
    4131: "After Lichdragon Fortissax",
    4132: None,
    4137: None,
    4138: None,
    # Blackguard
    4145: "Liurnia",
    4146: None,
    4147: None,
    4158: None,
    # Sage Gowry
    4165: "Gowry's Shack",
    4166: None,
    4167: "Gowry's Shack: Millicent cured",
    4168: "Millicent at the Haligtree",
    4169: "Millicent's choice made",
    4170: "Millicent's choice made",
    4171: None,
    4178: None,
    # Millicent
    4185: "Church of the Plague",
    4186: "Cured",
    4187: "Gowry's Shack",
    4188: "After the Godskin Apostle",
    4189: "Met again",
    4190: "Met again",
    4191: None,
    4192: "Met again",
    4193: "Haligtree",
    4194: None,
    4197: None,
    4198: None,
    # Scribe Corhyn
    4205: "Roundtable Hold",
    4206: "Roundtable Hold: talked",
    4207: "Looking for Goldmask",
    4208: "Leaving to join Goldmask",
    4209: "With Goldmask",
    4210: "Goldmask's path done",
    4211: "Goldmask's path done, terrified",
    4217: None,
    4218: None,
    # Nepheli Loux, Warrior
    4225: "Stormveil Castle",
    4226: "Roundtable Hold",
    4227: "Met again",
    4228: "After the Omenkiller",
    4229: "Lord of Stormveil Castle",
    4230: None,
    4238: None,
    # Dung Eater
    4245: "Roundtable Hold",
    4246: "Roundtable Hold: talked",
    4247: None,
    4248: None,
    4249: None,
    4251: None,
    4258: None,
    # Igon
    4265: "Not met yet",
    4267: None,
    4268: None,
    4278: None,
    # St. Trina
    4285: "Not met yet",
    4286: None,
    4298: None,
    # Fire Knight Queelign
    4345: "Not met yet",
    4346: None,
    4358: None,
    # Hornsent
    4365: "Met",
    4366: None,
    4368: None,
    4369: None,
    4370: None,
    4378: None,
    # Sir Ansbach
    4405: "Not met yet",
    4406: "Met",
    4407: None,
    4408: None,
    4418: None,
    # Redmane Freyja
    4425: "Not met yet",
    4426: "Talked of Leda",
    4427: None,
    4438: None,
    # Needle Knight Leda
    4445: "Mohgwyn Palace: met",
    4446: "Realm of Shadow",
    4447: "Said farewell",
    4448: None,
    4458: None,
    # Thiollier
    4465: "Not met yet",
    4466: "Met",
    4467: None,
    4468: None,
    4469: None,
    4478: None,
    # Hornsent Grandam
    4485: None,
    4498: None,
    # Count Ymir, Mother of Fingers
    4505: None,
    4506: None,
    4518: None,
    # Merchants (no questline; their other flags are wares and sales)
    4705: "Church of Elleh",
    4717: None,
    4718: None,
}
# Merchants' wares and sales flags.
for flag in (4725, 4726, 4727, 4728, 4730, 4731, 4732, 4733, 4735, 4736, 4737, 4738,
          4745, 4746, 4747, 4748, 4750, 4751, 4752, 4753, 4755, 4756, 4757, 4758,
          4765, 4766, 4767, 4768, 4770, 4771, 4772, 4773, 4775, 4776, 4777, 4778,
          4785, 4790, 4791, 4792, 4793, 4795, 4796, 4797, 4798,
          4805, 4806, 4807, 4808, 4810, 4811, 4812, 4813):
    NPC_STAGES[flag] = None


def map_names():
    """Candidate map ids: legacy dungeons mAA_BB_CC_00 and the overworld tiles of m60/m61."""
    names = [f"m{aa}_{bb:02d}_{cc:02d}_00" for aa in range(10, 46) for bb in range(100) for cc in (0, 1)]
    names += [f"m{aa}_{x:02d}_{y:02d}_{z:02d}" for aa in (60, 61) for x in range(70) for y in range(70)
              for z in (0, 1, 2, 10, 11, 12)]
    return names


def parse_emevd(data):
    """EMEVD 0xCD: {event id: [(bank, id, args, [(target, source, size)])]}."""
    assert data[:4] == b"EVD\0"
    (event_count, events_offset, _ic, instrs_offset, _uc, _uo, _lc, _lo,
     _pc, params_offset, _fc, _fo, _al, args_offset) = struct.unpack_from("<14q", data, 0x10)
    events = {}
    for i in range(event_count):
        event_id, count, offset, param_count, param_offset = struct.unpack_from(
            "<qqqqq", data, events_offset + i * 0x30)
        instrs = []
        for k in range(count):
            bank, index, size, arg_offset, _layer = struct.unpack_from(
                "<IIqqq", data, instrs_offset + offset + k * 0x20)
            start = args_offset + arg_offset
            instrs.append((bank, index, data[start:start + size], []))
        for k in range(param_count):
            instr, target, source, size = struct.unpack_from("<qqqi", data, params_offset + param_offset + k * 0x20)
            instrs[instr][3].append((target, source, size))
        events[event_id] = instrs
    return events


def substitute(args, params, values):
    out = bytearray(args)
    for target, source, size in params:
        if source + size <= len(values):
            out[target:target + size] = values[source:source + size]
    return bytes(out)


def event_instances(map_events, common_events):
    """Instructions of every initialized event with its arguments filled in, plus the map's
    events that run without initialization."""
    initialized = set()
    for instrs in map_events.values():
        for bank, index, args, _params in instrs:
            if (bank, index) == (2000, 0):      # Initialize Event: slot, event id, args
                events = map_events
            elif (bank, index) == (2000, 6):    # Initialize Common Event: slot, event id, args
                events = common_events
            else:
                continue
            event_id = struct.unpack_from("<I", args, 4)[0]
            if event_id in events:
                if events is map_events:
                    initialized.add(event_id)
                yield [(b, i, substitute(a, p, args[8:])) for b, i, a, p in events[event_id]]
    for event_id, instrs in map_events.items():
        if event_id not in initialized:
            yield [(b, i, a) for b, i, a, _p in instrs]


def status_blocks(instances):
    """{block base: Counter(character entity)} of the events that set a status flag."""
    blocks = collections.defaultdict(collections.Counter)
    for instrs in instances:
        characters = set()
        for bank, index, args in instrs:
            if bank == 4 and index in (0, 2):   # IF Character Dead/Alive, IF Character HP Ratio
                characters.add(struct.unpack_from("<I", args, 4)[0])
        cleared = []
        for bank, index, args in instrs:
            if bank == 2003 and index in (22, 63) and len(args) >= 9 and args[8] == 0:
                start, end = struct.unpack_from("<II", args, 0)
                if NPC_FLAG_MIN <= start < NPC_FLAG_MAX and start % NPC_BLOCK == 0 and end - start in (3, 4):
                    cleared.append(start)
            elif bank == 2003 and index in (66, 69) and len(args) >= 9 and args[8] == 1:
                flag = struct.unpack_from("<I", args, 4)[0]
                for start in cleared[-2:]:
                    if start <= flag < start + NPC_STATUS:
                        blocks[start].update(characters or {0})
    return blocks


def msb_enemies(data):
    """MSBE enemy parts: {entity id: (NpcParam id, talk id)}."""
    lists, offset = {}, 0x10
    while offset:
        _version, count, name_offset = struct.unpack_from("<iiq", data, offset)
        entries = struct.unpack_from(f"<{count - 1}q", data, offset + 16)
        end = name_offset
        while data[end:end + 2] != b"\0\0":
            end += 2
        lists[data[name_offset:end].decode("utf-16le", "ignore")] = entries
        offset = struct.unpack_from("<q", data, offset + 16 + (count - 1) * 8)[0]
    out = {}
    for part in lists["PARTS_PARAM_ST"]:
        if struct.unpack_from("<i", data, part + 0xC)[0] != ENEMY_PART:
            continue
        entity_offset, type_offset = struct.unpack_from("<qq", data, part + 0x60)
        entity = struct.unpack_from("<I", data, part + entity_offset)[0]
        npc, talk = struct.unpack_from("<ii", data, part + type_offset + 0xC)
        out[entity] = (npc, talk)
    return out


def stage_flags(instances):
    """Flags of the questline range (base + 5 .. base + 18) of any block the events set."""
    out = set()
    for instrs in instances:
        for bank, index, args in instrs:
            if bank == 2003 and index in (66, 69) and len(args) >= 9 and args[8] == 1:
                flag = struct.unpack_from("<I", args, 4)[0]
                if NPC_FLAG_MIN <= flag < NPC_FLAG_MAX and NPC_STATUS <= flag % NPC_BLOCK <= NPC_STAGE_LAST:
                    out.add(flag)
    return out


def npcs(archives):
    """([(status base, name)] of the named NPCs by flag, questline stage flags the events set)."""
    maps = [m for m in map_names() if archives.has(f"/event/{m}.emevd.dcx")]
    common = parse_emevd(archives.read("/event/common_func.emevd.dcx"))
    blocks = collections.defaultdict(collections.Counter)
    stages = set()
    for script in ["common"] + maps:
        instances = list(event_instances(parse_emevd(archives.read(f"/event/{script}.emevd.dcx")), common))
        for base, chars in status_blocks(instances).items():
            blocks[base].update(chars)
        stages |= stage_flags(instances)

    npc_rows = erdata.parse_param(archives.regulation()["NpcParam.param"])
    names = {}
    for path in ITEM_MSG:
        if archives.has(path):
            for name, fmg in archives.bnd(path).items():
                if os.path.basename(name.replace("\\", "/")).startswith("NpcName"):
                    names.update(erdata.parse_fmg2(fmg))
    entity_names, talk_names = {}, collections.defaultdict(collections.Counter)
    for m in maps:
        path = f"/map/mapstudio/{m}.msb.dcx"
        if not archives.has(path):
            continue
        for entity, (npc, talk) in msb_enemies(archives.read(path)).items():
            row = npc_rows.get(npc)
            name = names.get(struct.unpack_from("<i", row, 0xC)[0]) if row else None
            if name and name not in NPC_NAME_PLACEHOLDERS:
                entity_names[entity] = name
                if talk > 0:
                    talk_names[talk][name] += 1

    # Talk scripts reading each block's flags.
    talk_reads = collections.defaultdict(collections.Counter)
    for m in maps:
        path = f"/script/talk/{m}.talkesdbnd.dcx"
        if not archives.has(path):
            continue
        for fname, esd in archives.bnd(path).items():
            match = re.search(r"t(\d+)\.esd$", fname)
            if not match:
                continue
            for literal in re.finditer(rb"\x82(....)", esd, re.S):    # ESD int32 literal
                value = struct.unpack("<i", literal.group(1))[0]
                if NPC_FLAG_MIN <= value < NPC_FLAG_MAX:
                    talk_reads[value - value % NPC_BLOCK][int(match.group(1))] += 1

    out = []
    for base in sorted(blocks):
        votes = collections.Counter()
        for entity, n in blocks[base].items():
            if entity in entity_names:
                votes[entity_names[entity]] += n
        if not votes:
            for talk, _n in talk_reads[base].most_common(1):
                votes = talk_names.get(talk, collections.Counter())
        if votes:
            out.append((base, votes.most_common(1)[0][0]))
    seen = collections.Counter(name for _, name in out)
    numbered, count = [], collections.Counter()
    for base, name in out:
        count[name] += 1
        numbered.append((base, f"{name} ({count[name]})" if seen[name] > 1 else name))
    bases = {base for base, _ in numbered}
    return numbered, {f for f in stages if f - f % NPC_BLOCK in bases}


def cpp_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_crlf(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    game_dir, repo = sys.argv[1:]
    archives = erdata.Archives(game_dir)
    table, stages = npcs(archives)
    if stages != set(NPC_STAGES):
        raise RuntimeError(f"NPC stages changed: not reviewed {sorted(stages - set(NPC_STAGES))}, "
                           f"no script sets {sorted(set(NPC_STAGES) - stages)}")
    stage_table = sorted((f, t) for f, t in NPC_STAGES.items() if t)
    out_dir = os.path.join(repo, "src", "parse", "EldenRing")
    write_crlf(os.path.join(out_dir, "NpcProgress.h"), f"""#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// Generated by tools/er_flags/gen_npcs.py, do not edit.
namespace fssm::parse::er {{
    // Status flags of an NPC, exactly one set once its map has loaded:
    // statusFlag friendly, + 1 or + 2 hostile, + 3 dead.
    struct Npc {{
        uint32_t statusFlag;
        std::string_view name;
    }};

    // A questline stage: a set flag of statusFlag + 5 .. statusFlag + 18
    // (docs/er-npc-stages.md).
    struct NpcStage {{
        uint32_t flag;
        std::string_view text;
    }};

    // NPCs with a status flag block, by flag.
    extern const std::array<Npc, {len(table)}> NPCS;
    // Questline stage texts, by flag (not every stage flag has one).
    extern const std::array<NpcStage, {len(stage_table)}> NPC_STAGES;
}}
""")
    lines = ['#include "NpcProgress.h"', "", "// Generated by tools/er_flags/gen_npcs.py, do not edit.",
             "namespace fssm::parse::er {", f"    const std::array<Npc, {len(table)}> NPCS = {{"]
    lines += [f"        Npc{{{base}, {cpp_string(name)}}}," for base, name in table]
    lines += ["    };", "", f"    const std::array<NpcStage, {len(stage_table)}> NPC_STAGES = {{"]
    lines += [f"        NpcStage{{{flag}, {cpp_string(text)}}}," for flag, text in stage_table]
    lines += ["    };", "}", ""]
    write_crlf(os.path.join(out_dir, "NpcProgress.cpp"), "\n".join(lines))
    print(f"{len(table)} NPCs, {len(stage_table)} stage texts ({len(stages)} stage flags)")


if __name__ == "__main__":
    main()
