#include <Mfc.h>
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/LeafCue.h>
#include <Gruntz/Grunt.h>
// Projectile.cpp - the CProjectile game-object (C:\Proj\Gruntz). Continues the
// CUserBase/CUserLogic/CMovingLogic hierarchy (see include/Gruntz/Projectile.h).
//
// CProjectile::CProjectile() (0x126e0) is the no-arg ctor: it folds the inline
// CMovingLogic init (the +0x38..+0x10c motion ints + the twelve default-bound
// doubles), constructs the +0x204 tracked-hit CObList, and stamps its vftable.
// Like the rest of the family it constructs a throwing CUserBaseLink (in the
// CUserLogic base) + a CObList, so MSVC emits the /GX EH frame -> built eh.
#include <Gruntz/Projectile.h>
#include <Gruntz/LightFx.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegistry.h>  // CGameRegistry singleton (pulls SoundCue.h + TileGrid.h)
#include <Gruntz/State.h>         // CState (reg->m_curState: the level-type descriptor)
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/StringNode.h>    // the shared type-name teardown slot (CStringNode::Free)
#include <Gruntz/ActColl.h>       // shared registry collection (CActColl/CVariantSlot Find/Insert/
                                  // RegisterRange + g_actCache/g_retAddrBreadcrumb/GetRetAddr)
#include <Bute/ButeMgr.h>         // CButeTree (the type-registry funnel)
#include <math.h>                 // sin / cos (StepMotion's parabola)
#include <string.h>               // memset (1-arg spawn ctor's +0x1e0 zero-fill)
#include <rva.h>
#include <Globals.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
// CTimeBomb's TU folds in below (ex TimeBomb.cpp, wave3-J).
#include <Gruntz/StatusBarUpdatersViews.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/HaznColl.h> // shared coordinate/activation-registry collection (CCoordColl)
#include <Gruntz/TimeBomb.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialObjRef.h>  // CSerialObjRef::Chain (0x8c00) on the +0x34 sub-object
#include <Gruntz/ActName.h>       // CActName (shared)
#include <Gruntz/ActReg.h>        // CLogicActTable::ResolveEntry (0xade60 dispatcher's real table)

// The +0x1a0 anim sub-object's setter/probe (CProjAnim::SetGeometry / Advance_15c360) and
// the render object's CGameObject-base name/sprite setters (CProjRenderObj::CacheFirstFrame /
// ApplyLookupGeometry) are folded onto the real classes (<Gruntz/Projectile.h>), reached
// directly; the former per-TU CDDrawBlitParam / CAniAdvanceCursor / CGruntSprite facet
// views are gone.

// StepMotion's two motion-phase thresholds (.rdata doubles) + the int amplitude
// global it folds into the trajectory (loaded as a double via fild). DATA pins so
// the fcomp/mov loads reloc-mask against the named symbols.
DATA(0x00245584)
extern i32 g_645584;

// ---------------------------------------------------------------------------
// Externs the reconstructed projectile methods reference (reloc-masked).
// ---------------------------------------------------------------------------
// The global node free-list the dtor / hit-scan recycle tracked-hit nodes onto.
DATA(0x00245544)
extern void* g_freeList;
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

// The draw-clock delta global fed to the render object's anim Tick on detach.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). The DATA pin
// reloc-masks the `mov ecx,ds:g_gameReg` load against the already-named symbol.
// The projectile sound/hit-scan/effects paths reach the canonical sub-objects
// through it: reg->m_world (CSpriteFactoryHolder) -> m_8 the HUD sprite factory
// (CSpriteFactory) + m_28 the sound-cue host (CSndHost, <Gruntz/SoundCue.h>);
// reg->m_tileGrid the terrain grid (CTileGrid, cell dword 0 = the terrain flags
// MovingSlot16 tests: water 0x900 / death 0x2 / gate 0x40); reg->m_curState
// the level-type descriptor (CState, +0x20 terrain-class id switch key).
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// A grunt in the hit-scan grid (g_gameReg->m_68 is a flat 15x15 cell table; each
// cell holds a grunt ptr). Only the offsets ScanTargets touches are modeled: the
// +0x10 owner (screen pos at +0x5c/+0x60), the +0x1fc live-projectile slot, the
// +0x170 alt-state gate, and the (+0x1ec,+0x1f0) spawn-cell key. The two hit
// handlers are out-of-line CGrunt methods (reloc-masked, reached via ILT thunks).
SIZE_UNKNOWN(CGruntOwner);
struct CGruntOwner {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
};
// A {x, y} cell-key node recycled through the global free-list (the same 2-int
// pair node BattlezMapConfig pulls). m_0 doubles as the free-list link.
SIZE_UNKNOWN(CHitKey);
struct CHitKey {
    i32 m_0;
    i32 m_4;
};

// The launch-sound lookup path is the canonical positional-cue subsystem
// (<Gruntz/SoundCue.h>, pulled by GameRegistry.h): reg->m_world->m_28 is the
// CSndHost, its embedded CSndFinder (m_10) Lookups the effect name to a LeafCue,
// whose CSoundCueMgr (m_10) GetItem (0x135d70) clones the DirectSound buffer the
// projectile owns as its CProjSample (m_sound).

// The shared default-bound doubles the CMovingLogic ctor copies into the twelve
// coordinate-bound members (retail .rdata 0x5f04b0 / 0x5f04b8). Defined here with
// DATA() pins so the ctor's dword loads reloc-mask against them.
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. Give CMovingLogic / CProjectile real vftables in
// this TU so the inline ctors emit their vptr stores. Bodies are not matched.
// (slot 16 Update is defined in MovingLogicUpdate.cpp, referenced externally.
// CProjectile's slot-17 anchor is the real LoadProjectileSprites body below -
// the old `ProjectileVfunc` placeholder is dissolved into it.)
// ---------------------------------------------------------------------------
CMovingLogic::~CMovingLogic() {}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): body, all
// field offsets, the double init and the CObList construction are byte-identical;
// the only residue is this ctor's OWN __ehfuncinfo (the EH prologue pushes
// funcinfo+0x0 vs retail funcinfo+0xe, and the CObList state id is `mov
// [esp+0x18],1` vs retail 2). NOT resolvable by completing the projectile TU:
// the funcinfo is per-function, and the out-of-line CMovingLogic ctor (0x13940)
// cannot be emitted from this TU (the no-arg ctor must keep CMovingLogic() inline
// to fold it; forcing the standalone drops this ctor 99%->19.7%; nothing here
// calls CMovingLogic out-of-line). See the CAVEAT in the pattern doc. ~99% wall.
RVA(0x000126e0, 0x1fc)
CProjectile::CProjectile() {}

// CTimeBomb::~CTimeBomb @0x012a70 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CKitchenSlime
// @0x013100 / the established leaf dtors; the empty body is enough for cl.
RVA(0x00012a70, 0x44)
CTimeBomb::~CTimeBomb() {}

// The 1-arg ctor's spawn constants (reloc-masked DIR32 loads).
extern "C" {
    DATA(0x00245588)
    u32 g_645588 = 0;
}
DATA(0x001eaa88)
const double g_5eaa88 = 0.0;
DATA(0x001f04e8)
u32 g_5f04e8 = 0;

// A post-init hook the spawn ctor fires (0x16ea90, no args; ecx unused).
extern void Fn16ea90();

