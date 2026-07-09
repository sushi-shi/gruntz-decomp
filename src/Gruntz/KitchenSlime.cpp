#include <Mfc.h> // real MFC CString (direction-name match temp; reloc-masked)
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>
#include <Bute/ButeTree.h>
#include <rva.h>
#include <math.h>   // floor (0x120580) / ceil (0x120480) / fabs (inline d9 e1)
#include <string.h> // inline strcmp for the ctor's direction-name match
#include <Bute/ButeMgr.h>
#include <Gruntz/StringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h> // CUserLogic base (CKitchenSlime : CUserLogic) for the leaf-dtor fold
#include <Gruntz/Sprite.h>    // CSprite (frame-data value; the looked-up direction sprite)
#include <Globals.h>
#include <Gruntz/GameRegistry.h>  // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/SerialArchive.h> // shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialObjRef.h>  // the shared +0x34 serialized-object-reference (Chain @0x8c00)
#include <Wap32/ZDArrayDerived.h>
// KitchenSlime.cpp - CKitchenSlime::LoadSprites @0x0b3160 (C:\Proj\Gruntz). The
// kitchen-slime hazard's per-step "advance to the next walkable tile" driver: it
// probes up to four tiles in the slime's current travel direction (m_10->m_124),
// stopping at the first in-bounds, walkable, on-screen tile; rotates the travel
// direction when blocked; then sets the movement vector + direction sprite, the
// per-tile timing (Hazardz/KitchenSlimeTimePerTile butemgr default), and caches
// the new direction sprite's first frame. Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.
// CButeMgr g_buteMgr / the CUserLogic base hierarchy come from <Gruntz/UserLogic.h>.

// CSprite (frame-data value) comes from <Gruntz/Sprite.h>.

// The sub-object embedded in the anim player at +0x1a0 (a CSubMgr-style member);
// Tick advances it once per frame via its 0x55c360 method (one int arg).
// CSlimeSubMgr::Advance @0x15c360 IS CAniAdvanceCursor::Advance_15c360 (header-less); local decl.
class CAniAdvanceCursor {
public:
    i32 Advance_15c360(i32 clock);
};
struct CSlimeSubMgr {
    // Advance @0x15c360 IS CAniAdvanceCursor::Advance_15c360; cast at the call.
};

// The animation player @this+0x38 that holds the current direction sprite at
// +0x194 and the cached first-frame trio at +0x190/+0x194/+0x198.
// CSlimeAnimPlayer::CacheFirstFrame @0x150540 IS CGruntSprite::CacheFirstFrame (header-less); local decl.
class CGruntSprite {
public:
    void CacheFirstFrame(const char* name);
};
struct CSlimeAnimPlayer {
    // CacheFirstFrame @0x150540 IS CGruntSprite::CacheFirstFrame; cast at each call.

    char m_pad0[0x8];
    i32 m_8; // +0x08  status/flags word (Tick sets bit 0x10000 when stalled)
    char m_padc[0x190 - 0xc];
    i32 m_190;          // +0x190  first frame number
    CSprite* m_194;     // +0x194  the current direction sprite
    i32* m_198;         // +0x198  first frame pointer
    char m_pad19c[4];   // +0x19c
    CSlimeSubMgr m_1a0; // +0x1a0  per-frame sub-mgr (Advance)
};

// The slime's resource/level holder (this->m_10). m_124 = travel direction
// (1..4); m_5c/m_posX = per-step pixel deltas; m_134..m_140 = the on-screen tile
// window; m_12c = a "lock direction" flag; m_7c = the level/timing object whose
// +0xbc overrides the per-tile time.
// The level/timing object at CSlimeLevel+0x7c; its +0xbc overrides the per-tile time.
struct CSlimeTiming {
    char m_pad0[0xbc];
    u32 m_bc;
};

struct CSlimeLevel {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c  step dx (pixels)
    i32 m_60; // +0x60  step dy (pixels)
    char m_pad64[0x7c - 0x64];
    CSlimeTiming* m_7c; // +0x7c  level/timing object (m_7c->m_bc time override)
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124 travel direction (1..4)
    char m_pad128[0x12c - 0x128];
    i32 m_12c; // +0x12c lock-direction flag
    char m_pad130[0x134 - 0x130];
    i32 m_134; // +0x134 window min X
    i32 m_138; // +0x138 window min Y
    i32 m_13c; // +0x13c window max X
    i32 m_140; // +0x140 window max Y
    i32 m_144; // +0x144 on-screen rect base (Tick passes &m_144 to the cue gate)
};

