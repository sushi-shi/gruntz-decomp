// BattlezMapConfig.cpp - CBattlezMapConfig::LoadConfig - the Battlez (multiplayer)
// map-config loader. Reads the [Battlez] tag-group of the global CButeMgr text
// config (g_buteMgr) into a freshly-zeroed config struct (this), seeds a pile of
// item/creation-rate fields, walks the level object tree twice to harvest the two
// player/team start markers into the start-coordinate arrays at this+0xdc / +0xf0,
// applies a per-difficulty (Easy/Normal/Hard) FP rescale to the two creation-time
// fields, then accumulates the per-item spawn-budget running totals into the
// this+0x150.. table. Returns 1.
//
// Signature: __thiscall int LoadConfig(CLevelInfo* lvl, int id, int diff).
//   lvl  (arg1, held in EBP) - the source level-info object; the loader copies a
//        handful of its fields (m_68/m_70/m_2c and the object-list root m_30) and
//        walks m_30->m_8 for the start markers.
//   id   (arg2) - stored at this+0x18 (the owning play-state / team id) AND used as
//        the marker-match key both tree loops compare against obj->m_124. Its stack
//        slot ([esp+0x20]) is later reused for the difficulty multiplier `r`.
//   diff (arg3) - the difficulty selector (0 Easy / 1 Normal / 2 Hard).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Plain /O2 /MT (no /GX): no stack C++ object,
// no EH frame. The CButeMgr getters are the already-matched leaf getters
// (butemgr unit), reloc-masked through the g_buteMgr singleton.
//
// Two tree-walk loops (the HARD part): each walks the level object list
// (lvl->m_30->m_8) via the engine's GetNext cursor idiom (head at +0x14, cursor at
// +0x64, node-next at +0x0, node-payload at +0x8), filters nodes whose +0x7c RTTI
// object reports type-id 5 AND whose +0x124 equals the per-map id (a stack local at
// [esp+0x20]), then pulls a 2-int pair node off the global freelist g_freeList
// (DAT_00645544), fills it with the marker's scaled (m_5c, m_60) coordinates, and
// hands it to SetAtGrow on the per-map start-coord array. Loop 1 scales by signed
// /32 (round toward zero) into the this+0xdc array; loop 2 scales by >>5 (floor)
// into the this+0xf0 array AND sets bit 0x10000 in the matched node's flags. The
// three RTTI type-descriptor pointers and the freelist global are reloc-masked
// engine externs (no body / data).
// ---------------------------------------------------------------------------
#include <rva.h>

// The global CButeMgr text-config tree (the singleton). Modeled as a minimal class
// so the `ecx=&g_buteMgr; call Get*` shape reloc-masks against the already-matched
// CButeMgr getters (butemgr unit).
#include <Bute/ButeMgr.h>
#include <Globals.h>
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// ---- Reloc-masked engine externs --------------------------------------------
// The 2-int-pair freelist head: a singly-linked list of recycled coord-pair nodes
// (node->next at +0, the usable pair at +4/+8). Referenced as data (DIR32).
DATA(0x00245544)
extern void* g_freeList;

// The per-map start-coord array's SetAtGrow appender (callee-cleanup engine free
// fn; 2 args). The marker pair node is appended to the array handle (arr->m_8).
extern "C" void __stdcall SetAtGrow(i32 arrayHandle, void* node);

// The three engine RTTI type-descriptor records the marker filters key off. Each
// loop's type test is `obj->m_7c->m_10 == (typeId)`, where the engine encodes the
// type id as `descriptor_address + 5`: the compiled `cmp $5, [rtti+0x10]` carries a
// DIR32 reloc to the descriptor on its immediate (imm32 = &descN + 5). Modeling the
// RHS as `(int)(&descN + 5)` reproduces that relocation byte-for-byte. The records
// are never dereferenced - only their address rides the immediate.

// The C runtime PRNG (reloc-masked).
extern "C" i32 rand(void);

// The FP scale constant the difficulty rescale multiplies by (a 4-byte float in
// .data; fmuls reads it). Reloc-masked const datum.

// The difficulty-tier sink the rescale stamps (5 Hard / 10 Normal / 20 Easy).