// @confidence: med
// @source: rtti-vptr / disasm
// @early-stop
// The 1-arg spawn ctor CProjectile(owner), 0.63% stub -> ~55%. Reconstructed
// ADDITIVELY (new overloaded ctors CProjectile(owner)/CMovingLogic(owner); the
// byte-exact no-arg ctor 0x126e0 + its inline CMovingLogic are UNTOUCHED - verified
// still 99.05%). The bounds/SetCoords/CProjectile-body region is byte-shaped.
// DOMINANT wall (asm-level, llvm-objdump -dr base vs target): retail CALLS the
// CUserLogic(owner) base init OUT-OF-LINE (`call 0x58cd0`, ~5 B), but MSVC here
// INLINES the whole CUserLogic init (~160 B: CUserBase vptr, CUserBaseLink ctor,
// EngStr temp, RegisterLogicTypesOnce, AddLogicHit/Attack/Bump). Cause: the
// CUserLogic(owner) modeled in UserLogic.h is a SUBSET of retail's real init, so
// it fits MSVC's inline budget when folded into this large ctor, whereas retail's
// fuller init exceeds it and is emitted out-of-line. #pragma inline_depth(1) has
// no effect on MSVC5's mem-init base-ctor inlining. Forcing it out-of-line needs
// either the full CUserLogic(owner) body (a separate CUserLogic reconstruction,
// risks the matched tile-trigger leaves that inline it) or making +0x38 a member-
// with-ctor (would regress the banked 99% no-arg ctor - task-forbidden).
// Secondary residues: +0x38 Init (0x136d0) emits after the CMovingLogic vptr
// stamp vs retail's member-init position (EH state 0); the EH-state numbering
// (retail 0/1/2/3 over +0x38 + CObList) and the m_hitList-vs-body order differ.
RVA(0x000dec60, 0x255)
CProjectile::CProjectile(CGameObject* owner) : CMovingLogic(owner) {
    m_148 = 0;
    m_14c = 0;
    m_object->m_moveMode = 7;
    Fn16ea90();
    m_150 = (i32)owner;
    m_sprite = (CProjRenderObj*)owner;
    m_158 = (i32)owner->m_7c;
    m_sprite->m_08 |= 0x2000002;
    m_sprite->m_40 |= 1;
    if (m_object->m_latchedAnimId != 0xcf850) {
        m_object->m_latchedAnimId = 0xcf850;
        m_object->m_flags |= 0x20000;
    }
    memset(&m_frame1, 0, 0x1c); // zero the seven +0x1e0..+0x1fb sprite-frame slots
    m_sound = 0;
    m_shadow = 0;
}

// ---------------------------------------------------------------------------
// CProjectile::ReleaseDeferred (0x13c70) - fire the two queued one-shot callbacks
// (m_08 first, gated on the recorded hit-handle still matching m_28; then m_04
// unconditionally), reset the handle to its default 0x3e9, then run the slot-16
// virtual. The callbacks are raw __thiscall code pointers cached in the inherited
// CUserLogic m_04/m_08 ints; expressed as pointer-to-member-functions so the
// `mov ecx,this; call ptr` falls out (MSVC5 reserves the __thiscall keyword).
// ---------------------------------------------------------------------------
typedef void (CProjectile::*ProjCallback)();

RVA(0x00013c70, 0x47)
void CProjectile::ReleaseDeferred(i32) {
    if (m_04 != 0) {
        if (m_08 != 0 && (i32)m_objAux->m_1c == m_28) {
            (this->*(ProjCallback&)m_08)();
            m_08 = 0;
        }
        (this->*(ProjCallback&)m_04)();
        m_04 = 0;
        m_28 = 0x3e9;
    }
    MovingSlot16(); // virtual slot 16 (vtable offset 0x40) - CMovingLogic's one new virtual
}