// The level tile map reached via g_gameReg->m_tileGrid is the canonical CTileGrid
// (<Gruntz/TileGrid.h>): m_c/m_10 = grid extents, m_8 = the row table
// (row[gy][gx*7] is the tile-flags word).
// The on-screen object reached as g_gameReg->m_68 (the visibility/cue gate). Its
// QueryAt resolves the entity under the slime's screen rect and ScrollTo posts a
// scroll; modeled NO-body so both calls reloc-mask.
namespace m4 {
    struct HitGrunt;
    struct HitTileRect;
    class GruntHitMgr {
    public:
        HitGrunt* FindGruntAt(i32 x, i32 y, HitTileRect* r, i32* a, i32* b, struct tagRECT* rect);
    };
} // namespace m4
class CTriggerMgr {
public:
    i32 CellDispatch(i32 a, i32 b, i32 c, i32 d);
};
struct CSlimeCueGate {
    // QueryAt(level->m_5c, level->m_60, &level->m_144, &outA, &outB, 0) -> entity*.
    // QueryAt IS m4::GruntHitMgr::FindGruntAt; cast at the call.
    // ScrollTo IS CTriggerMgr::CellDispatch; cast at the call.
};

// The canonical CGameRegistry view of the singleton; m_posY (cue gate) and m_dirX
// (tile map) are void*/CTileGrid* here, cast locally at the deref sites.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The entity QueryAt returns; +0x258 is its type/state tag (0x38 == the slime
// itself, so its own footprint is ignored when probing the destination tile).
struct CSlimeEntity {
    char m_pad0[0x258];
    i32 m_258; // +0x258 type tag
};

// 32.0 (the per-tile-time -> per-frame-speed reciprocal numerator).

// Per-frame scroll/scale factor (.data int) Tick multiplies into m_speed to get the
// per-frame pixel step.
DATA(0x00245584)
extern i32 g_slimeFrameScale; // VA 0x645584

// 0.0 (the velocity-sign comparand for the movement integrator).
DATA(0x001ea400)
extern const double g_slimeZero; // VA 0x5ea400

// A frame/tick counter (BSS) the anim sub-mgr Advance consumes.
DATA(0x002bf3bc)
extern i32 g_slimeTick; // VA 0x6bf3bc

// CKitchenSlime : CUserLogic is modeled in <Gruntz/KitchenSlime.h> (canonical
// header, included below). The CUserLogic base gives the +0x18 destructible link,
// so ~CKitchenSlime folds the shared teardown (the /GX leaf-dtor archetype, see
// UserLogic.cpp 0x10ab0). CSlimeLevel/CSlimeAnimPlayer full defs live above.
#include <Gruntz/KitchenSlime.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CKitchenSlime::FireActivation
// (0x0b2940) dispatches through - the SAME archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150, see UserLogic.cpp), but a
// DIFFERENT registry instance (the slime's, at 0x646228 vs the trigger's at
// 0x244688). A coordinate maps to an Entry* either directly (when within the
// fast [g_kslimeLo,g_kslimeHi] range) via g_kslimeBase + (coord-lo)*stride, or
// by a slow Find in the collection (0x16da80, __thiscall ret 8), which on miss
// rebuilds (GetRetAddr 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret
// 0xc) and yields g_kslimeCur. The entry's first dword is a fn-ptr; a nonzero
// entry's handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (shared with the trigger registry's engine functions). The
// alloc-cache pair (g_actCache 0x6bf464 / g_retAddrBreadcrumb 0x6bf428) is the
// SAME shared global both registries write.
struct CKSlimeEntry;       // an entry: first dword is the registered handler
extern void* GetRetAddr(); // 0x16d990

DATA(0x00246228)
extern CTypeKeyColl g_kslimeColl;
DATA(0x002bf464)
extern void* g_actCache;
extern void* g_retAddrBreadcrumb;