// ---------------------------------------------------------------------------
// The level object-list node + the per-tag store the loops walk.
//   CLevelNode  - a list cell: m_0 next, m_8 payload object.
//   CLevelObj   - a walked object: +0x10 type-id (via its +0x7c RTTI record),
//                 +0x5c/+0x60 marker coords, +0x124 map id, +0x8 flags.
//   CLevelList  - the list head: +0x14 first, +0x64 GetNext cursor.
// ---------------------------------------------------------------------------
// The object hung off CLevelInfo::m_2c: only its +0x2e4 word is read (into m_14).
struct CLevelSpawnInfo {
    char m_pad00[0x2e4];
    i32 m_2e4; // +0x2e4  read into CBattlezMapConfig::m_14
};

struct CLevelObj; // defined below
struct CMapDims;  // defined below
struct CLevelNode {
    CLevelNode* m_0; // +0x00  next cell
    char m_pad04[4];
    CLevelObj* m_8; // +0x08  payload object
};

// The +0x7c RTTI record: +0x10 is the engine type-id word the filter compares
// against `&g_typeDescN + 5` (see the type-descriptor externs above).
struct CRttiRec {
    char m_pad00[0x10];
    i32 m_10;
};

struct CLevelObj {
    char m_pad00[0x8];
    i32 m_8; // +0x08  flags (loop 2 ors in 0x10000)
    char m_pad0c[0x5c - 0xc];
    i32 m_5c; // +0x5c  marker X (scaled into the coord pair)
    i32 m_60; // +0x60  marker Y
    char m_pad64[0x7c - 0x64];
    CRttiRec* m_7c; // +0x7c  RTTI object whose +0x10 holds the type-id
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124 per-map id (matched against the map-id local)
};

struct CLevelList {
    char m_pad00[0x8];
    CLevelList* m_8; // +0x08  the actual object list ListGetFirst/Next walk
    char m_pad0c[0x14 - 0xc];
    CLevelNode* m_14; // +0x14  list first
    char m_pad18[0x64 - 0x18];
    CLevelNode* m_64; // +0x64  GetNext cursor
};

// The source level-info object (arg1, in EBP). Only the load-bearing reads.
struct CLevelInfo {
    char m_pad00[0x2c];
    CLevelSpawnInfo* m_2c; // +0x2c  copied to this+0x10
    CLevelList* m_30;      // +0x30  object-list root (its m_8 is the walked list)
    char m_pad34[0x68 - 0x34];
    void* m_68; // +0x68  copied to this+0x8
    char m_pad6c[0x70 - 0x6c];
    CMapDims* m_70; // +0x70  copied to this+0xc
};

// The 2-int coord-pair node pulled off the freelist (the slot the SetAtGrow array
// stores). The freelist links through node->m_0; the usable pair is m_4/m_8.
struct CCoordPair {
    void* m_0; // +0x00  freelist next
    i32 m_4;   // +0x04  X
    i32 m_8;   // +0x08  Y
};

// The per-map start-coord array sub-object (embedded at this+0xdc and this+0xf0).
// SetAtGrow takes the inner handle at +0x8.
struct CStartArray {
    char m_pad00[8];
    i32 m_8; // +0x08  array handle passed to SetAtGrow
};

// The dims object m_c points at: its +0xc word drives the /3 and >>2 fields.
struct CMapDims {
    char m_pad00[0xc];
    u32 m_c;
};

// The list GetFirst/GetNext cursor idiom the three marker loops share. GetFirst
// seeds the cursor (m_64) from the head (m_14) and returns the first payload;
// GetNext advances the cursor (m_64 = node->m_0) and returns node->m_8. Both
// return 0 at the end. Marked inline so the bodies fold into each loop.
static inline CLevelObj* ListGetFirst(CLevelList* list) {
    CLevelNode* n = list->m_14;
    list->m_64 = n;
    if (n == 0) {
        return 0;
    }
    list->m_64 = n->m_0;
    return n->m_8;
}

static inline CLevelObj* ListGetNext(CLevelList* list) {
    CLevelNode* n = list->m_64;
    if (n == 0) {
        return 0;
    }
    list->m_64 = n->m_0;
    return n->m_8;
}

