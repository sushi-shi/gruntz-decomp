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
DATA(0x2453d8)
extern CButeMgr g_buteMgr;

// ---- Reloc-masked engine externs --------------------------------------------
// The 2-int-pair freelist head: a singly-linked list of recycled coord-pair nodes
// (node->next at +0, the usable pair at +4/+8). Referenced as data (DIR32).
DATA(0x245544)
extern void *g_freeList;

// The per-map start-coord array's SetAtGrow appender (callee-cleanup engine free
// fn; 2 args). The marker pair node is appended to the array handle (arr->m_8).
extern "C" void __stdcall SetAtGrow(int arrayHandle, void *node);

// The three engine RTTI type-descriptor records the marker filters key off. Each
// loop's type test is `obj->m_7c->m_10 == (typeId)`, where the engine encodes the
// type id as `descriptor_address + 5`: the compiled `cmp $5, [rtti+0x10]` carries a
// DIR32 reloc to the descriptor on its immediate (imm32 = &descN + 5). Modeling the
// RHS as `(int)(&descN + 5)` reproduces that relocation byte-for-byte. The records
// are never dereferenced - only their address rides the immediate.
DATA(0x051510)
extern char g_typeDesc1[];
DATA(0x0c2640)
extern char g_typeDesc2[];
DATA(0x00b620)
extern char g_typeDesc3[];

// The C runtime PRNG (reloc-masked).
extern "C" int rand(void);

// The FP scale constant the difficulty rescale multiplies by (a 4-byte float in
// .data; fmuls reads it). Reloc-masked const datum.
DATA(0x1e96ec)
extern const float g_diffScale;

// The difficulty-tier sink the rescale stamps (5 Hard / 10 Normal / 20 Easy).
DATA(0x22b738)
extern int g_diffTier;

// ---------------------------------------------------------------------------
// The level object-list node + the per-tag store the loops walk.
//   CLevelNode  - a list cell: m_0 next, m_8 payload object.
//   CLevelObj   - a walked object: +0x10 type-id (via its +0x7c RTTI record),
//                 +0x5c/+0x60 marker coords, +0x124 map id, +0x8 flags.
//   CLevelList  - the list head: +0x14 first, +0x64 GetNext cursor.
// ---------------------------------------------------------------------------
struct CLevelNode {
    CLevelNode *m_0;     // +0x00  next cell
    char        m_pad04[4];
    void       *m_8;     // +0x08  payload object (a CLevelObj*)
};

// The +0x7c RTTI record: +0x10 is the engine type-id word the filter compares
// against `&g_typeDescN + 5` (see the type-descriptor externs above).
struct CRttiRec { char m_pad00[0x10]; int m_10; };

struct CLevelObj {
    char  m_pad00[0x8];
    int   m_8;           // +0x08  flags (loop 2 ors in 0x10000)
    char  m_pad0c[0x5c - 0xc];
    int   m_5c;          // +0x5c  marker X (scaled into the coord pair)
    int   m_60;          // +0x60  marker Y
    char  m_pad64[0x7c - 0x64];
    CRttiRec *m_7c;      // +0x7c  RTTI object whose +0x10 holds the type-id
    char  m_pad80[0x124 - 0x80];
    int   m_124;         // +0x124 per-map id (matched against the map-id local)
};

struct CLevelList {
    char        m_pad00[0x8];
    CLevelList *m_8;     // +0x08  the actual object list ListGetFirst/Next walk
    char        m_pad0c[0x14 - 0xc];
    CLevelNode *m_14;    // +0x14  list first
    char        m_pad18[0x64 - 0x18];
    CLevelNode *m_64;    // +0x64  GetNext cursor
};

// The source level-info object (arg1, in EBP). Only the load-bearing reads.
struct CLevelInfo {
    char        m_pad00[0x2c];
    void       *m_2c;     // +0x2c  copied to this+0x10
    CLevelList *m_30;     // +0x30  object-list root (its m_8 is the walked list)
    char        m_pad34[0x68 - 0x34];
    void       *m_68;     // +0x68  copied to this+0x8
    char        m_pad6c[0x70 - 0x6c];
    void       *m_70;     // +0x70  copied to this+0xc
};