// ---------------------------------------------------------------------------
// CProjectile::~CProjectile (0xdef60) - the most-derived dtor. Stop+rewind the
// launch sample, recycle each tracked-hit node back onto the global free-list,
// RemoveAll the list, then the compiler auto-destructs the CObList member and the
// CMovingLogic/CUserLogic/link base subobjects (the throwing link forces the /GX
// frame). Field names are placeholders; the offsets are load-bearing.
//
// @early-stop
// EH-state-numbering / base-dtor-inlining wall (docs/patterns/eh-state-numbering-base.md):
// the body is byte-identical through `m_hitList.RemoveAll()` (the m_sound stop, the
// free-list recycle walk, the AddTail/RemoveAll). The residue is the base-dtor
// tail: retail INLINES the whole CMovingLogic/CUserLogic/CUserBaseLink chain (vptr
// restamp 0x5e705c, ~EngStr on +0x18, vptr 0x5e70b4) and numbers the EH states
// 2/1/3 in a `sub esp,8` frame; our recompile emits an out-of-line `~CMovingLogic`
// call with states 1/0/-1 in a `push ecx` frame. Inlining the chain is not
// reachable from this TU without un-emitting the standalone base dtors (matched
// elsewhere) - the same per-function funcinfo wall the no-arg ctor hits. ~90%.
// ---------------------------------------------------------------------------
RVA(0x000def60, 0xbc)
CProjectile::~CProjectile() {
    if (m_sound != 0) {
        ((DirectSoundMgr*)m_sound)->StopAndRewind();
        m_sound = 0;
    }
    for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {
        CObject* data = m_hitList.GetNext(pos);
        if (data != 0) {
            // authentic: freelist recycle - bias the node back to its list-link header
            void** node = (void**)((char*)data - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_hitList.RemoveAll();
}

// ===========================================================================
// CProjectile::LoadProjectileSprites (0xdf050, /GX) - resolve the projectile's
// per-type sprite frames + launch trajectory at spawn. Snap the two grid
// endpoints to tile centres (m_targetX/m_targetY), record the target/owner ids, then
// switch on the projectile kind to pick the sprite-set base name + the
// "<Kind>ProjectileTimePerTile" bute value (m_timePerTile) and the arc flag (m_isArcing; Wingz
// also loops a launch sound + measures the tile distance). Look up the six frame
// sprites ("<base>1".."<base>5", "<base>IMPACT") + "<base>FALL", install the
// resolved frame-0 geometry, cache the object frame, compute the normalised
// launch velocity, spawn the LightFx shadow companion, and latch the "A" act key.
// (g_buteMgr is declared extern in <Gruntz/UserLogic.h>; BattlezMapConfig owns
// its DATA label, so the GetDwordDef call here reloc-masks against it. The global
// bute-tree g_buteTree @0x6bf620 is defined in the registration section below.)
// ===========================================================================
extern CButeTree g_buteTree;

// The shooter-grunt projectile kind LoadProjectileSprites dispatches on (kind);
// each name is confirmed by its case's GRUNTZ_<NAME>GRUNT_PROJECTILE sprite key +
// its "<Name>ProjectileTimePerTile" bute key. Same immediates as the bare labels
// -> naming is matching-neutral.
enum ProjectileKind {
    PROJ_BOOMERANG = 2, // GRUNTZ_BOOMERANGGRUNT
    PROJ_GUNHAT = 9,    // GRUNTZ_GUNHATGRUNT
    PROJ_NERFGUN = 10,  // GRUNTZ_NERFGUNGRUNT
    PROJ_ROCK = 11,     // GRUNTZ_ROCKGRUNT
    PROJ_WELDER = 21,   // GRUNTZ_WELDERGRUNT
    PROJ_WINGZ = 22,    // GRUNTZ_WINGZGRUNT
};

// @early-stop
// x87-scheduling + EH-frame-size wall (~58%, docs/patterns, same family as
// StepMotion ~70% and gx-scoped-local-eh-frame-size): the prologue, the tile-snap
// + id stores, the dense jump-table switch (kind 2/9/10/11/21/22 -> the six sprite
// bases, tail-merged arc-flag commons), the seven CString `base + "N"` concat +
// geometry-map Lookups, the frame-0 Setup and CacheFirstFrame are reproduced. Two
// residues: (1) the fxch-laden trajectory-normalisation block (0xdf4db..0xdf6ae:
// the sqrt / fdiv / fdivr chain, the two sign-of-component fcomp ladders and their
// interleaved qword stores) whose x87 stack ordering is not steerable from C; its
// double temps also miss the retail /GX frame by a dword (cl `sub esp,0x18` vs
// retail `sub esp,0x1c` - tried 0x18/0x1c/0x20 local combos, none lands 0x1c),
// which shifts EVERY front-half [esp+N] displacement by 4 (opcodes match, disp
// bytes differ). (2) engine-call/string relocs (operator+/Lookup/CreateSprite/
// GetDwordDef/CacheFirstFrame reached direct where retail uses ILT thunks). Logic
// complete; parked for the final sweep.
RVA(0x000df050, 0x6ba)
i32 CProjectile::LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1) {
    CString key;
    m_srcRow = a;
    m_targetX = (sx & ~0x1f) + 0x10;
    m_srcCol = b;
    m_kind = kind;
    m_targetY = (sy & ~0x1f) + 0x10;
    m_targetId = t0;
    m_ownerId = t1;

    CGameObject* owner = m_object;
    double dx = (double)(m_targetX - owner->m_screenX);
    double dy = (double)(m_targetY - owner->m_screenY);
    i32 count = 1;

    switch (kind) {
        case PROJ_ROCK: // ROCK
            key = "GRUNTZ_ROCKGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDwordDef("Projectile", "RockProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_GUNHAT: // GUNHAT
            key = "GRUNTZ_GUNHATGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "GunhatProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_BOOMERANG: // BOOMERANG
            key = "GRUNTZ_BOOMERANGGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "BoomerangProjectileTimePerTile", 0xbb8);
            m_isArcing = 0;
            break;
        case PROJ_NERFGUN: // NERFGUN
            key = "GRUNTZ_NERFGUNGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "NerfGunProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_WELDER: // WELDER
            key = "GRUNTZ_WELDERGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WelderProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_WINGZ: { // WINGZ
            key = "GRUNTZ_WINGZGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WingzProjectileTimePerTile", 0xbb8);
            LaunchSound("GRUNTZ_WINGZGRUNT_WINGZGRUNTLOOP");
            m_isArcing = 0;
            i32 ddx = (m_targetX >> 5) - (owner->m_screenX >> 5);
            if (ddx < 0) {
                ddx = -ddx;
            }
            i32 ddy = (m_targetY >> 5) - (owner->m_screenY >> 5);
            if (ddy < 0) {
                ddy = -ddy;
            }
            count = ddx;
            if (ddx <= ddy) {
                count = ddy;
            }
            break;
        }
        default:
            return 0;
    }

    // Resolve the six numbered frame sprites; frame "1" is required.
    CProjSpriteMap& map = m_sprite->m_c->m_2c->m_10;
    void* out;
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "1", (CObject*&)out);
    m_frame1 = out;
    if (m_frame1 == 0) {
        return 0;
    }
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "2", (CObject*&)out);
    m_frame2 = out;
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "3", (CObject*&)out);
    m_frame3 = out;
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "4", (CObject*&)out);
    m_frame4 = out;
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "5", (CObject*&)out);
    m_frame5 = out;
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "IMPACT", (CObject*&)out);
    m_impactSprite = out;
    out = 0;
    ((CMapStringToOb*)&map)->Lookup(key + "FALL", (CObject*&)out);
    m_fallSprite = out;

    m_savedFrameGeo = m_sprite->m_1b4;
    m_sprite->m_1a0.SetGeometry(m_frame1);
    m_sprite->CacheFirstFrame(key + "_OBJECT");

    // Normalise the launch trajectory into the per-frame velocity + sign vectors.
    u32 totalTime = (u32)(count * m_timePerTile);
    double len = sqrt(dx * dx + dy * dy);
    double t = (double)totalTime;
    double vx = dx / len;
    m_flightDist = len;
    m_velScale = len / t;
    m_posX = vx;
    m_posY = dy / len;
    m_velX = vx;
    m_velY = dy / len;
    // sign(dx): +0.5 / 0.0 / -0.5 stored as a double {lo=0, hi=+-0x3fe00000}
    m_roundXLo = 0;
    if (vx > 0.0) {
        m_roundXHi = 0x3fe00000;
    } else if (vx < 0.0) {
        m_roundXHi = (i32)0xbfe00000;
    } else {
        m_roundXHi = 0;
    }
    m_roundYLo = 0;
    if (dy > 0.0) {
        m_roundYHi = 0x3fe00000;
    } else if (dy < 0.0) {
        m_roundYHi = (i32)0xbfe00000;
    } else {
        m_roundYHi = 0;
    }
    m_flightDist = len < 0.0 ? -len : len;
    m_curX = owner->m_screenX;
    m_curY = owner->m_screenY;
    m_arrived = 0;

    // Spawn the LightFx shadow companion + activate its two frames.
    CSpriteFactory* factory = g_gameReg->m_world->m_8;
    m_shadow =
        (CProjRenderObj*)factory
            ->CreateSprite(0, owner->m_screenX, owner->m_screenY, 0xcf84f, "LightFx", 0x2040003);
    if (m_shadow != 0) {
        m_shadow->m_7c->Init(m_shadow);
        m_shadow->m_7c->m_18
            ->Activate((i32)(const char*)(key + "_SHADOW"), (i32)(const char*)(key + "1"), 5, 1);
    }

    // Latch the class act key ("A"): save the old registry node, then re-point it.
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    return 1;
}

// ===========================================================================
// CProjectile level-load registration (the same dual-registry archetype as
// CKitchenSlime::RegisterType): a per-coordinate activation table (R2, the
// projectile's own collection at @0x64c758) and the shared game-object type-name
// table (R1, @0x6bf650) keyed by the per-type id the global bute-tree assigns to
// the class name ("A"). All globals are BSS / DATA-pinned (reloc-masked); the
// collection / CString helpers are external/no-body.
// ===========================================================================

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 / Insert 0x16db90).
extern CButeTree g_buteTree;

// The activation-collection primitives are the shared CActColl/CVariantSlot
// (<Gruntz/ActColl.h>): Find 0x16da80, RegisterRange 0x3742-thunk (-> 0x408710),
// Insert 0x16d850, plus GetRetAddr 0x16d990 and the shared g_actCache (0x6bf464) /
// g_retAddrBreadcrumb scratch. The per-registry field globals below form the CActReg
// bodies (g_projTypeColl @0x6bf650, g_projActColl @0x64c758) around those coll objects.

// R1 - the shared type-name table (@0x6bf650).
DATA(0x002bf658)
extern i32 g_projTypeLo;
DATA(0x002bf65c)
extern i32 g_projTypeHi;
DATA(0x002bf660)
extern char* g_projTypeBase;
DATA(0x002bf668)
extern i32 g_projTypeStride;
DATA(0x002bf664)
extern CTypeNameEntry* g_projTypeCur;
DATA(0x002bf670)
extern i32 g_projTypeCount;
DATA(0x002bf650)
extern CActColl g_projTypeColl;
DATA(0x002bf654)
extern CVariantSlot* g_projTypeColl2;
DATA(0x002bf66c)
extern void* g_projTypeNodes;
DATA(0x0021aea8)
extern i32 g_projTypeCounter; // 0x61aea8 (global type counter)

// R2 - the projectile's per-coordinate activation table (@0x64c758).
struct CProjActEntry;
DATA(0x0024c758)
extern CActColl g_projActColl;

// The per-slot CString teardown node the type-name table walks is the shared
// CStringNode (<Gruntz/StringNode.h>: m_0 slot + Free 0x1b9b93 __thiscall); name
// assign into the resolved record is the real CString::operator= (0x1b9e74).

// The projectile's activation handler (LAB_00403896, an ILT thunk).
extern "C" void ProjActivationHandler(); // 0x403896