// ---------------------------------------------------------------------------
// CBattlezMapConfig - the Battlez map-config struct (this). Only the offsets the
// loader writes are reconstructed; the bulk is opaque padding.
// ---------------------------------------------------------------------------
class CBattlezMapConfig {
public:
    i32 LoadConfig(CLevelInfo* lvl, i32 id, i32 diff);

    i32 m_0;                 // +0x00  = 1
    CLevelInfo* m_levelInfo; // +0x04  = lvl
    void* m_8;               // +0x08  = lvl->m_68
    CMapDims* m_dims;        // +0x0c  = lvl->m_70
    CLevelSpawnInfo* m_10;   // +0x10  = lvl->m_2c
    i32 m_14;                // +0x14  = m_10->m_2e4
    i32 m_ownerId;           // +0x18  = id (arg2, owner/team id)
    char m_pad1c[0x30 - 0x1c];
    DWORD m_defenderChance; // +0x30  DefenderChance
    char m_pad34[0x48 - 0x34];
    DWORD m_gruntCreationTime;    // +0x48  GruntCreationTime (rescaled)
    DWORD m_4c;                   // +0x4c  (zeroed)
    i32 m_50;                     // +0x50  (zeroed)
    DWORD m_resourceCreationTime; // +0x54  ResourceCreationTime (rescaled)
    i32 m_58;                     // +0x58  (zeroed)
    i32 m_5c;                     // +0x5c  (zeroed)
    DWORD m_gauntletzChance;      // +0x60  GauntletzChance
    DWORD m_shovelzChance;        // +0x64  ShovelzChance
    DWORD m_spyzChance;           // +0x68  SpyzChance
    DWORD m_brickzChance;         // +0x6c  BrickzChance
    DWORD m_gooberzChance;        // +0x70  GooberzChance
    DWORD m_gruntRatio;           // +0x74  GruntRatio
    i32 m_78;                     // +0x78  (epilogue = 0)
    i32 m_7c;                     // +0x7c  (epilogue = 0)
    i32 m_80;                     // +0x80  (epilogue = 0)
    i32 m_84;                     // +0x84  (epilogue = 0)
    char m_pad88[0x8c - 0x88];
    i32 m_8c; // +0x8c  = 6
    i32 m_90; // +0x90  = 6
    i32 m_94; // +0x94  = 6
    i32 m_98; // +0x98  = 6
    char m_pad9c[0xa4 - 0x9c];
    i32 m_a4; // +0xa4  = 8
    char m_pada8[0xac - 0xa8];
    u32 m_ac; // +0xac  = (m_dims->m_c)/3
    u32 m_b0; // +0xb0  = (m_dims->m_c)/3
    char m_padb4[0xc0 - 0xb4];
    u32 m_c0; // +0xc0  = (m_dims->m_c)>>2
    char m_padc4[0xd0 - 0xc4];
    i32 m_markerX; // +0xd0  loop-1 fast-path marker X
    i32 m_markerY; // +0xd4  loop-1 fast-path marker Y
    char m_padd8[0xdc - 0xd8];
    CStartArray m_dc; // +0xdc  loop-1 start-coord array
    char m_padec[0xf0 - (0xdc + 0xc)];
    CStartArray m_f0; // +0xf0  loop-2 start-coord array
    char m_pad100[0x140 - (0xf0 + 0xc)];
    i32 m_140;              // +0x140 = 0
    i32 m_144;              // +0x144 = ((rand "mod" 4) + 5) * 25 * 8
    i32 m_148;              // +0x148 = 0
    i32 m_14c;              // +0x14c = 0
    i32 m_toolzPct;         // +0x150 ToolzPercent (running total seed)
    i32 m_toyzPct;          // +0x154 ToyzPercent
    i32 m_brickzPct;        // +0x158 BrickzPercent
    i32 m_redBrickPct;      // +0x15c RedBrick
    i32 m_blueBrickPct;     // +0x160 BlueBrick
    i32 m_goldBrickPct;     // +0x164 GoldBrick
    i32 m_blackBrickPct;    // +0x168 BlackBrick
    i32 m_babyWalkerzPct;   // +0x16c BabyWalkerz
    i32 m_beachBallzPct;    // +0x170 BeachBallz
    i32 m_bigWheelzPct;     // +0x174 BigWheelz
    i32 m_goKartzPct;       // +0x178 GoKartz
    i32 m_jackInTheBoxzPct; // +0x17c JackInTheBoxz
    i32 m_jumpRopezPct;     // +0x180 JumpRopez
    i32 m_pogoStickzPct;    // +0x184 PogoStickz
    i32 m_scrollzPct;       // +0x188 Scrollz
    i32 m_squeakToyzPct;    // +0x18c SqueakToyz
    i32 m_yoyozPct;         // +0x190 Yoyoz
    i32 m_bombzPct;         // +0x194 Bombz
    i32 m_boomerangzPct;    // +0x198 Boomerangz (+ Brickz)
    i32 m_clubzPct;         // +0x19c Clubz
    i32 m_gauntletzPct;     // +0x1a0 Gauntletz
    i32 m_glovezPct;        // +0x1a4 Glovez
    i32 m_gooberzPct;       // +0x1a8 Gooberz
    i32 m_gravityBootzPct;  // +0x1ac GravityBootz
    i32 m_gunHatzPct;       // +0x1b0 GunHatz
    i32 m_nerfGunzPct;      // +0x1b4 NerfGunz
    i32 m_rockzPct;         // +0x1b8 Rockz
    i32 m_shieldzPct;       // +0x1bc Shieldz
    i32 m_shovelzPct;       // +0x1c0 Shovelz
    i32 m_springzPct;       // +0x1c4 Springz
    i32 m_spyzPct;          // +0x1c8 Spyz
    i32 m_swordzPct;        // +0x1cc Swordz
    i32 m_timeBombzPct;     // +0x1d0 TimeBombz
    i32 m_toobzPct;         // +0x1d4 Toobz
    i32 m_wandzPct;         // +0x1d8 Wandz
    i32 m_welderzPct;       // +0x1dc Welderz
    i32 m_wingzPct;         // +0x1e0 Wingz
    i32 m_1e4;              // +0x1e4 (final running total)
};