// The 2-int coord-pair node pulled off the freelist (the slot the SetAtGrow array
// stores). The freelist links through node->m_0; the usable pair is m_4/m_8.
struct CCoordPair {
    void *m_0;     // +0x00  freelist next
    int   m_4;     // +0x04  X
    int   m_8;     // +0x08  Y
};

// The per-map start-coord array sub-object (embedded at this+0xdc and this+0xf0).
// SetAtGrow takes the inner handle at +0x8.
struct CStartArray {
    char m_pad00[8];
    int  m_8;      // +0x08  array handle passed to SetAtGrow
};

// The dims object m_c points at: its +0xc word drives the /3 and >>2 fields.
struct CMapDims { char m_pad00[0xc]; unsigned int m_c; };

// The list GetFirst/GetNext cursor idiom the three marker loops share. GetFirst
// seeds the cursor (m_64) from the head (m_14) and returns the first payload;
// GetNext advances the cursor (m_64 = node->m_0) and returns node->m_8. Both
// return 0 at the end. Marked inline so the bodies fold into each loop.
static inline CLevelObj *ListGetFirst(CLevelList *list)
{
    CLevelNode *n = list->m_14;
    list->m_64 = n;
    if (n == 0)
        return 0;
    list->m_64 = n->m_0;
    return (CLevelObj *)n->m_8;
}

static inline CLevelObj *ListGetNext(CLevelList *list)
{
    CLevelNode *n = list->m_64;
    if (n == 0)
        return 0;
    list->m_64 = n->m_0;
    return (CLevelObj *)n->m_8;
}

// ---------------------------------------------------------------------------
// CBattlezMapConfig - the Battlez map-config struct (this). Only the offsets the
// loader writes are reconstructed; the bulk is opaque padding.
// ---------------------------------------------------------------------------
class CBattlezMapConfig {
public:
    int LoadConfig(CLevelInfo *lvl, int id, int diff);