// The entry's first dword is a pointer-to-member-function of CKitchenSlime
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CKitchenSlime is defined
// COMPLETE above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CKitchenSlime::*KSlimeHandler)();
struct CKSlimeEntry {
    KSlimeHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CKSlimeEntry* KSlimeLookup(i32 coord) {
    g_kslimeScratch = 0;
    if (coord >= g_kslimeLo && coord <= g_kslimeHi) {
        return (CKSlimeEntry*)(g_kslimeBase + (coord - g_kslimeLo) * g_kslimeStride);
    }
    if ((i32)((_zvec*)&g_kslimeColl)->GrowTo(coord, 0)) {
        return (CKSlimeEntry*)(g_kslimeBase + (coord - g_kslimeLo) * g_kslimeStride);
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_kslimeColl2->Set(&g_kslimeColl, (i32)item, 0xc);
    return g_kslimeCur;
}

// CKitchenSlime::~CKitchenSlime @0x013100 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to the established leaf
// dtors (UserLogic.cpp 0x10ab0 / 0x11540); the empty body is enough for cl.
RVA(0x00013100, 0x44)
CKitchenSlime::~CKitchenSlime() {}

// The global bute store the ctor binds the "A" node through (g_buteTree @0x6bf620;
// Find 0x16d190). Declared here so the ctor's lookup reloc-masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The bound CGameObject viewed by the ctor (m_10 == m_38). The slime reads the
// screen position (m_5c/m_posX), the layer key (m_74), the flags (m_08), the travel
// window (m_134..m_140) clamped from the raw target tile (m_164/m_168), the
// direction name (m_194+0x24), and re-seeds the rect (m_144..m_150). Only the
// touched offsets are modeled.
struct CSlimeCtorObj {
    char m_pad00[0x08];
    i32 m_08; // +0x08 flags
    char m_pad0c[0x5c - 0x0c];
    i32 m_5c; // +0x5c screen X
    i32 m_60; // +0x60 screen Y
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74 layer key
    char m_pad78[0x124 - 0x78];
    i32 m_124; // +0x124 travel direction (1..4)
    char m_pad128[0x134 - 0x128];
    i32 m_134; // +0x134 window min X
    i32 m_138; // +0x138 window min Y
    i32 m_13c; // +0x13c window max X
    i32 m_140; // +0x140 window max Y
    i32 m_144; // +0x144 rect base
    i32 m_148; // +0x148
    i32 m_14c; // +0x14c
    i32 m_150; // +0x150
    char m_pad154[0x164 - 0x154];
    i32 m_164; // +0x164 target tile X (raw -> scaled in place)
    i32 m_168; // +0x168 target tile Y
    char m_pad16c[0x194 - 0x16c];
    void* m_194; // +0x194 sprite/name record (dir name at +0x24)
    char m_pad198[0x1b4 - 0x198];
    i32 m_1b4; // +0x1b4 cycle-geometry id
};

// CKitchenSlime::CKitchenSlime @0x0b23a0 - fold the shared CUserLogic(obj) init,
// snap the bound object to the tile grid (m_posX/m_posY doubles + m_74 layer key +
// the m_tileX/m_tileY tile coords), scale the raw target tile (m_164/m_168) to pixels
// and compute the travel window (min/max of the start and target), match the
// slime's direction name (LEVEL_KITCHENSLIME_{NORTH,EAST,SOUTH,WEST}) into the
// direction id, then run LoadSprites for the first leg, bind the "A" bute node +
// cycle geometry, and clear the bound sprite's rect.
//
// @early-stop
// inline-strcmp + min/max-polarity + eh wall (docs/patterns/strcmp-eq-bool-local-setcc.md,
// zero-register-pinning.md, eh-ctor-vptr-restamp-position.md): body byte-faithful
// (the four unrolled inline-strcmp loops + CString temp EH, the min/max window
// clamp, the LoadSprites/bute/geometry tail). Residual is the strcmp result-reg
// alloc, the cmov-vs-branch min/max selection polarity, and the /GX leaf-vptr
// re-stamp position. Not source-steerable (global regalloc/EH numbering).
RVA(0x000b23a0, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 0x2000002;

    CSlimeCtorObj* o = (CSlimeCtorObj*)m_object;
    i32 snapX = (o->m_5c & ~0x1f) + 0x10;
    i32 snapY = (o->m_60 & ~0x1f) + 0x10;
    o->m_5c = snapX;
    m_posX = (double)snapX;
    o->m_60 = snapY;
    m_posY = (double)snapY;
    if (o->m_74 != 0x13) {
        o->m_74 = 0x13;
        o->m_08 |= 0x20000;
    }
    m_tileY = snapY;
    m_tileX = snapX;

    o->m_164 = (o->m_164 << 5) + 0x10;
    o->m_168 = (o->m_168 << 5) + 0x10;
    if (o->m_5c == o->m_164 && o->m_60 == o->m_168) {
        m_38->m_flags |= 0x10000;
        return;
    }
    o->m_134 = (o->m_5c < o->m_164) ? o->m_5c : o->m_164;
    o->m_13c = (o->m_5c <= o->m_164) ? o->m_164 : o->m_5c;
    o->m_138 = (o->m_60 >= o->m_168) ? o->m_168 : o->m_60;
    o->m_140 = (o->m_60 <= o->m_168) ? o->m_168 : o->m_60;

    CSlimeAnimPlayer* obj38 = Anim();
    if (obj38->m_194 != 0) {
        CString name;
        name = (char*)obj38->m_194 + 0x24;
        const char* s = (LPCTSTR)name;
        if (strcmp(s, "LEVEL_KITCHENSLIME_NORTH") == 0) {
            o->m_124 = 1;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_EAST") == 0) {
            o->m_124 = 2;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_SOUTH") == 0) {
            o->m_124 = 3;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_WEST") == 0) {
            o->m_124 = 4;
        }
    }

    m_stepMag = 0;
    m_stepMagHi = 0;
    if (LoadSprites() == 0) {
        m_38->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    o->m_144 = 0;
    o->m_14c = 0;
    o->m_148 = 0;
    o->m_150 = 0;
}

// ---------------------------------------------------------------------------
// The shared game-object type-name registry (R1, @0x6bf650) the level-object
// registration funnels through, keyed by the per-type id the global bute-tree
// (g_buteTree @0x6bf620) assigns to a class name. Same fast-range/slow-Find/
// rebuild lookup shape as the per-class activation table (KSlimeLookup); a fresh
// type-id is allocated by inserting the class name into the bute-tree, recording
// it into R1's entry (after freeing any CString nodes the slot held), and bumping
// the global type counter. All globals are BSS (DATA-pinned so the loads
// reloc-mask); the collection / CString helpers are external/no-body.
DATA(0x002bf658)
extern i32 g_typeLo;
DATA(0x002bf65c)
extern i32 g_typeHi;
DATA(0x002bf660)
extern char* g_typeBase;
DATA(0x002bf668)
extern i32 g_typeStride;
DATA(0x002bf664)
extern CTypeNameEntry* g_typeCur;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl;
DATA(0x002bf654)
extern CVariantSlot* g_typeColl2;
DATA(0x002bf66c)
extern void* g_typeNodes;

// The global type counter (0x61aea8). The class-name bute key is the shared
// "A" string literal (DAT_0060a454, the same $SG constant CLightFx.cpp uses).
DATA(0x0021aea8)
extern i32 g_typeCounter;

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 / Insert 0x16db90).
extern CButeTree g_buteTree;

// R1 lookup: the type-id -> R1 entry resolution shared with the per-class table.
static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(key, 0)) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl2->Set(&g_typeColl, (i32)item, 0xc);
    return g_typeCur;
}