// R2 lookup (projectile activation table).
static inline CProjActEntry* ProjActLookup(i32 coord) {
    g_projActScratch = 0;
    if (coord >= g_projActLo && coord <= g_projActHi) {
        return (CProjActEntry*)(g_projActBase + (coord - g_projActLo) * g_projActStride);
    }
    if ((i32)((_zvec*)&g_projActColl)->GrowTo(coord, 0)) {
        return (CProjActEntry*)(g_projActBase + (coord - g_projActLo) * g_projActStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_projActColl2->Set(&g_projActColl, (i32)item, 0xc);
    return g_projActCur;
}

// R1 lookup (shared type-name table).
static inline CTypeNameEntry* ProjTypeLookup(i32 key) {
    g_projTypeCount = 0;
    if (key >= g_projTypeLo && key <= g_projTypeHi) {
        return (CTypeNameEntry*)(g_projTypeBase + (key - g_projTypeLo) * g_projTypeStride);
    }
    if ((i32)((_zvec*)&g_projTypeColl)->GrowTo(key, 0)) {
        return (CTypeNameEntry*)(g_projTypeBase + (key - g_projTypeLo) * g_projTypeStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_projTypeColl2->Set(&g_projTypeColl, (i32)item, 0xc);
    return g_projTypeCur;
}

// CProjectile::RegisterRange @0x0df920 - seed the projectile's activation table's
// fast-range bounds (RegisterRange(0x7d0, 0x7da)). A static initializer.
RVA(0x000df920, 0x15)
void CProjectile::RegisterRange() {
    ((CZDArrayDerived*)&g_projActColl)->Construct(0x7d0, 0x7da);
}

// The projectile activation entry's leading slot is a __thiscall handler run on the
// projectile (CProjectile is single-inheritance -> a 4-byte code-pointer PMF).
struct CProjActEntry {
    i32 (CProjectile::*m_fn)();
};
SIZE_UNKNOWN(CProjActEntry);

// CProjectile::RunAct @0x0df9a0 - the class's vtable slot-4 (UserLogicVfunc2) body:
// resolve the activation entry for `coord` (R2 lookup, inlined twice); if a handler
// is bound, re-resolve and invoke it __thiscall on this, else return the entry
// pointer. Same archetype as CProjActDispatcher::Dispatch / CPathHazard::RunAct.
RVA(0x000df9a0, 0x102)
i32 CProjectile::RunAct(i32 coord) {
    CProjActEntry* e = ProjActLookup(coord);
    if (e->m_fn != 0) {
        return (this->*(ProjActLookup(coord)->m_fn))();
    }
    return (i32)e;
}

// CProjectile::RegisterType @0x0dfb00 - the level-load class registrar (same
// archetype as CKitchenSlime::RegisterType): assign the class a type-id via the
// global bute-tree, record the name into the shared type-name table, then store
// the projectile's activation handler (0x403896) into the per-class table.
// @early-stop
// ~91%: byte-correct operations/offsets/strings/calls; the residual is the same
// regalloc + count-down induction wall RegisterType carries (type-id register
// coloring + the `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]` node-free loop idiom). Not
// source-steerable; deferred to the final sweep.
RVA(0x000dfb00, 0x18d)
void CProjectile::RegisterType() {
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        g_buteTree.Insert("A", (void*)g_projTypeCounter);
        i32 key = g_projTypeCounter;
        id = key;
        CTypeNameEntry* slot = ProjTypeLookup(key);
        i32 cnt = g_projTypeCount;
        CStringNode* nodes = (CStringNode*)g_projTypeNodes;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->~CString();
                }
                nodes++;
            } while (--cnt);
        }
        slot->m_name = "A";
        g_projTypeCounter++;
    }
    *(void**)ProjActLookup(id) = (void*)&ProjActivationHandler;
}

