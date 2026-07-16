#include <Mfc.h>           // real MFC CString (direction-name match temp; reloc-masked)
#include <Wap32/zBitVec.h> // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype (g_kslimeColl)
#include <Bute/ButeTree.h>
#include <rva.h>
#include <math.h>   // floor (0x120580) / ceil (0x120480) / fabs (inline d9 e1)
#include <string.h> // inline strcmp for the ctor's direction-name match
#include <Bute/ButeMgr.h>
#include <Gruntz/StringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h> // CUserLogic base (CKitchenSlime : CUserLogic) + CGameObject::ApplyName (0x150540)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (0x15c360) - the +0x1a0 sub-object
#include <Gruntz/Sprite.h>           // CSprite (frame-data value; the looked-up direction sprite)
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

// The slime's bound object (Level() == m_object, Anim() == m_38) IS the canonical
// CGameObject; the ex CSlimeLevel / CSlimeAnimPlayer / CSlimeTiming views are
// dissolved onto it (KitchenSlime.h). The level state reads m_screenX/m_screenY
// (+0x5c/+0x60), m_124 (travel dir), m_12c (lock-dir), m_extentL..m_extentB
// (+0x134..+0x140, the on-screen tile window), m_areaL (+0x144, the cue-gate rect
// base), and m_7c->m_bc (the AnimWorkerObj per-tile-time override) - all cast-free.
// The anim facet reaches the leaf-embedded +0x1a0 CAniAdvanceCursor (Advance
// @0x15c360) by documented address and the +0x190.. frame cache through the
// m_194/m_198 role-union CGameObject already models (frame-cache reinterpret).

// The level tile map reached via g_gameReg->m_tileGrid is the canonical CTileGrid
// (<Gruntz/TileGrid.h>): m_c/m_10 = grid extents, m_8 = the row table
// (row[gy][gx*7] is the tile-flags word).
// The on-screen object reached as g_gameReg->m_cmdGrid (the +0x68 CTriggerMgr slot,
// the visibility/cue gate): FindGruntAt @0x75c60 resolves the entity under the slime's
// screen rect and CellDispatch posts a scroll - both real CTriggerMgr methods, called
// cast-free (<Gruntz/TriggerMgr.h>).
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (FindGruntAt @0x75c60, CellDispatch)

// The canonical CGameRegistry view of the singleton; m_posY (cue gate) and m_dirX
// (tile map) are void*/CTileGrid* here, cast locally at the deref sites.
extern "C" CGameRegistry* g_gameReg;

// The entity FindGruntAt returns IS a CGrunt (retail signature
// ?FindGruntAt@CTriggerMgr@@QAEPAVCGrunt@@..., returns CGrunt*; the header still
// types it CTmCell* pending the deferred cross-lane retype, so cast at the site).
// Its +0x258 is CGrunt::m_gruntKind (the object-kind id); 0x38 is the slime's own
// kind, so its own footprint is skipped. The ex CSlimeEntity view is dissolved onto
// the canonical CGrunt (<Gruntz/Grunt.h>), exactly as SpotLightCtor does.
#include <Gruntz/Grunt.h>

// 32.0 (the per-tile-time -> per-frame-speed reciprocal numerator).

// Per-frame scroll/scale factor (.data int) Tick multiplies into m_speed to get the
// per-frame pixel step.
// g_slimeFrameScale was a SECOND NAME for g_frameDelta (0x245584 per-frame delta) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" u32 g_frameDelta;

// 0.0 (the velocity-sign comparand for the movement integrator).
DATA(0x001ea400)
const double g_slimeZero = 0.0;

// A frame/tick counter (BSS) the anim sub-mgr Advance consumes.
// g_slimeTick was a SECOND NAME for g_engineFrameDelta (0x2bf3bc per-frame delta (wwd)) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" u32 g_engineFrameDelta;

// CKitchenSlime : CUserLogic is modeled in <Gruntz/KitchenSlime.h> (canonical
// header, included below). The CUserLogic base gives the +0x18 destructible link,
// so ~CKitchenSlime folds the shared teardown (the /GX leaf-dtor archetype, see
// UserLogic.cpp 0x10ab0). The slime's level/anim views are dissolved onto CGameObject.
#include <Gruntz/KitchenSlime.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CKitchenSlime::FireActivation
// (0x0b2940) dispatches through - the SAME archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150, see UserLogic.cpp), but a
// DIFFERENT registry instance (the slime's, at 0x646228 vs the trigger's at
// 0x244688). A coordinate maps to an Entry* either directly (when within the
// fast [g_kslimeLo,g_kslimeHi] range) via g_kslimeBase + (coord-lo)*stride, or
// by a slow Find in the collection (0x16da80, __thiscall ret 8), which on miss
// rebuilds (GetRetAddr 0x16d990 -> g_projActCache, Insert 0x16d850 __thiscall ret
// 0xc) and yields g_kslimeCur. The entry's first dword is a fn-ptr; a nonzero
// entry's handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (shared with the trigger registry's engine functions). The
// alloc-cache pair (g_projActCache 0x6bf464 / g_retAddrBreadcrumb 0x6bf428) is the
// SAME shared global both registries write.