// The slime's activation handler (LAB_0040180c, an ILT thunk). Referenced by
// address so the store emits a reloc-masked DIR32 to the named symbol.
extern "C" void KSlimeActivationHandler(); // 0x40180c

// CKitchenSlime::RegisterType @0x0b2aa0 - the level-load class registrar. Assign
// the slime class a type-id via the global bute-tree (registering its name on
// first use), record the name into the shared type-name table, then store the
// slime's activation handler (0x40180c) into the per-class activation table at
// that id. A static initializer (no `this`); same archetype as CProjectile's.
// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is pure
// regalloc + induction-variable coloring - retail pins the type-id in esi (mine
// edi), reads the node count into ebp via the `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]`
// count-down idiom (mine a plain --cnt), and orders the `id=key` store before the
// scratch=0. Not source-steerable (regalloc/strength-reduction wall); deferred.
RVA(0x000b2aa0, 0x18d)
void CKitchenSlime::RegisterType() {
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        g_buteTree.Insert("A", (void*)g_typeCounter);
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = TypeLookup(key);
        i32 cnt = g_typeCount;
        CStringNode* nodes = (CStringNode*)g_typeNodes;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->~CString();
                }
                nodes++;
            } while (--cnt);
        }
        slot->m_name = "A";
        g_typeCounter++;
    }
    *(void**)KSlimeLookup(id) = (void*)&KSlimeActivationHandler;
}