// ===========================================================================
// CProjectile::MovingSlot16 (0xdfd00) - per-frame trajectory advance +
// impact-effect select. Runs each frame until the projectile reaches its target
// tile (m_curX/m_curY catch up to m_targetX/m_targetY): integrate the parabola into the
// render position (m_posX/m_posY), clamp the muzzle-tracked position against the
// target, and for the arc kinds (m_isArcing) select one of five impact-effect sprite
// tiers by the fractional distance-to-target. On arrival it stops the loop
// sound, runs a final hit-scan, then - by the destination terrain flags - spills
// the water / level death splash effect or installs the IMPACT/FALL sprite. The
// one-shot m_arrived latch (0 while in flight; set on arrival) gates re-entry.
// (WINGZ, kind 0x16, additionally loops its flight sound while over the level.)
//
// @early-stop
// x87-scheduling + EH-frame family wall (same as LoadProjectileSprites ~58% /
// StepMotion ~70%; docs/patterns): the control flow, the WINGZ sound gate, the
// reached-destination hit-scan + terrain-flag switch (water/death/tier), the
// LEVEL_DEATHSPLASH/GAME_WATER spawns and the five distance-tier sprite installs
// are reconstructed. The residue is the dense fxch/fcompp FP schedule of the
// parabola integration (0xdfeb6..0xdff37) and the distance/sqrt tier ladder
// (0xdffd7..0xe00e9) - MSVC5 keeps `dist` live on the x87 stack across the eight
// tier comparisons and pre-computes m_flightDist*0.9 interleaved with the fsqrt, which
// is not steerable from C source - plus the effect-spawn regalloc (ecx vs edx for
// reg->m_world) and the unnamed engine-call relocs. Logic complete; parked.
// ===========================================================================
RVA(0x000dfd00, 0x6f5)
void CProjectile::MovingSlot16() {
    if (m_arrived != 0) {
        return;
    }

    if (m_kind == 0x16) { // WINGZ: loop the flight sound while over the level
        CGameObject* owner = m_object;
        CGameRegistry* reg = g_gameReg;
        if (owner->m_screenX < reg->m_viewOriginR && owner->m_screenX >= reg->m_viewOriginL
            && owner->m_screenY < reg->m_viewOriginB && owner->m_screenY >= reg->m_viewOriginT) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_sound != 0) {
            ((DirectSoundMgr*)m_sound)->StopAndRewind();
            m_sound = 0;
        }
    }

    if (m_curX != m_targetX || m_curY != m_targetY) {
        // -- in flight: integrate one frame + select the impact tier ----------
        if (m_kind == 0x16) {
            ScanTargets(0);
        }
        m_posX = m_posX + (double)(u32)g_645584 * m_velX * m_velScale;
        m_posY = m_posY + (double)(u32)g_645584 * m_velY * m_velScale;
        i32 xRes = (i32)(*(double*)&m_roundXLo + m_posX);
        i32 yRes = (i32)(*(double*)&m_roundYLo + m_posY);
        i32 localX = xRes;
        if (m_velX > 0.0) {
            if (xRes > m_targetX) {
                localX = m_targetX;
                xRes = m_targetX;
            }
        } else if (m_velX < 0.0) {
            if (xRes < m_targetX) {
                localX = m_targetX;
                xRes = m_targetX;
            }
        }
        if (m_velY > 0.0) {
            if (yRes > m_targetY) {
                yRes = m_targetY;
            }
        } else if (m_velY < 0.0) {
            if (yRes < m_targetY) {
                yRes = m_targetY;
            }
        }
        m_curX = xRes;
        m_curY = yRes;
        i32 offX = 0;
        i32 offY = 0;
        if (m_isArcing != 0) {
            double dx = fabs((double)m_targetX - m_posX);
            double dy = fabs((double)m_targetY - m_posY);
            double dist = sqrt(dx * dx + dy * dy);
            double mag = m_flightDist;
            if (dist >= mag * 0.9 || dist < mag * 0.1) {
                offX = 0x4;
                offY = -0x4;
                if (m_sprite->m_1b4 != (i32)m_frame1) {
                    m_savedFrameGeo = m_sprite->m_1b4;
                    m_sprite->m_1a0.SetGeometry(m_frame1);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.SetGeometry(m_frame1);
                    }
                }
            } else if (dist >= mag * 0.8 || dist < mag * 0.2) {
                offX = 0x8;
                offY = -0x8;
                if (m_sprite->m_1b4 != (i32)m_frame2) {
                    m_savedFrameGeo = m_sprite->m_1b4;
                    m_sprite->m_1a0.SetGeometry(m_frame2);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.SetGeometry(m_frame2);
                    }
                }
            } else if (dist >= mag * 0.7 || dist < mag * 0.3) {
                offX = 0xc;
                offY = -0xc;
                if (m_sprite->m_1b4 != (i32)m_frame3) {
                    m_savedFrameGeo = m_sprite->m_1b4;
                    m_sprite->m_1a0.SetGeometry(m_frame3);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.SetGeometry(m_frame3);
                    }
                }
            } else if (dist >= mag * 0.6 || dist < mag * 0.4) {
                offX = 0x10;
                offY = -0x10;
                if (m_sprite->m_1b4 != (i32)m_frame4) {
                    m_savedFrameGeo = m_sprite->m_1b4;
                    m_sprite->m_1a0.SetGeometry(m_frame4);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.SetGeometry(m_frame4);
                    }
                }
            } else {
                offX = 0x14;
                offY = -0x14;
                if (m_sprite->m_1b4 != (i32)m_frame5) {
                    m_savedFrameGeo = m_sprite->m_1b4;
                    m_sprite->m_1a0.SetGeometry(m_frame5);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.SetGeometry(m_frame5);
                    }
                }
            }
        }
        m_object->m_screenX = offX + m_curX;
        m_object->m_screenY = offY + m_curY;
        if (m_shadow != 0) {
            m_shadow->m_5c = localX;
            m_shadow->m_60 = yRes;
        }
        return;
    }

    // -- arrived at the target tile ------------------------------------------
    if (m_sound != 0) {
        ((DirectSoundMgr*)m_sound)->StopAndRewind();
        m_sound = 0;
    }
    ScanTargets(0);
    if (m_shadow != 0) {
        m_shadow->m_08 |= 0x10000;
        m_shadow = 0;
    }
    m_arrived = 1;
    i32 tier = 0;
    if (m_kind != 0x16) {
        CGameRegistry* reg = g_gameReg;
        CTileGrid* plane = reg->m_tileGrid;
        i32 tileX = m_targetX >> 5;
        i32 tileY = m_targetY >> 5;
        u32 flags;
        if ((u32)tileX >= (u32)plane->m_c || (u32)tileY >= (u32)plane->m_10) {
            flags = 1;
        } else {
            flags = plane->m_8[tileY][tileX * 7]; // cell dword 0 = terrain flags
        }
        if (flags & 0x900) {
            // water tile: spill a splash then hide the projectile
            if (m_targetX < reg->m_viewOriginR && m_targetX >= reg->m_viewOriginL
                && m_targetY < reg->m_viewOriginB && m_targetY >= reg->m_viewOriginT) {
                CGameObject* fx =
                    reg->m_world->m_8
                        ->CreateSprite(0, m_targetX, m_targetY, 0xcf84f, "Particlez", 0x40003);
                if (fx != 0) {
                    fx->ApplyName("GAME_WATER");
                    fx->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
            m_sprite->m_08 |= 0x10000;
            return;
        }
        if (flags & 0x2) {
            if (flags & 0x40) {
                tier = 1;
            } else {
                switch (reg->m_curState->m_levelType) {
                    case 4:
                    case 5:
                    case 8:
                        tier = 1;
                        break;
                    case 6:
                        break;
                    default:
                        // level death tile: spill the death-splash then hide
                        if (m_targetX < reg->m_viewOriginR && m_targetX >= reg->m_viewOriginL
                            && m_targetY < reg->m_viewOriginB && m_targetY >= reg->m_viewOriginT) {
                            CGameObject* fx = reg->m_world->m_8->CreateSprite(
                                0,
                                m_targetX,
                                m_targetY,
                                0xcf84f,
                                "Particlez",
                                0x40003
                            );
                            if (fx != 0) {
                                fx->ApplyName("LEVEL_DEATHSPLASH");
                                fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                            }
                        }
                        m_sprite->m_08 |= 0x10000;
                        return;
                }
            }
        }
    }
    void* sprite = (tier != 0) ? m_fallSprite : m_impactSprite;
    if (sprite == 0) {
        m_sprite->m_08 |= 0x10000;
        return;
    }
    m_savedFrameGeo = m_sprite->m_1b4;
    m_sprite->m_1a0.SetGeometry(sprite);
}

// ---------------------------------------------------------------------------
// CProjectile::DetachRenderObj (0xe05e0) - clear the render object's bit-0 flag,
// re-target its animation to the current draw-delta, and (when the object is in
// the "active but un-anchored" state) raise its hide bit. Returns 0.
// ---------------------------------------------------------------------------
RVA(0x000e05e0, 0x4e)
i32 CProjectile::DetachRenderObj() {
    m_sprite->m_40 &= ~1u;
    // m_1a0 is a CAniAdvanceCursor (the CProjAnim view is a reduced facet of it);
    // bind Advance_15c360 to the real ?..@CAniAdvanceCursor@@QAEHI@Z (0x15c360).
    ((CAniAdvanceCursor*)&m_sprite->m_1a0)->Advance_15c360(g_6bf3bc);
    CProjRenderObj* r = m_sprite;
    if (r->m_1c8 != 0 && r->m_1c0 == 0) {
        r->m_08 |= 0x10000;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CProjectile::StepMotion (0xe08b0) - advance the projectile one frame. On the
// launch frame it snaps the render objects to the muzzle (m_targetX/m_targetY); once past
// the second phase threshold it expires (scan for the terminal impact, raise the
// hide bit on the shadow + owner). Otherwise it integrates the sin/cos parabola
// into the render position and rounds it into the render objects' screen coords.
//
// @early-stop
// x87-scheduling wall: the sin/cos parabola block (0xe0929..0xe09e1) is a dense
// fxch-laden FP stack schedule MSVC5 emits from the trajectory expression; the
// control flow, the muzzle-snap, the expire path and all __ftol rounds match, but
// the FP body's stack ordering is not steerable from C source. ~70% plateau.
// ---------------------------------------------------------------------------
RVA(0x000e08b0, 0x1de)
void CProjectile::StepMotion() {
    i32 impact = 0;
    if (m_launched == 0) {
        if (m_phase > g_projPhase0) {
            // launch frame: snap render objects to the muzzle, mark launched.
            m_object->m_screenX = m_targetX;
            m_object->m_screenY = m_targetY;
            if (m_shadow != 0) {
                m_shadow->m_5c = m_targetX;
                m_shadow->m_60 = m_targetY;
            }
            m_launched = 1;
            goto step;
        }
    } else if (m_phase > g_projPhase1) {
        // past the terminal threshold: deliver the impact scan + expire.
        ScanTargets(1);
        if (m_shadow != 0) {
            m_shadow->m_08 |= 0x10000;
            m_shadow = 0;
        }
        m_sprite->m_08 |= 0x10000;
        return;
    }
step:
    ScanTargets(impact);
    // integrate the sin/cos parabola into the render position.
    double s = sin(m_phase);
    double c = cos(m_phase);
    double amp = (double)g_645584;
    double vx = -m_dirX;
    double vy = m_dirY;
    double px = m_originX + vy * m_velScale * s - vx * amp * c + m_phase;
    double py = m_originY + vy * amp * c + vx * m_velScale * s;
    m_posX = px;
    m_posY = py;
    m_phase = px;
    m_object->m_screenX = (i32)m_posX;
    m_object->m_screenY = (i32)m_posY;
    if (m_shadow != 0) {
        m_shadow->m_5c = (i32)m_posX;
        m_shadow->m_60 = (i32)m_posY;
    }
}

// ---------------------------------------------------------------------------
// CProjectile::ScanTargets (0xe0b10) - sweep the 15x15 grunt grid centered on the
// projectile, AABB-test each grunt's 14x14 footprint against the projectile's
// 0x20x0x20 box, and for each fresh overlap (not self, not already tracked) record
// the grunt's spawn-cell key in the tracked-hit list and deliver the hit. The
// self cell, when `impact` is set, triggers the grunt's self-impact handler.
//
// @early-stop
// Logic byte-exact end to end (the grid scan, the AABB test, the GetNext list
// walk, the free-list pull/AddTail, the deliver/self-impact branches, the void
// epilogue). Residue is a register-coloring difference in the bbox slot pair
// (projXhi/projYlo land in swapped [esp+0x20]/[esp+0x24] vs retail) and the
// 8-arg DeliverHit push temps (edx/eax/ecx vs retail eax/ecx/edx) - same values,
// same push order, different temp registers - plus the three unnamed engine-call
// relocs (AddTail/DeliverHit/SelfImpact at instantiation-specific RVAs not yet in
// symbol_names.csv). ~94%, not steerable from C source.
// ---------------------------------------------------------------------------
RVA(0x000e0b10, 0x1bd)
void CProjectile::ScanTargets(i32 impact) {
    i32 tileY = 0;                            // [esp+0x10]  outer (row) counter
    i32 projXlo = m_object->m_screenX - 0x10; // [esp+0x1c]  m_10 = owner CGameObject
    i32 projYlo = m_object->m_screenY - 0x10; // [esp+0x20]
    i32 projXhi = projXlo + 0x20;             // [esp+0x24]
    i32 projYhi = projYlo + 0x20;             // [esp+0x28]
    i32 rowBase = 0x1c;                       // [esp+0x18]  row byte stride base
    i32 colOff;                               // [esp+0x14]
    i32 col;                                  // ebp
    do {
        col = 0;
        colOff = rowBase;
        for (; col < 0xf; col++, colOff += 4) {
            // authentic: sliding-window grid access - 0x1c row-stride overlaps the
            // 4-byte column pitch, so it is raw byte arithmetic, not a 2D pointer array.
            CGrunt* g = *(CGrunt**)((char*)g_gameReg->m_cmdGrid + colOff);
            if (g == 0) {
                continue;
            }
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            i32 gx = g->m_10->m_5c - 7;
            i32 gy = g->m_10->m_60 - 7;
            i32 gxhi = gx + 0xe;
            i32 gyhi = gy + 0xe;
            if (projXlo > gxhi) {
                continue;
            }
            if (projXhi < gx) {
                continue;
            }
            if (projYlo > gyhi) {
                continue;
            }
            if (projYhi < gy) {
                continue;
            }
            if (m_srcRow == tileY && m_srcCol == col) {
                // self cell
                if (impact != 0 && g->m_entranceCommitted != 0 && g->m_entranceReason == 0) {
                    g->LoadGruntTypeTable(2, 1, 0, 0);
                }
                return;
            }
            // already-tracked? walk the hit list for this grunt's cell key.
            i32 keyX = g->m_tileOwnerHi;
            i32 keyY = g->m_tileOwnerLo;
            for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {
                // authentic: MFC CObList stores raw hit-key nodes; GetNext returns CObject*
                CHitKey* k = (CHitKey*)m_hitList.GetNext(pos);
                if (k->m_0 == keyX && k->m_4 == keyY) {
                    return;
                }
            }
            // fresh hit: pull a key node off the free-list, record + deliver.
            CHitKey* slot = 0;
            CHitKey* p = (CHitKey*)g_freeList; // authentic: freelist head is void*
            if (p->m_0 != 0) {
                slot =
                    (CHitKey*)&p->m_4; // authentic: node payload overlays +4 (past the link word)
                slot->m_0 = keyX;
                slot->m_4 = keyY;
                g_freeList = (void*)p->m_0;
            }
            m_hitList.AddTail((CObject*)slot); // authentic: MFC AddTail takes CObject*
            g->StepCombatReaction(m_kind, 1, m_srcRow, m_srcCol, m_targetId, m_ownerId, 1, 0);
        }
        rowBase += 0x3c;
        tileY++;
    } while (rowBase < 0x10c);
}

// ===========================================================================
// CTimeBomb (ex TimeBomb.cpp, merged wave3-J): the 0x0dec60-0x0e2213 interval is
// ONE original TU - the text is a P-T-P sandwich (projectile x10 | timebomb x6 |
// projectile @0xe2190 LaunchSound), and the private initialized-.data extents
// are contiguous (projectile 0x213624..0x213838, timebomb 0x213860..0x2138b4).
// NOTE the shared type-name registry cells (0x2bf650-0x2bf670 / 0x21aea8 /
// 0x6bf464) keep BOTH view-name sets for now (g_projType* above, g_nameReg*/
// g_nextActId below) - unifying that family tree-wide is deferred work.
// ===========================================================================

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CTimeBomb::FireActivation (0x0e1830)
// dispatches through - the SAME archetype as CKitchenSlime::FireActivation
// (0x0b2940, src/Gruntz/KitchenSlime.cpp), but CTimeBomb's OWN registry instance
// at 0x64c780. A coordinate maps to an Entry* either directly (when within the
// fast [g_tbombLo, g_tbombHi] range) via g_tbombBase + (coord-lo)*stride, or by a
// slow Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (GetRetAddr 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and yields
// g_tbombCur. The entry's first dword is a fn-ptr; a nonzero entry's handler is
// called __thiscall on `this`. All globals are unnamed BSS (DATA-pinned so the
// loads reloc-mask); the collection methods are external/no-body (the SAME shared
// engine functions both registries call). The alloc-cache pair (g_actCache
// 0x6bf464 / g_retAddrBreadcrumb 0x6bf428) is the SAME shared global both registries
// write (already named by KitchenSlime.cpp - re-declared here, address-pinned).
struct CTBombEntry;        // an entry: first dword is the registered handler
extern void* GetRetAddr(); // 0x16d990

DATA(0x0024c780)
extern CCoordColl g_tbombColl;

// ConstructTBombRange @0x0e17b0 - the static initializer that builds g_tbombColl's fast
// [0x7d0, 0x7da] id range (CZDArrayDerived::Construct). Re-homed from
// src/Stub/BoundaryLowerThunks.cpp (was RegRangee17b0).
RVA(0x000e17b0, 0x15)
void ConstructTBombRange() {
    ((CZDArrayDerived*)&g_tbombColl)->Construct(0x7d0, 0x7da);
}
// g_projActCache (0x2bf464) + g_retAddrBreadcrumb come from <Gruntz/ActColl.h>.

// The entry's first dword is a pointer-to-member-function of CTimeBomb (single
// inheritance -> 4-byte code pointer); FireActivation invokes it on `this`,
// emitting `mov ecx,this; call [entry]`. CTimeBomb is defined COMPLETE in the
// header above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CTimeBomb::*TBombHandler)();
struct CTBombEntry {
    TBombHandler m_fn; // [entry]
};
SIZE_UNKNOWN(CTBombEntry);

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CTBombEntry* TBombLookup(i32 coord) {
    g_tbombScratch = 0;
    if (coord >= g_tbombLo && coord <= g_tbombHi) {
        return (CTBombEntry*)(g_tbombBase + (coord - g_tbombLo) * g_tbombStride);
    }
    if ((i32)((_zvec*)&g_tbombColl)->GrowTo(coord, 0)) {
        return (CTBombEntry*)(g_tbombBase + (coord - g_tbombLo) * g_tbombStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_tbombColl2->Set(&g_tbombColl, (i32)item, 0xc);
    return g_tbombCur;
}

// ---------------------------------------------------------------------------
// The shared activation-NAME registry CTimeBomb::RegisterActs (0x0e1990) interns
// the key "A" into g_buteTree (Find returns the id, 0 == absent); on a fresh id it
// records the key in the shared scratch name registry (@0x6bf650, the SAME range/
// cache shape as g_tbombColl) and bumps g_nextActId. Then it resolves id->Entry in
// CTimeBomb's OWN registry (g_tbombColl via TBombLookup, the SAME instance
// FireActivation uses) and stores the logic handler (the ILT to LoadAttributes
// @0x0e1e60). g_buteTree (0x6bf620, named by mangled symbol) doubles as the
// name->id map; g_nextActId (0x61aea8) is the running id counter; s_actKeyA
// (0x60a454) is the "A" key. The id->name-slot resolve reuses the shared
// Find/GetRetAddr/Insert + g_actCache/g_retAddrBreadcrumb collection methods.
// ---------------------------------------------------------------------------
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CCoordColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CVariantSlot* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo;
DATA(0x002bf65c)
extern i32 g_nameRegHi;
DATA(0x002bf660)
extern char* g_nameRegBase;
DATA(0x002bf668)
extern i32 g_nameRegStride;
DATA(0x002bf664)
extern char* g_nameRegCur; // slow-path result slot
DATA(0x002bf66c)
extern void** g_nameRegCurList; // the slot's CString list base
DATA(0x002bf670)
extern i32 g_nameRegScratch; // zeroed first; doubles as the list count

// The shared bute store the key is interned in (?g_buteTree@@3VCButeTree@@A
// @0x6bf620, pulled via UserLogic.h; named by mangled symbol so Find/Insert
// reloc-mask).
extern CButeTree g_buteTree;

// The id->name-slot resolve (fast range path + slow Find/GetRetAddr/Insert rebuild).
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if ((i32)((_zvec*)&g_nameReg)->GrowTo(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_nameReg2->Set(&g_nameReg, (i32)item, 0xc);
    return g_nameRegCur;
}

// The logic handler bound into the registry slot (the ILT to CTimeBomb's
// attribute-loader @0x0e1e60); referenced by address so the DIR32 operand
// reloc-masks.
extern i32 TBombLogic_e1e60();

extern CButeMgr g_buteMgr;

// The running game clock g_645588 is DEFINED above (the spawn-ctor constants).

// The bound game object is the inherited CUserLogic m_10/m_38 (both point at the
// same CGameObject); the ctor reads/writes it directly (+0x08 flag word, +0x5c/
// +0x60 screen pos, +0x74 z gate, +0x120 per-tile-time gate, +0x124, +0x1b4
// active-anim descriptor, and ApplyName/ApplyLookupGeometry) - all modeled on
// CGameObject (<Gruntz/UserLogic.h>). Re-reads m_10/m_38 per access, matching retail.

// The collision grid the ctor marks / the per-frame step reads is
// g_gameReg->m_tileGrid, already typed CTileGrid* on the canonical CGameRegistry:
// an 0x1c-byte cell grid (m_8[row] -> cell-row base; cols 0x1c B apart) bounded by
// m_c x m_10 - no local grid view.
// The registry's tile-manager (g_gameReg->m_68, a reused void* slot): the per-frame
// detonate path posts the bomb's tile event to it via NotifyMoveAt (thunk 0x2fb3 ->
// 0x7b330, 4 args). Modeled NO-body so the call reloc-masks.
SIZE_UNKNOWN(TBombTileMgr);
// (g_gameReg is declared/DATA-pinned in the projectile preamble above.)

// CTimeBomb::FireActivation @0x0e1830 - look the activation coordinate up in the
// timebomb's per-coordinate registry; if the entry has a registered handler, look
// it up again and dispatch it __thiscall on this. Same archetype as
// CKitchenSlime::FireActivation (0x0b2940).
RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    CTBombEntry* e = TBombLookup(coord);
    if (e->m_fn != 0) {
        CTBombEntry* e2 = TBombLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CTimeBomb::RegisterActs @0x0e1990 - bind the per-frame logic handler to the
// activation key "A" in the timebomb's OWN registry (g_tbombColl). See the
// registration commentary above. The SAME archetype as CParticlez::RegisterActs /
// RegisterSimpleAnimLogic.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail (the slot-vs-id callee-saved choice cascading into the free-loop
// count). Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    *(void**)TBombLookup(id) = (void*)&TBombLogic_e1e60;
}

// ---------------------------------------------------------------------------
// CTimeBomb::CTimeBomb(CGameObject*) @0xe1b90 - the 1-arg leaf ctor: the standard
// CUserLogic(obj) init (folded inline) plus the timebomb tail - raise the bound
// object's logic/collision bits, set its z gate, apply the GAME_TIMEBOMB sprite,
// cache the anim-set node off the "A" bute key, snapshot m_38->m_1b4, then pick
// the FAST (per-tile-time>0) or SLOW (bute TimeBombSlowTime) geometry + running
// clock, mark the collision-grid cell at the bound object's tile, and arm the
// bound object's per-tile-time gate (-1). Constructs a throwing CUserBaseLink, so
// MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical to retail (the CUserLogic init, the two flag RMWs, the
// ApplyName/anim-cache, the FAST/SLOW branch, the >>5 grid-cell mark, the m_124
// arm); the residue is this ctor's own __ehfuncinfo state numbering + the 1-slot
// callee-saved scheduling delta MSVC coin-flips. The SAME plateau as
// CBrickz::CBrickz / CStaticHazard::CStaticHazard; not source-steerable. Parked
// for the final sweep.
RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 0x2000002;
    if (m_object->m_latchedAnimId != 0xf) {
        m_object->m_latchedAnimId = 0xf;
        m_object->m_flags |= 0x20000;
    }
    m_38->ApplyName("GAME_TIMEBOMB");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_prevAnimNode = m_38->m_geoId;
    if (m_object->m_120 > 0) {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_durationLo = m_object->m_120;
        m_durationHi = 0;
        m_startTimeLo = g_645588;
        m_startTimeHi = 0;
        m_fastPhase = 1;
    } else {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBSLOW", 0);
        m_durationLo = (i32)g_buteMgr.GetDwordDef("Projectile", "TimeBombSlowTime", 0xfa0);
        m_durationHi = 0;
        m_startTimeLo = g_645588;
        m_startTimeHi = 0;
        m_fastPhase = 0;
    }
    i32 cx = m_object->m_screenX >> 5;
    i32 cy = m_object->m_screenY >> 5;
    CTileGrid* g = g_gameReg->m_tileGrid;
    if (cx < g->m_c && cy < g->m_10) {
        char* row = (char*)g->m_8[cy];
        *(i32*)(row + cx * 0x1c) |= 0x1000000;
    }
    m_object->m_124 = -1;
}

// The +0x1a0 animation sub-mgr the per-frame step advances each draw-delta
// (Advance 0x15c360, __thiscall ret 4) - the SAME engine sink CTeleporter::Begin
// drives. The draw-delta mirror (g_6bf3bc) is consumed by the advance.
SIZE_UNKNOWN(TBombAnimSink);
// (g_6bf3bc is declared/DATA-pinned in the projectile preamble above.)

// The collision-grid cell lookup the per-frame step folds in three times: the
// initial state read (out-of-bounds reads as 1) and the two detonate-path
// clears. Same grid shape the ctor marks; the bounds compares are UNSIGNED.
static inline i32 TBombGridCell(CGameObject* obj) {
    CTileGrid* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if ((u32)cx < (u32)g->m_c && (u32)cy < (u32)g->m_10) {
        char* row = (char*)g->m_8[cy];
        return *(i32*)(row + cx * 0x1c);
    }
    return 1;
}
static inline void TBombGridClear(CGameObject* obj) {
    CTileGrid* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if ((u32)cx < (u32)g->m_c && (u32)cy < (u32)g->m_10) {
        char* row = (char*)g->m_8[cy];
        *(i32*)(row + cx * 0x1c) &= ~0x1000000;
    }
}

// CTimeBomb::LoadAttributes @0x0e1e60 - the bomb's per-frame logic step. Read the
// bomb's tile state; if it sits under a blocking/hazard cell, raise m_38's pending
// flag, clear the cell's owner bit, and bail. Otherwise advance the anim sink and
// gate on the 64-bit running-clock timer (m_startTimeLo/m_startTimeHi base vs m_durationLo/m_durationHi duration);
// once it expires, either re-arm the FAST phase (m_fastPhase==0 -> apply GAME_TIMEBOMBFAST,
// reload the Projectile/TimeBombFastTime duration) or, on the second expiry
// (m_fastPhase!=0), detonate: raise m_38's flag, clear the cell, and post the tile event
// to the registry's tile-manager (NotifyMoveAt). Returns 0 on every path.
//
// @early-stop
// regalloc/scheduling wall (~90.5%, docs/patterns/zero-register-pinning.md +
// reread-member-view-pointer.md): the body is byte-faithful - the grid lookup,
// the `(cell&0x939)||(cell&2)` split test, the 64-bit clock-elapsed compare, the
// FAST re-arm bute read, and both detonate blocks all match. The residue is the
// coin-flip that pins the bound object (m_object) in eax vs ecx, which cascades into
// the screen-coord load order at the three inlined grid sites; plus the i64
// jl/jg branch-emission order and the NotifyMoveAt arg-push schedule. Not
// source-steerable (cy-first reordering regressed it). Parked for the final sweep.
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::LoadAttributes() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & 0x939) || (cell & 2)) {
        m_38->m_flags |= 0x10000;
        TBombGridClear(m_object);
        return 0;
    }
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    if ((i64)g_645588 - *(i64*)&m_startTimeLo < *(i64*)&m_durationLo) {
        return 0;
    }
    if (m_fastPhase == 0) {
        m_prevAnimNode = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_durationLo = (i32)g_buteMgr.GetDwordDef("Projectile", "TimeBombFastTime", 0x3e8);
        m_durationHi = 0;
        m_startTimeLo = (i32)g_645588;
        m_startTimeHi = 0;
        m_fastPhase = 1;
        return 0;
    }
    m_38->m_flags |= 0x10000;
    TBombGridClear(m_object);
    ((EngineLabelBacklog*)g_gameReg->m_cmdGrid)
        ->LoadExplosionSprites(m_object->m_screenX, m_object->m_screenY, m_object->m_124, 1);
    return 0;
}

// CTimeBomb::SerializeMove @0xe2080 - vtable slot 1. Bail unless the resource
// manager is loaded (g_gameReg->m_world); round-trip the 64-bit phase-start clock
// (m_startTime) + the phase duration (m_duration) + the fast/slow phase flag
// (m_fastPhase) through the archive stream (mode 4 = Write @+0x30, mode 7 = Read
// @+0x2c), then chain the shared CUserLogic serialize helper (SerializeChain,
// 0x16e7f0) and the +0x34 CSerialObjRef sub-object's Chain (0x8c00). Same two-chain
// archetype as CGruntPuddle::Serialize.
// @early-stop
// regalloc/hoist wall (~79%, docs/patterns/zero-register-pinning.md): logic is
// byte-correct (the m_world gate, the m_58/m_60/m_54 round-trip, the SerializeChain
// + CSerialObjRef Chain tail). Residue: retail pins `this` in ebx and hoists
// `lea edi,[this+0x58]` above the mode branches (reusing edi via `add edi,8` for the
// consecutive 8-byte fields), where cl keeps `this` in edi and recomputes each
// field address - a callee-saved-register coloring choice not steerable from C.
RVA(0x000e2080, 0xc1)
i32 CTimeBomb::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    CSerialArchive* sa = (CSerialArchive*)arc;
    if (mode == 4) {
        sa->Write(&m_startTimeLo, 8);
        sa->Write(&m_durationLo, 8);
    } else if (mode == 7) {
        sa->Read(&m_startTimeLo, 8);
        sa->Read(&m_durationLo, 8);
    }
    if (mode == 4) {
        sa->Write(&m_fastPhase, 4);
    } else if (mode == 7) {
        sa->Read(&m_fastPhase, 4);
    }
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)arc), mode, a3, a4)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain(sa, mode, a3, (CSerialObj*)a4) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// CProjectile::LaunchSound (0xe2190) - lazily create + play the launch sound the
// first time. Look the effect up in the game-registry sound map by name, clone a
// sample off the matched entry, store it at m_sound, and start it on the configured
// channel. Returns 1 on success, 0 if already launched / any lookup gate fails.
//
// @early-stop
// Code bytes byte-exact vs retail (verified by full llvm-objdump compare); the
// residue is purely the reloc-naming artifact (docs/patterns, objdiff-reloc-scoring
// memory): the four engine callees - CMapStringToOb::Lookup (0x1b8438), the sample
// factory GetItem (0x135d70) and CSample Play/StopAndRewind (0x136300) - are not
// yet named in symbol_names.csv, so their REL32 relocs stay fuzzy against the
// target's FUN_ names. g_gameReg IS named (CGameRegistry). Flips to exact once those
// engine functions get RVA-annotated stubs. ~44% scoring artifact, logic complete.
// ---------------------------------------------------------------------------
RVA(0x000e2190, 0x83)
i32 CProjectile::LaunchSound(const char* key) {
    if (m_sound != 0) {
        return 0;
    }
    CGameRegistry* reg = g_gameReg;
    if (reg->m_soundEnabled == 0) {
        return 0;
    }
    LeafCue* entry = 0;
    ((CMapStringToOb*)&reg->m_world->m_28->m_10)->Lookup(key, (CObject*&)entry);
    if (entry == 0) {
        return 0;
    }
    if (entry->m_10 == 0) {
        return 0;
    }
    // GetItem returns the pooled DirectSound buffer (CStatusBarItem2 in the cue-mgr
    // view); the projectile owns the same buffer as its CProjSample sound sample.
    m_sound = (CProjSample*)entry->m_10->GetItem();
    if (m_sound == 0) {
        return 0;
    }
    ((DirectSoundMgr*)m_sound)->ApplyAndPlay(g_gameReg->m_inputFlag, 0, 0, 1);
    return 1;
}