    int          m_0;          // +0x00  = 1
    CLevelInfo  *m_4;          // +0x04  = lvl
    void        *m_8;          // +0x08  = lvl->m_68
    CMapDims    *m_c;          // +0x0c  = lvl->m_70
    void        *m_10;         // +0x10  = lvl->m_2c
    int          m_14;         // +0x14  = ((int*)m_10)[0x2e4/4]
    void        *m_18;         // +0x18  = id (arg2)
    char         m_pad1c[0x30 - 0x1c];
    DWORD        m_30;         // +0x30  DefenderChance
    char         m_pad34[0x48 - 0x34];
    DWORD        m_48;         // +0x48  GruntCreationTime (rescaled)
    DWORD        m_4c;         // +0x4c  (zeroed)
    int          m_50;         // +0x50  (zeroed)
    DWORD        m_54;         // +0x54  ResourceCreationTime (rescaled)
    int          m_58;         // +0x58  (zeroed)
    int          m_5c;         // +0x5c  (zeroed)
    DWORD        m_60;         // +0x60  GauntletzChance
    DWORD        m_64;         // +0x64  ShovelzChance
    DWORD        m_68;         // +0x68  SpyzChance
    DWORD        m_6c;         // +0x6c  BrickzChance
    DWORD        m_70;         // +0x70  GooberzChance
    DWORD        m_74;         // +0x74  GruntRatio
    int          m_78;         // +0x78  (epilogue = 0)
    int          m_7c;         // +0x7c  (epilogue = 0)
    int          m_80;         // +0x80  (epilogue = 0)
    int          m_84;         // +0x84  (epilogue = 0)
    char         m_pad88[0x8c - 0x88];
    int          m_8c;         // +0x8c  = 6
    int          m_90;         // +0x90  = 6
    int          m_94;         // +0x94  = 6
    int          m_98;         // +0x98  = 6
    char         m_pad9c[0xa4 - 0x9c];
    int          m_a4;         // +0xa4  = 8
    char         m_pada8[0xac - 0xa8];
    unsigned int m_ac;         // +0xac  = (m_c->m_c)/3
    unsigned int m_b0;         // +0xb0  = (m_c->m_c)/3
    char         m_padb4[0xc0 - 0xb4];
    unsigned int m_c0;         // +0xc0  = (m_c->m_c)>>2
    char         m_padc4[0xd0 - 0xc4];
    int          m_d0;         // +0xd0  loop-1 fast-path marker X
    int          m_d4;         // +0xd4  loop-1 fast-path marker Y
    char         m_padd8[0xdc - 0xd8];
    CStartArray  m_dc;         // +0xdc  loop-1 start-coord array
    char         m_padec[0xf0 - (0xdc + 0xc)];
    CStartArray  m_f0;         // +0xf0  loop-2 start-coord array
    char         m_pad100[0x140 - (0xf0 + 0xc)];
    int          m_140;        // +0x140 = 0
    int          m_144;        // +0x144 = ((rand "mod" 4) + 5) * 25 * 8
    int          m_148;        // +0x148 = 0
    int          m_14c;        // +0x14c = 0
    int          m_150;        // +0x150 ToolzPercent (running total seed)
    int          m_154;        // +0x154 ToyzPercent
    int          m_158;        // +0x158 BrickzPercent
    int          m_15c;        // +0x15c RedBrick
    int          m_160;        // +0x160 BlueBrick
    int          m_164;        // +0x164 GoldBrick
    int          m_168;        // +0x168 BlackBrick
    int          m_16c;        // +0x16c BabyWalkerz
    int          m_170;        // +0x170 BeachBallz
    int          m_174;        // +0x174 BigWheelz
    int          m_178;        // +0x178 GoKartz
    int          m_17c;        // +0x17c JackInTheBoxz
    int          m_180;        // +0x180 JumpRopez
    int          m_184;        // +0x184 PogoStickz
    int          m_188;        // +0x188 Scrollz
    int          m_18c;        // +0x18c SqueakToyz
    int          m_190;        // +0x190 Yoyoz
    int          m_194;        // +0x194 Bombz
    int          m_198;        // +0x198 Boomerangz (+ Brickz)
    int          m_19c;        // +0x19c Clubz
    int          m_1a0;        // +0x1a0 Gauntletz
    int          m_1a4;        // +0x1a4 Glovez
    int          m_1a8;        // +0x1a8 Gooberz
    int          m_1ac;        // +0x1ac GravityBootz
    int          m_1b0;        // +0x1b0 GunHatz
    int          m_1b4;        // +0x1b4 NerfGunz
    int          m_1b8;        // +0x1b8 Rockz
    int          m_1bc;        // +0x1bc Shieldz
    int          m_1c0;        // +0x1c0 Shovelz
    int          m_1c4;        // +0x1c4 Springz
    int          m_1c8;        // +0x1c8 Spyz
    int          m_1cc;        // +0x1cc Swordz
    int          m_1d0;        // +0x1d0 TimeBombz
    int          m_1d4;        // +0x1d4 Toobz
    int          m_1d8;        // +0x1d8 Wandz
    int          m_1dc;        // +0x1dc Welderz
    int          m_1e0;        // +0x1e0 Wingz
    int          m_1e4;        // +0x1e4 (final running total)
};