// SHREDDED-OBJECT FIX: g_kslimeColl is ONE 0x24-byte CActReg object, not eight globals -
// 0x64622c..0x646248 are its INTERIOR FIELDS (m_coll2/m_lo/m_hi/m_base/m_cur/m_stride/
// m_scratch), which this TU used to declare as seven separate DATA()-pinned scalars, and
// the hand-inlined KSlimeLookup over them was CActReg::ResolveEntry spelled out.
// DEFINED here (owner TU) as zero-init .bss with NO ctor - what retail has: the CRT
// dynamic-init table (30 entries @0x2096e4) has no initializer for 0x646228, and the
// address is past .data's raw extent, so the loader zero-fills it. Construct (0x408710)
// ctors it in place at runtime; hence the (_zvec*) cast at the engine call sites.
DATA(0x00246228)
CActReg g_kslimeColl;

// The entry record (KSlimeHandler/CKSlimeEntry, the PMF slot) is defined in
// <Gruntz/KitchenSlime.h> after the complete class.

// The coordinate->Entry* lookup FireActivation folds in twice: the shared archetype
// inline, typed to this registry's entry.
static inline CKSlimeEntry* KSlimeLookup(i32 coord) {
    return (CKSlimeEntry*)g_kslimeColl.ResolveEntry(coord);
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

// The bound object the ctor reads IS the canonical CGameObject (m_object == m_38):
// the screen position m_screenX/m_screenY (+0x5c/+0x60), the layer key
// m_latchedAnimId (+0x74), the flags m_flags (+0x08), the on-screen travel window
// m_extentL..m_extentB (+0x134..+0x140) clamped from the raw target tile m_164/m_168,
// the direction name at m_194+0x24, and the re-seeded rect m_areaL..m_areaB
// (+0x144..+0x150) all read cast-free (the ex CSlimeCtorObj view is dissolved).

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

    CGameObject* o = m_object;
    i32 snapX = (o->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (o->m_screenY & ~0x1f) + 0x10;
    o->m_screenX = snapX;
    m_posX = (double)snapX;
    o->m_screenY = snapY;
    m_posY = (double)snapY;
    if (o->m_latchedAnimId != 0x13) {
        o->m_latchedAnimId = 0x13;
        o->m_flags |= 0x20000;
    }
    m_tileY = snapY;
    m_tileX = snapX;

    o->m_164 = (o->m_164 << 5) + 0x10;
    o->m_168 = (o->m_168 << 5) + 0x10;
    if (o->m_screenX == o->m_164 && o->m_screenY == o->m_168) {
        m_38->m_flags |= 0x10000;
        return;
    }
    o->m_extentL = (o->m_screenX < o->m_164) ? o->m_screenX : o->m_164;
    o->m_extentR = (o->m_screenX <= o->m_164) ? o->m_164 : o->m_screenX;
    o->m_extentT = (o->m_screenY >= o->m_168) ? o->m_168 : o->m_screenY;
    o->m_extentB = (o->m_screenY <= o->m_168) ? o->m_168 : o->m_screenY;

    CGameObject* obj38 = Anim();
    if (obj38->m_194 != 0) {
        CString name;
        name = obj38->m_194 + 0x24;
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
    m_savedGeoId = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    o->m_areaL = 0;
    o->m_areaR = 0;
    o->m_areaT = 0;
    o->m_areaB = 0;
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

// The global type counter (0x61aea8; retail .data init = 2000). The class-name bute
// key is the shared "A" string literal (DAT_0060a454, the same $SG constant
// CLightFx.cpp uses).
DATA(0x0021aea8)
i32 g_typeCounter = 2000;

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 / Insert 0x16db90).

// R1 lookup: the type-id -> R1 entry resolution shared with the per-class table.
static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return (CTypeNameEntry*)(g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride);
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(key, 0)) {
        return (CTypeNameEntry*)(g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, (i32)item, 0xc);
    return (CTypeNameEntry*)g_typeColl.m_spare; // m_spare is the i32-typed slow-path slot
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
        i32 cnt = g_typeColl.m_grown;
        CStringNode* nodes = (CStringNode*)g_typeColl.m_alloc;
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
    m_38->m_1a0.Advance((i32)g_engineFrameDelta);

    CGameRegistry* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_134 != 1) {
        CGameObject* lvl = Level();
        i32 outX, outY;
        CGrunt* ent = (CGrunt*)reg->m_cmdGrid->FindGruntAt(
            lvl->m_screenX,
            lvl->m_screenY,
            (RECT*)&lvl->m_areaL,
            &outY,
            &outX,
            (RECT*)0
        );
        if (ent && ent->m_gruntKind != 0x38) {
            ((CTriggerMgr*)g_gameReg->m_cmdGrid)->CellDispatch(outY, outX, 5, -1);
        }
    }

    CGameObject* lvl = Level();
    if (lvl->m_screenX == m_tileX && lvl->m_screenY == m_tileY && LoadSprites() == 0) {
        m_38->m_flags |= 0x10000;
        return 0;
    }

    double step = (double)(i64)(u64)(u32)g_frameDelta * m_speed;
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
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else if (m_dirY < g_slimeZero) {
        double t = (m_posY = m_posY - step);
        newY = (i32)ceil(t);
        i32 ty = m_tileY;
        *m88d = fabs(m_posY - (double)ty);
        if (newY < ty) {
            Level()->m_screenX = newX;
            Level()->m_screenY = ty;
            return 0;
        }
    } else {
        newY = (i32)floor(m_posY);
    }

    Level()->m_screenX = newX;
    Level()->m_screenY = newY;
    return 0;
}