// ===========================================================================
// 0x0ade60 - per-coordinate action dispatch over the per-logic-class dispatch
// table g_logicActReg_646010 (@0x246010, a CLogicActTable - NOT the projectile's
// own g_projActColl @0x24c758; that was a lookup-table conflation). Resolves the
// activation entry for `coord` (ResolveEntry, inlined twice); if the entry's
// leading handler slot is non-null, re-resolves and invokes it __thiscall on this
// dispatcher object. Sits in the MenuSparkle/logic leaf-init COMDAT pool
// (LogicActReg646010.cpp), not the projectile bodies.
extern CLogicActTable g_logicActReg_646010; // 0x246010 (bound in LogicActReg646010.cpp)

SIZE_UNKNOWN(CProjActDispatcher);
class CProjActDispatcher {
public:
    void Dispatch(i32 coord); // 0x0ade60
};

// The entry's leading slot is a __thiscall handler taking this dispatcher; MSVC5
// rejects the __thiscall keyword, so model it as a single-inheritance member
// pointer (a bare 4-byte code address) reinterpreted from the entry word.
typedef void (CProjActDispatcher::*ProjActHandler)();

RVA(0x000ade60, 0x102)
void CProjActDispatcher::Dispatch(i32 coord) {
    char* e = g_logicActReg_646010.ResolveEntry(coord);
    if (*(void**)e != 0) {
        char* e2 = g_logicActReg_646010.ResolveEntry(coord);
        ProjActHandler h = *(ProjActHandler*)e2;
        (this->*h)();
    }
}