// CKitchenSlime::RegisterRange @0x0b28c0 - seed the slime's activation table's
// fast-range bounds via the shared zDArray registry ctor (RegisterRange(0x7d0,
// 0x7da), 0x408710 through the 0x3742 ILT thunk). A static initializer; same
// archetype as CProjectile::RegisterRange (0x0df920).
RVA(0x000b28c0, 0x15)
void CKitchenSlime::RegisterRange() {
    ((CZDArrayDerived*)&g_kslimeColl)->Construct(0x7d0, 0x7da);
}

// CKitchenSlime::FireActivation @0x0b2940 - look the activation coordinate up in
// the slime's per-coordinate registry; if the entry has a registered handler,
// look it up again and dispatch it __thiscall on this. Same archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150).
RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CKSlimeEntry* e = KSlimeLookup(coord);
    if (e->m_fn != 0) {
        CKSlimeEntry* e2 = KSlimeLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CKitchenSlime::Tick @0x0b2ca0 - the per-frame driver. Advances the anim
// sub-mgr, runs the on-screen visibility/scroll gate (unless the registry is in
// the no-scroll mode), and if the slime has reached its destination tile asks
// LoadSprites for the next leg; otherwise integrates the sub-pixel movement
// vector (m_dirX/m_dirY unit signs * the per-frame step) into m_posX/m_posY, snapping to
// the target tile on overshoot and writing the new grid position back to m_10.
// The integer scaffolding + visibility/already-arrived blocks are byte-exact.
// @early-stop
// x87 FP movement-integrator wall (docs/patterns/x87-fp-stack-schedule.md): the
// residual is a stack-slot swap (MSVC parks `step` at [esp+8] vs retail's
// [esp+0x10], swapping the per-iter temp) plus the dead x-clamp redundant-jump
// schedule. Logic byte-for-byte correct; ~95%, above the documented 60-75% range.
RVA(0x000b2ca0, 0x29c)
i32 CKitchenSlime::Tick() {
    ((CAniAdvanceCursor*)&Anim()->m_1a0)->Advance_15c360((i32)g_slimeTick);

    CGameRegistry* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_134 != 1) {
        CSlimeLevel* lvl = Level();
        i32 outX, outY;
        CSlimeEntity* ent = (CSlimeEntity*)((m4::GruntHitMgr*)reg->m_cmdGrid)
                                ->FindGruntAt(
                                    lvl->m_5c,
                                    lvl->m_60,
                                    (m4::HitTileRect*)&lvl->m_144,
                                    &outY,
                                    &outX,
                                    (struct tagRECT*)0
                                );
        if (ent && ent->m_258 != 0x38) {
            ((CTriggerMgr*)g_gameReg->m_cmdGrid)->CellDispatch(outY, outX, 5, -1);
        }
    }

    CSlimeLevel* lvl = Level();
    if (lvl->m_5c == m_tileX && lvl->m_60 == m_tileY && LoadSprites() == 0) {
        Anim()->m_8 |= 0x10000;
        return 0;
    }

    double step = (double)(i64)(u64)(u32)g_slimeFrameScale * m_speed;
    double* m88d = (double*)&m_stepMag;

    i32 newX;
    if (m_dirX > g_slimeZero) {
        double t = (m_posX = m_posX + step);
        newX = (i32)floor(t);
        i32 tx = m_tileX;
        *m88d = fabs(m_posX - (double)tx);
        // The X axis never clamps (unlike Y), but retail still emits the compare
        // (a min/max fold whose result equals the input); the empty-body test
        // reproduces the cmp + m_tileX stack-spill shared with the fabs.
        if (newX > tx) {
            newX = newX;
        }
    } else if (m_dirX < g_slimeZero) {
        double t = (m_posX = m_posX - step);
        newX = (i32)ceil(t);
        i32 tx = m_tileX;
        *m88d = fabs(m_posX - (double)tx);
        if (newX < tx) {
            newX = newX;
        }
    } else {
        newX = (i32)floor(m_posX);
    }

    i32 newY;
    if (m_dirY > g_slimeZero) {
        double t = (m_posY = m_posY + step);
        newY = (i32)floor(t);
        i32 ty = m_tileY;
        *m88d = fabs(m_posY - (double)ty);
        if (newY > ty) {
            Level()->m_5c = newX;
            Level()->m_60 = ty;
            return 0;
        }
    } else if (m_dirY < g_slimeZero) {
        double t = (m_posY = m_posY - step);
        newY = (i32)ceil(t);
        i32 ty = m_tileY;
        *m88d = fabs(m_posY - (double)ty);
        if (newY < ty) {
            Level()->m_5c = newX;
            Level()->m_60 = ty;
            return 0;
        }
    } else {
        newY = (i32)floor(m_posY);
    }

    Level()->m_5c = newX;
    Level()->m_60 = newY;
    return 0;
}