// ===========================================================================
// CBattlezMapConfig::LoadConfig
// ===========================================================================
RVA(0x00025020, 0x984)
i32 CBattlezMapConfig::LoadConfig(CLevelInfo* lvl, i32 id, i32 diff) {
    // --- prologue: zero the scratch fields, copy the level-info handles. ---
    m_gruntCreationTime = 0;
    m_4c = 0;
    m_50 = 0;
    m_resourceCreationTime = 0;
    m_58 = 0;
    m_5c = 0;
    m_levelInfo = lvl;
    m_ownerId = id;
    m_8 = lvl->m_68;
    m_dims = lvl->m_70;
    m_10 = lvl->m_2c;
    m_14 = m_10->m_2e4;
    m_0 = 1;

    // --- the [Battlez] creation-rate / chance block. ---
    m_gruntCreationTime = g_buteMgr.GetDwordDef("Battlez", "GruntCreationTime", 10000);
    m_resourceCreationTime = g_buteMgr.GetDwordDef("Battlez", "ResourceCreationTime", 10000);
    m_gauntletzChance = g_buteMgr.GetDwordDef("Battlez", "GauntletzChance", 50);
    m_shovelzChance = g_buteMgr.GetDwordDef("Battlez", "ShovelzChance", 50);
    m_spyzChance = g_buteMgr.GetDwordDef("Battlez", "SpyzChance", 50);
    m_brickzChance = g_buteMgr.GetDwordDef("Battlez", "BrickzChance", 50);
    m_gooberzChance = g_buteMgr.GetDwordDef("Battlez", "GooberzChance", 50);
    m_gruntRatio = g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25);
    m_defenderChance = g_buteMgr.GetDwordDef("Battlez", "DefenderChance", 50);

    // --- loop 1: append EVERY type-1 start marker to the +0xdc array. The marker
    //     coords are scaled by signed /32 (round-toward-zero) into a freelist pair.
    //     The list is re-derived (lvl->m_30->m_8) and advanced via the GetNext
    //     cursor idiom on every step. ---
    for (CLevelObj* cur = ListGetFirst(lvl->m_30->m_8); cur != 0;
         cur = ListGetNext(lvl->m_30->m_8)) {
        if (cur->m_7c->m_10 == (i32)(g_typeDesc1 + 5) && cur->m_124 == id) {
            CCoordPair* p = (CCoordPair*)g_freeList;
            i32* slot = 0;
            if (p->m_0 != 0) {
                slot = &p->m_4;
                g_freeList = p->m_0;
            }
            slot[0] = cur->m_5c / 32;
            slot[1] = cur->m_60 / 32;
            SetAtGrow(m_dc.m_8, slot);
        }
    }

    // --- loop 2: find the FIRST type-2 marker, stamp m_markerX/m_markerY with its /32 coords,
    //     and stop (fall straight into loop 3). ---
    for (CLevelObj* cur2 = ListGetFirst(lvl->m_30->m_8); cur2 != 0;
         cur2 = ListGetNext(lvl->m_30->m_8)) {
        if (cur2->m_7c->m_10 == (i32)(g_typeDesc2 + 5) && cur2->m_124 == id) {
            m_markerX = cur2->m_5c / 32;
            m_markerY = cur2->m_60 / 32;
            break;
        }
    }

    // --- loop 3: append EVERY type-3 marker to the +0xf0 array, scaled by >>5
    //     (arithmetic floor), and set bit 0x10000 in the matched object's flags. ---
    for (CLevelObj* cur3 = ListGetFirst(lvl->m_30->m_8); cur3 != 0;
         cur3 = ListGetNext(lvl->m_30->m_8)) {
        if (cur3->m_7c->m_10 == (i32)(g_typeDesc3 + 5) && cur3->m_124 == id) {
            CCoordPair* p = (CCoordPair*)g_freeList;
            i32* slot = 0;
            if (p->m_0 != 0) {
                slot = &p->m_4;
                g_freeList = p->m_0;
            }
            slot[0] = cur3->m_5c >> 5;
            slot[1] = cur3->m_60 >> 5;
            SetAtGrow(m_f0.m_8, slot);
            cur3->m_8 |= 0x10000;
        }
    }

    // --- per-difficulty rescale of the two creation-time fields. ---
    switch (diff) {
        case 0: { // Easy
            g_buteMgr.GetIntDef("Battlez", "EasyDifficulty", 100);
            g_diffTier = 20;
            break;
        }
        case 1: { // Normal
            i32 r = g_buteMgr.GetIntDef("Battlez", "NormalDifficulty", 50);
            g_diffTier = 10;
            m_gruntCreationTime =
                (i32)((double)r * ((double)(i64)m_gruntCreationTime * g_diffScale));
            m_resourceCreationTime =
                (i32)((double)r * ((double)(i64)m_resourceCreationTime * g_diffScale));
            break;
        }
        case 2: { // Hard
            i32 r = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
            g_diffTier = 5;
            m_gruntCreationTime =
                (i32)((double)r * ((double)(i64)m_gruntCreationTime * g_diffScale));
            m_resourceCreationTime =
                (i32)((double)r * ((double)(i64)m_resourceCreationTime * g_diffScale));
            break;
        }
        default:
            break;
    }

    // --- mid-block scalar seeds. ---
    m_50 = 0;
    m_14c = 0;
    {
        i32 rv = rand();
        m_144 = ((rv % 4) + 5) * 125 * 8;
    }
    m_148 = 0;
    m_8c = 6;
    m_90 = 6;
    m_94 = 6;
    m_98 = 6;
    m_a4 = 8;
    m_ac = m_dims->m_c / 3;
    m_b0 = m_dims->m_c / 3;
    m_c0 = m_dims->m_c >> 2;
    m_140 = 0;

    // --- the per-item spawn-budget running totals (each = prev_total + GetInt). ---
    m_toolzPct = g_buteMgr.GetInt("Battlez", "ToolzPercent");
    m_toyzPct = m_toolzPct + g_buteMgr.GetInt("Battlez", "ToyzPercent");
    m_brickzPct = m_toyzPct + g_buteMgr.GetInt("Battlez", "BrickzPercent");
    m_redBrickPct = g_buteMgr.GetInt("Battlez", "RedBrick");
    m_blueBrickPct = m_redBrickPct + g_buteMgr.GetInt("Battlez", "BlueBrick");
    m_goldBrickPct = g_buteMgr.GetInt("Battlez", "GoldBrick");
    m_blackBrickPct = m_goldBrickPct + g_buteMgr.GetInt("Battlez", "BlackBrick");
    m_babyWalkerzPct = g_buteMgr.GetInt("Battlez", "BabyWalkerz");
    m_beachBallzPct = m_babyWalkerzPct + g_buteMgr.GetInt("Battlez", "BeachBallz");
    m_bigWheelzPct = g_buteMgr.GetInt("Battlez", "BigWheelz");
    m_goKartzPct = m_bigWheelzPct + g_buteMgr.GetInt("Battlez", "GoKartz");
    m_jackInTheBoxzPct = g_buteMgr.GetInt("Battlez", "JackInTheBoxz");
    m_jumpRopezPct = m_jackInTheBoxzPct + g_buteMgr.GetInt("Battlez", "JumpRopez");
    m_pogoStickzPct = g_buteMgr.GetInt("Battlez", "PogoStickz");
    m_scrollzPct = m_pogoStickzPct + g_buteMgr.GetInt("Battlez", "Scrollz");
    m_squeakToyzPct = g_buteMgr.GetInt("Battlez", "SqueakToyz");
    m_yoyozPct = m_squeakToyzPct + g_buteMgr.GetInt("Battlez", "Yoyoz");
    m_bombzPct = g_buteMgr.GetInt("Battlez", "Bombz");
    m_boomerangzPct = m_bombzPct + g_buteMgr.GetInt("Battlez", "Boomerangz");
    g_buteMgr.GetInt("Battlez", "Brickz");
    m_clubzPct = m_boomerangzPct + g_buteMgr.GetInt("Battlez", "Clubz");
    m_gauntletzPct = g_buteMgr.GetInt("Battlez", "Gauntletz");
    m_glovezPct = m_gauntletzPct + g_buteMgr.GetInt("Battlez", "Glovez");
    m_gooberzPct = g_buteMgr.GetInt("Battlez", "Gooberz");
    m_gravityBootzPct = m_gooberzPct + g_buteMgr.GetInt("Battlez", "GravityBootz");
    m_gunHatzPct = g_buteMgr.GetInt("Battlez", "GunHatz");
    m_nerfGunzPct = m_gunHatzPct + g_buteMgr.GetInt("Battlez", "NerfGunz");
    m_rockzPct = g_buteMgr.GetInt("Battlez", "Rockz");
    m_shieldzPct = m_rockzPct + g_buteMgr.GetInt("Battlez", "Shieldz");
    m_shovelzPct = g_buteMgr.GetInt("Battlez", "Shovelz");
    m_springzPct = m_shovelzPct + g_buteMgr.GetInt("Battlez", "Springz");
    m_spyzPct = g_buteMgr.GetInt("Battlez", "Spyz");
    m_swordzPct = m_spyzPct + g_buteMgr.GetInt("Battlez", "Swordz");
    m_timeBombzPct = g_buteMgr.GetInt("Battlez", "TimeBombz");
    m_toobzPct = m_timeBombzPct + g_buteMgr.GetInt("Battlez", "Toobz");
    m_wandzPct = g_buteMgr.GetInt("Battlez", "Wandz");
    m_welderzPct = m_wandzPct + g_buteMgr.GetInt("Battlez", "Welderz");
    m_wingzPct = g_buteMgr.GetInt("Battlez", "Wingz");
    m_1e4 = m_wingzPct + g_buteMgr.GetInt("Battlez", "Wingz");

    // --- epilogue: clear the +0x78..+0x84 block, return 1. ---
    m_78 = 0;
    m_80 = 0;
    m_7c = 0;
    m_84 = 0;
    return 1;
}

SIZE_UNKNOWN(CBattlezMapConfig);
SIZE_UNKNOWN(CCoordPair);
SIZE_UNKNOWN(CLevelInfo);
SIZE_UNKNOWN(CLevelList);
SIZE_UNKNOWN(CLevelNode);
SIZE_UNKNOWN(CLevelObj);
SIZE_UNKNOWN(CLevelSpawnInfo);
SIZE_UNKNOWN(CMapDims);
SIZE_UNKNOWN(CRttiRec);
SIZE_UNKNOWN(CStartArray);