// ===========================================================================
// CBattlezMapConfig::LoadConfig
// ===========================================================================
RVA(0x025020, 0x984)
int CBattlezMapConfig::LoadConfig(CLevelInfo *lvl, int id, int diff)
{
    // --- prologue: zero the scratch fields, copy the level-info handles. ---
    m_48 = 0;
    m_4c = 0;
    m_50 = 0;
    m_54 = 0;
    m_58 = 0;
    m_5c = 0;
    m_4  = lvl;
    m_18 = (void *)id;
    m_8  = lvl->m_68;
    m_c  = (CMapDims *)lvl->m_70;
    m_10 = lvl->m_2c;
    m_14 = ((int *)m_10)[0x2e4 / 4];
    m_0  = 1;

    // --- the [Battlez] creation-rate / chance block. ---
    m_48 = g_buteMgr.GetDwordDef("Battlez", "GruntCreationTime", 10000);
    m_54 = g_buteMgr.GetDwordDef("Battlez", "ResourceCreationTime", 10000);
    m_60 = g_buteMgr.GetDwordDef("Battlez", "GauntletzChance", 50);
    m_64 = g_buteMgr.GetDwordDef("Battlez", "ShovelzChance", 50);
    m_68 = g_buteMgr.GetDwordDef("Battlez", "SpyzChance", 50);
    m_6c = g_buteMgr.GetDwordDef("Battlez", "BrickzChance", 50);
    m_70 = g_buteMgr.GetDwordDef("Battlez", "GooberzChance", 50);
    m_74 = g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25);
    m_30 = g_buteMgr.GetDwordDef("Battlez", "DefenderChance", 50);

    // --- loop 1: append EVERY type-1 start marker to the +0xdc array. The marker
    //     coords are scaled by signed /32 (round-toward-zero) into a freelist pair.
    //     The list is re-derived (lvl->m_30->m_8) and advanced via the GetNext
    //     cursor idiom on every step. ---
    for (CLevelObj *cur = ListGetFirst(lvl->m_30->m_8);
         cur != 0;
         cur = ListGetNext(lvl->m_30->m_8)) {
        if (cur->m_7c->m_10 == (int)(g_typeDesc1 + 5) && cur->m_124 == id) {
            CCoordPair *p = (CCoordPair *)g_freeList;
            int *slot = 0;
            if (p->m_0 != 0) {
                slot = &p->m_4;
                g_freeList = p->m_0;
            }
            slot[0] = cur->m_5c / 32;
            slot[1] = cur->m_60 / 32;
            SetAtGrow(m_dc.m_8, slot);
        }
    }

    // --- loop 2: find the FIRST type-2 marker, stamp m_d0/m_d4 with its /32 coords,
    //     and stop (fall straight into loop 3). ---
    for (CLevelObj *cur2 = ListGetFirst(lvl->m_30->m_8);
         cur2 != 0;
         cur2 = ListGetNext(lvl->m_30->m_8)) {
        if (cur2->m_7c->m_10 == (int)(g_typeDesc2 + 5) && cur2->m_124 == id) {
            m_d0 = cur2->m_5c / 32;
            m_d4 = cur2->m_60 / 32;
            break;
        }
    }

    // --- loop 3: append EVERY type-3 marker to the +0xf0 array, scaled by >>5
    //     (arithmetic floor), and set bit 0x10000 in the matched object's flags. ---
    for (CLevelObj *cur3 = ListGetFirst(lvl->m_30->m_8);
         cur3 != 0;
         cur3 = ListGetNext(lvl->m_30->m_8)) {
        if (cur3->m_7c->m_10 == (int)(g_typeDesc3 + 5) && cur3->m_124 == id) {
            CCoordPair *p = (CCoordPair *)g_freeList;
            int *slot = 0;
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
        int r = g_buteMgr.GetIntDef("Battlez", "NormalDifficulty", 50);
        g_diffTier = 10;
        m_48 = (long)((double)r * ((double)(__int64)m_48 * g_diffScale));
        m_54 = (long)((double)r * ((double)(__int64)m_54 * g_diffScale));
        break;
    }
    case 2: { // Hard
        int r = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
        g_diffTier = 5;
        m_48 = (long)((double)r * ((double)(__int64)m_48 * g_diffScale));
        m_54 = (long)((double)r * ((double)(__int64)m_54 * g_diffScale));
        break;
    }
    default:
        break;
    }

    // --- mid-block scalar seeds. ---
    m_50  = 0;
    m_14c = 0;
    {
        int rv = rand();
        m_144 = ((rv % 4) + 5) * 125 * 8;
    }
    m_148 = 0;
    m_8c  = 6;
    m_90  = 6;
    m_94  = 6;
    m_98  = 6;
    m_a4  = 8;
    m_ac  = m_c->m_c / 3;
    m_b0  = m_c->m_c / 3;
    m_c0  = m_c->m_c >> 2;
    m_140 = 0;

    // --- the per-item spawn-budget running totals (each = prev_total + GetInt). ---
    m_150 = g_buteMgr.GetInt("Battlez", "ToolzPercent");
    m_154 = m_150 + g_buteMgr.GetInt("Battlez", "ToyzPercent");
    m_158 = m_154 + g_buteMgr.GetInt("Battlez", "BrickzPercent");
    m_15c = g_buteMgr.GetInt("Battlez", "RedBrick");
    m_160 = m_15c + g_buteMgr.GetInt("Battlez", "BlueBrick");
    m_164 = g_buteMgr.GetInt("Battlez", "GoldBrick");
    m_168 = m_164 + g_buteMgr.GetInt("Battlez", "BlackBrick");
    m_16c = g_buteMgr.GetInt("Battlez", "BabyWalkerz");
    m_170 = m_16c + g_buteMgr.GetInt("Battlez", "BeachBallz");
    m_174 = g_buteMgr.GetInt("Battlez", "BigWheelz");
    m_178 = m_174 + g_buteMgr.GetInt("Battlez", "GoKartz");
    m_17c = g_buteMgr.GetInt("Battlez", "JackInTheBoxz");
    m_180 = m_17c + g_buteMgr.GetInt("Battlez", "JumpRopez");
    m_184 = g_buteMgr.GetInt("Battlez", "PogoStickz");
    m_188 = m_184 + g_buteMgr.GetInt("Battlez", "Scrollz");
    m_18c = g_buteMgr.GetInt("Battlez", "SqueakToyz");
    m_190 = m_18c + g_buteMgr.GetInt("Battlez", "Yoyoz");
    m_194 = g_buteMgr.GetInt("Battlez", "Bombz");
    m_198 = m_194 + g_buteMgr.GetInt("Battlez", "Boomerangz");
    g_buteMgr.GetInt("Battlez", "Brickz");
    m_19c = m_198 + g_buteMgr.GetInt("Battlez", "Clubz");
    m_1a0 = g_buteMgr.GetInt("Battlez", "Gauntletz");
    m_1a4 = m_1a0 + g_buteMgr.GetInt("Battlez", "Glovez");
    m_1a8 = g_buteMgr.GetInt("Battlez", "Gooberz");
    m_1ac = m_1a8 + g_buteMgr.GetInt("Battlez", "GravityBootz");
    m_1b0 = g_buteMgr.GetInt("Battlez", "GunHatz");
    m_1b4 = m_1b0 + g_buteMgr.GetInt("Battlez", "NerfGunz");
    m_1b8 = g_buteMgr.GetInt("Battlez", "Rockz");
    m_1bc = m_1b8 + g_buteMgr.GetInt("Battlez", "Shieldz");
    m_1c0 = g_buteMgr.GetInt("Battlez", "Shovelz");
    m_1c4 = m_1c0 + g_buteMgr.GetInt("Battlez", "Springz");
    m_1c8 = g_buteMgr.GetInt("Battlez", "Spyz");
    m_1cc = m_1c8 + g_buteMgr.GetInt("Battlez", "Swordz");
    m_1d0 = g_buteMgr.GetInt("Battlez", "TimeBombz");
    m_1d4 = m_1d0 + g_buteMgr.GetInt("Battlez", "Toobz");
    m_1d8 = g_buteMgr.GetInt("Battlez", "Wandz");
    m_1dc = m_1d8 + g_buteMgr.GetInt("Battlez", "Welderz");
    m_1e0 = g_buteMgr.GetInt("Battlez", "Wingz");
    m_1e4 = m_1e0 + g_buteMgr.GetInt("Battlez", "Wingz");

    // --- epilogue: clear the +0x78..+0x84 block, return 1. ---
    m_78 = 0;
    m_80 = 0;
    m_7c = 0;
    m_84 = 0;
    return 1;
}