// The serialization stream is the shared CSerialArchive: slot +0x2c (index 11) Read
// reads n bytes into a buffer, slot +0x30 (index 12) Write transfers n bytes (was
// the per-TU CSlimeStream view; only the slot offsets are load-bearing, the virtual
// call is reloc-masked, as in CSBI_RectOnly::Serialize).

// The +0x34 serializable sub-object the slime chains into after the shared
// CUserLogic::SerializeChain is the shared CSerialObjRef (Chain @0x8c00 via the
// 0x1aff thunk); same archetype as CFortressFlag::Serialize.

// CKitchenSlime::Serialize @0x0b2ff0 - the slime's serialize override. For the
// read tag (7) read the seven motion quadwords (m_speed..m_88) through the stream's
// Read slot; for the transfer tag (4) transfer them through the Transfer slot.
// Then chain the shared CUserLogic serialize on `this` (bail on failure) and the
// +0x34 sub-object's chain, returning whether that chain succeeded.
// The seven 8-byte fields span the doubles m_speed..m_78 plus the (m_tileX,m_tileY) and
// (m_stepMag,m_stepMagHi) int pairs, so they are addressed by offset (codegen-neutral here).
RVA(0x000b2ff0, 0x11b)
i32 CKitchenSlime::Serialize(void* stream, i32 tag, i32 c, i32 d) {
    char* B = (char*)this;
    CSerialArchive* s = (CSerialArchive*)stream;
    // Written as `if (tag != 4) { if (tag == 7) Read... } else Transfer...` so
    // MSVC lays the tag-7 (Read) block physically first (cmp 4/je else; cmp 7/jne;
    // Read; jmp; else: Transfer) - the retail dispatch order.
    if (tag != 4) {
        if (tag == 7) {
            s->Read(B + 0x58, 8);
            s->Read(B + 0x60, 8);
            s->Read(B + 0x68, 8);
            s->Read(B + 0x70, 8);
            s->Read(B + 0x78, 8);
            s->Read(B + 0x80, 8);
            s->Read(B + 0x88, 8);
        }
    } else {
        s->Write(B + 0x58, 8);
        s->Write(B + 0x60, 8);
        s->Write(B + 0x68, 8);
        s->Write(B + 0x70, 8);
        s->Write(B + 0x78, 8);
        s->Write(B + 0x80, 8);
        s->Write(B + 0x88, 8);
    }
    if (SerializeChain(stream, tag, c, d) == 0) {
        return 0;
    }
    return ((CSerialObjRef*)(B + 0x34))->Chain((CSerialArchive*)stream, tag, c, (CSerialObj*)d)
           != 0;
}