// The serialization stream is the shared CSerialArchive: slot +0x2c (index 11) Read
// reads n bytes into a buffer, slot +0x30 (index 12) Write transfers n bytes (was
// the per-TU CSlimeStream view; only the slot offsets are load-bearing, the virtual
// call is reloc-masked, as in CStatusBarMgr::Serialize).

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
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(stream), tag, c, d) == 0) {
        return 0;
    }
    return ((CSerialObjRef*)(B + 0x34))->Chain((CSerialArchive*)stream, tag, c, (CGameObject*)d)
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
        CGameObject* lvl = Level();
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

        if (tileY >= lvl->m_extentT && tileX <= lvl->m_extentR && tileY <= lvl->m_extentB
            && tileX >= lvl->m_extentL && !(tileFlags & 0x939) && !(tileFlags & 2)) {
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
                Anim()->ApplyName("LEVEL_KITCHENSLIME_NORTH");
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
                Anim()->ApplyName("LEVEL_KITCHENSLIME_EAST");
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
                Anim()->ApplyName("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case 3: // west
            m_posX = -(double)*(i32*)&m_stepMag;
            m_dirX = 0;
            m_dirY = 0;
            *((i32*)&m_dirX + 1) = 0xbff00000;
            *((i32*)&m_dirY + 1) = 0;
            if (changed) {
                Anim()->ApplyName("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_posX = (double)Level()->m_screenX + m_posX;
    m_posY = (double)Level()->m_screenY + m_posY;

    u32 time;
    if (Level()->m_7c->m_bc != 0) {
        time = Level()->m_7c->m_bc;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_speed = g_slimeSpeedNum / (double)(i64)(u64)time;
    m_tileX = tileX;
    m_tileY = tileY;

    // The direction sprite + first-frame cache is the CGameObject frame-cache
    // role-union: +0x194 (m_194) is the cached CSprite*, +0x198 (m_layer) doubles as
    // the first-frame pointer - the same reinterpret CGameObject's own ApplyName does
    // (authentic union access, cast at the site).
    CGameObject* player = Anim();
    CSprite* spr = (CSprite*)player->m_194;
    if (changed != 0 && spr != 0) {
        if (spr->m_firstFrame <= 1 && spr->m_lastFrame >= 1) {
            player->m_190 = 1;
            *(i32**)&player->m_layer = spr->m_frames.m_pData[1];
            m_stepMag = 0;
            m_stepMagHi = 0;
            return 1;
        }
        player->m_190 = 1;
        *(i32**)&player->m_layer = 0;
        m_stepMag = 0;
        m_stepMagHi = 0;
        return 1;
    }
    m_stepMag = 0;
    m_stepMagHi = 0;
    return 1;
}
// size 0x90 from operator-new vtable attribution (gruntz.analysis.news)

// (CKSlimeEntry SIZE_UNKNOWN lives in KitchenSlime.h; CSlimeAnimPlayer/CSlimeLevel/
//  CSlimeTiming/CSlimeCtorObj are dissolved onto CGameObject/AnimWorkerObj.)
// (CSlimeEntity is dissolved onto the canonical CGrunt - FindGruntAt's real return.)
SIZE_UNKNOWN(CSprite);
SIZE_UNKNOWN(CStringNode);