// @early-stop
// Returns int (1 on success, 0 when no walkable tile was found) - the true
// signature, needed by Tick's `LoadSprites() == 0` test. Residual is the same FP
// /jump-table stack-frame schedule wall it has carried (retail reserves 0x1c vs
// our 0x14 - an extra direction-magnitude stack temp). ~69%, logic exact.
RVA(0x000b3160, 0x339)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = Level()->m_124;

    i32 tileX, tileY;
    i32 found = 0;
    for (i32 i = 0; i <= 4;) {
        CSlimeLevel* lvl = Level();
        i32 sw = lvl->m_124 - 1;
        switch (sw) {
            case 0:
                tileX = m_tileX;
                tileY = m_tileY - 0x20;
                break; // north
            case 1:
                tileX = m_tileX + 0x20;
                tileY = m_tileY;
                break; // east
            case 2:
                tileX = m_tileX;
                tileY = m_tileY + 0x20;
                break; // south
            case 3:
                tileX = m_tileX - 0x20;
                tileY = m_tileY;
                break; // west
        }

        i32 gx = tileX >> 5;
        i32 gy = tileY >> 5;
        i32 tileFlags;
        CTileGrid* map = g_gameReg->m_tileGrid;
        if ((u32)gx >= (u32)map->m_c || (u32)gy >= (u32)map->m_10) {
            tileFlags = 1;
        } else {
            tileFlags = ((i32*)map->m_8[gy])[gx * 7];
        }

        if (tileY >= lvl->m_138 && tileX <= lvl->m_13c && tileY <= lvl->m_140 && tileX >= lvl->m_134
            && !(tileFlags & 0x939) && !(tileFlags & 2)) {
            found = 1;
            break;
        }

        if (++i > 4) {
            return 0;
        }

        if (lvl->m_12c == 1) {
            lvl->m_124 = sw;
            if (Level()->m_124 <= 0) {
                Level()->m_124 = 4;
            }
        } else {
            lvl->m_124++;
            if (Level()->m_124 > 4) {
                Level()->m_124 = 1;
            }
        }
    }
    if (!found) {
        return 0;
    }

    m_posX = 0;
    m_posY = 0;
    i32 changed = (Level()->m_124 != savedDir);
    switch (Level()->m_124 - 1) {
        case 0: // north
            m_posY = -(double)*(i32*)&m_stepMag;
            m_dirX = 0;
            m_dirY = 0;
            *(i32*)&m_dirY = 0;
            *((i32*)&m_dirX + 1) = 0;
            *((i32*)&m_dirY + 1) = 0xbff00000;
            if (changed) {
                ((CGruntSprite*)Anim())->CacheFirstFrame("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case 1: // east
            *(i32*)&m_posX = m_stepMag;
            *((i32*)&m_posX + 1) = *((i32*)&m_stepMag + 1);
            m_dirX = 0;
            m_dirY = 0;
            *((i32*)&m_dirX + 1) = 0x3ff00000;
            *((i32*)&m_dirY + 1) = 0;
            if (changed) {
                ((CGruntSprite*)Anim())->CacheFirstFrame("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case 2: // south
            *(i32*)&m_posY = m_stepMag;
            *((i32*)&m_posY + 1) = *((i32*)&m_stepMag + 1);
            m_dirX = 0;
            m_dirY = 0;
            *((i32*)&m_dirY + 1) = 0x3ff00000;
            *((i32*)&m_dirX + 1) = 0;
            if (changed) {
                ((CGruntSprite*)Anim())->CacheFirstFrame("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case 3: // west
            m_posX = -(double)*(i32*)&m_stepMag;
            m_dirX = 0;
            m_dirY = 0;
            *((i32*)&m_dirX + 1) = 0xbff00000;
            *((i32*)&m_dirY + 1) = 0;
            if (changed) {
                ((CGruntSprite*)Anim())->CacheFirstFrame("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_posX = (double)Level()->m_5c + m_posX;
    m_posY = (double)Level()->m_60 + m_posY;

    u32 time;
    if (Level()->m_7c->m_bc != 0) {
        time = Level()->m_7c->m_bc;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_speed = g_slimeSpeedNum / (double)(i64)(u64)time;
    m_tileX = tileX;
    m_tileY = tileY;

    CSlimeAnimPlayer* player = Anim();
    CSprite* spr = player->m_194;
    if (changed != 0 && spr != 0) {
        if (spr->m_firstFrame <= 1 && spr->m_lastFrame >= 1) {
            player->m_190 = 1;
            player->m_198 = spr->m_frames.m_pData[1];
            m_stepMag = 0;
            m_stepMagHi = 0;
            return 1;
        }
        player->m_190 = 1;
        player->m_198 = 0;
        m_stepMag = 0;
        m_stepMagHi = 0;
        return 1;
    }
    m_stepMag = 0;
    m_stepMagHi = 0;
    return 1;
}
// size 0x90 from operator-new vtable attribution (gruntz.analysis.news)

SIZE_UNKNOWN(CKSlimeEntry);
SIZE_UNKNOWN(CSlimeAnimPlayer);
SIZE_UNKNOWN(CSlimeCtorObj);
SIZE_UNKNOWN(CSlimeCueGate);
SIZE_UNKNOWN(CSlimeEntity);
SIZE_UNKNOWN(CSlimeLevel);
SIZE_UNKNOWN(CSlimeSubMgr);
SIZE_UNKNOWN(CSlimeTiming);
SIZE_UNKNOWN(CSprite);
SIZE_UNKNOWN(CStringNode);
