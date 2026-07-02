// Projectile.cpp - the CProjectile game-object (C:\Proj\Gruntz). Continues the
// CUserBase/CUserLogic/CMovingLogic hierarchy (see include/Gruntz/Projectile.h).
//
// CProjectile::CProjectile() (0x126e0) is the no-arg ctor: it folds the inline
// CMovingLogic init (the +0x38..+0x10c motion ints + the twelve default-bound
// doubles), constructs the +0x204 tracked-hit CObList, and stamps its vftable.
// Like the rest of the family it constructs a throwing CUserBaseLink (in the
// CUserLogic base) + a CObList, so MSVC emits the /GX EH frame -> built eh.
#include <Gruntz/Projectile.h>
#include <Gruntz/CGameRegistry.h>
#include <Bute/ButeMgr.h> // CButeTree (the type-registry funnel)
#include <math.h>         // sin / cos (StepMotion's parabola)
#include <string.h>       // memset (1-arg spawn ctor's +0x1e0 zero-fill)
#include <rva.h>
#include <Globals.h>

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

// The draw-clock delta global passed to the render object's SetAnim on detach.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A). Modeled here with
// the offsets the projectile sound/hit-scan paths touch; the DATA pin reloc-masks
// the `mov ecx,ds:g_gameReg` load against the already-named symbol.
struct CProjSoundEntry; // map value: the per-effect sound table entry
struct CProjSoundInner; // reg->m_30->m_28: holds the CMapStringToOb at +0x10
struct CProjSpriteFactory {
    // The HUD sprite factory (reg->m_30->m_8); CreateSprite @0x1597b0, __thiscall.
    CProjRenderObj*
    CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
};
struct CProjSoundCat { // reg->m_30: the sound-category object
    char m_pad00[0x8];
    CProjSpriteFactory* m_8; // +0x8  the HUD sprite factory (LightFx shadow)
    char m_pad0c[0x28 - 0xc];
    CProjSoundInner* m_28; // +0x28  -> the lookup map lives at (*m_28)+0x10
};
// The level "type" descriptor (reg->m_2c); LoadProjectileEffects switches on its
// +0x20 terrain-class id to pick the level death effect.
struct CProjLevelInfo {
    char m_pad00[0x20];
    i32 m_20; // +0x20  terrain-class id (switch key)
};
// The level terrain plane (reg->m_70): a width x height grid of 28-byte tiles
// reached row-major through the +0x8 row-pointer array; tile dword 0 is the
// terrain flags LoadProjectileEffects tests (water 0x900 / death 0x2 / gate 0x40).
struct CTerrainTile {
    u32 m_0; // +0x0  terrain flags
    char m_pad04[0x1c - 0x4];
};
struct CTerrainPlane {
    char m_pad00[0x8];
    CTerrainTile** m_8; // +0x8  row pointers
    i32 m_c;            // +0xc  width (tiles)
    i32 m_10;           // +0x10 height (tiles)
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// A grunt in the hit-scan grid (g_gameReg->m_68 is a flat 15x15 cell table; each
// cell holds a grunt ptr). Only the offsets ScanTargets touches are modeled: the
// +0x10 owner (screen pos at +0x5c/+0x60), the +0x1fc live-projectile slot, the
// +0x170 alt-state gate, and the (+0x1ec,+0x1f0) spawn-cell key. The two hit
// handlers are out-of-line CGrunt methods (reloc-masked, reached via ILT thunks).
struct CGruntOwner {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
};
struct CGruntTarget {
    void DeliverHit(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h); // 0x4646b0 (ret 0x20)
    void SelfImpact(i32 a, i32 b, i32 c, i32 d);                             // 0x4dd50  (ret 0x10)
    char m_pad00[0x10];
    CGruntOwner* m_10; // +0x10
    char m_pad14[0x170 - 0x14];
    i32 m_170; // +0x170
    char m_pad174[0x1ec - 0x174];
    i32 m_1ec, m_1f0; // +0x1ec/+0x1f0  spawn-cell key
    char m_pad1f4[0x1fc - 0x1f4];
    i32 m_1fc; // +0x1fc  live-projectile slot
};

// A {x, y} cell-key node recycled through the global free-list (the same 2-int
// pair node BattlezMapConfig pulls). m_0 doubles as the free-list link.
struct CHitKey {
    i32 m_0;
    i32 m_4;
};

// The map value the launch-sound lookup returns: its +0x10 sub-object owns the
// sample factory (GetItem 0x135d70, __thiscall) the projectile clones from.
struct CProjSampleFactory {
    CProjSample* GetItem(); // 0x135d70 (__thiscall, 0 args; returns a new sample)
};
struct CProjSoundEntry {
    char m_pad00[0x10];
    CProjSampleFactory* m_10; // +0x10
};
// The CMapStringToOb the category exposes (its Lookup is the engine method
// 0x1b8438). Embedded inside reg->m_30->m_28 at +0x10.
struct CProjSoundMap {
    i32 Lookup(const char* key, CProjSoundEntry** out); // 0x1b8438 (ret 8)
};
struct CProjSoundInner {
    char m_pad00[0x10];
    CProjSoundMap m_10; // +0x10  the lookup map
};

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
// ---------------------------------------------------------------------------
CMovingLogic::~CMovingLogic() {}
i32 CMovingLogic::MovingLogicVfunc() {
    return 0;
}
i32 CMovingLogic::MovingLogicVfunc2() {
    return 0;
}
i32 CMovingLogic::MovingLogicVfunc3() {
    return 0;
}

i32 CProjectile::ProjectileVfunc() {
    return 0;
}

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
// (retail 0/1/2/3 over +0x38 + CObList) and the m_204-vs-body order differ.
RVA(0x000dec60, 0x255)
CProjectile::CProjectile(CGameObject* owner) : CMovingLogic(owner) {
    m_148 = 0;
    m_14c = 0;
    *(i32*)((char*)m_10 + 0xe4) = 7;
    Fn16ea90();
    m_150 = (i32)owner;
    m_154 = (CProjRenderObj*)owner;
    m_158 = (i32)owner->m_7c;
    m_154->m_08 |= 0x2000002;
    m_154->m_40 |= 1;
    if (*(i32*)((char*)m_10 + 0x74) != 0xcf850) {
        *(i32*)((char*)m_10 + 0x74) = 0xcf850;
        m_10->m_08 |= 0x20000;
    }
    memset((char*)this + 0x1e0, 0, 0x1c);
    m_200 = 0;
    m_1fc = 0;
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
        if (m_08 != 0 && (i32)m_14->m_1c == m_28) {
            (this->*(ProjCallback&)m_08)();
            m_08 = 0;
        }
        (this->*(ProjCallback&)m_04)();
        m_04 = 0;
        m_28 = 0x3e9;
    }
    MovingLogicVfunc3(); // virtual slot 16 (vtable offset 0x40)
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
// the body is byte-identical through `m_204.RemoveAll()` (the m_200 stop, the
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
    if (m_200 != 0) {
        m_200->StopAndRewind();
        m_200 = 0;
    }
    for (POSITION pos = m_204.GetHeadPosition(); pos != NULL;) {
        CObject* data = m_204.GetNext(pos);
        if (data != 0) {
            void** node = (void**)((char*)data - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_204.RemoveAll();
}

// ===========================================================================
// CProjectile::LoadProjectileSprites (0xdf050, /GX) - resolve the projectile's
// per-type sprite frames + launch trajectory at spawn. Snap the two grid
// endpoints to tile centres (m_17c/m_180), record the target/owner ids, then
// switch on the projectile kind to pick the sprite-set base name + the
// "<Kind>ProjectileTimePerTile" bute value (m_190) and the arc flag (m_1d8; Wingz
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
    m_174 = a;
    m_17c = (sx & ~0x1f) + 0x10;
    m_178 = b;
    m_170 = kind;
    m_180 = (sy & ~0x1f) + 0x10;
    m_220 = t0;
    m_224 = t1;

    CGameObject* owner = m_10;
    double dx = (double)(m_17c - owner->m_5c);
    double dy = (double)(m_180 - owner->m_60);
    i32 count = 1;

    switch (kind) {
        case PROJ_ROCK: // ROCK
            key = "GRUNTZ_ROCKGRUNT_PROJECTILE";
            m_190 = g_buteMgr.GetDwordDef(
                (char*)"Projectile",
                (char*)"RockProjectileTimePerTile",
                0xbb8
            );
            m_1d8 = 1;
            break;
        case PROJ_GUNHAT: // GUNHAT
            key = "GRUNTZ_GUNHATGRUNT_PROJECTILE";
            m_190 = g_buteMgr.GetDwordDef(
                (char*)"Projectile",
                (char*)"GunhatProjectileTimePerTile",
                0xbb8
            );
            m_1d8 = 1;
            break;
        case PROJ_BOOMERANG: // BOOMERANG
            key = "GRUNTZ_BOOMERANGGRUNT_PROJECTILE";
            m_190 = g_buteMgr.GetDwordDef(
                (char*)"Projectile",
                (char*)"BoomerangProjectileTimePerTile",
                0xbb8
            );
            m_1d8 = 0;
            break;
        case PROJ_NERFGUN: // NERFGUN
            key = "GRUNTZ_NERFGUNGRUNT_PROJECTILE";
            m_190 = g_buteMgr.GetDwordDef(
                (char*)"Projectile",
                (char*)"NerfGunProjectileTimePerTile",
                0xbb8
            );
            m_1d8 = 1;
            break;
        case PROJ_WELDER: // WELDER
            key = "GRUNTZ_WELDERGRUNT_PROJECTILE";
            m_190 = g_buteMgr.GetDwordDef(
                (char*)"Projectile",
                (char*)"WelderProjectileTimePerTile",
                0xbb8
            );
            m_1d8 = 1;
            break;
        case PROJ_WINGZ: { // WINGZ
            key = "GRUNTZ_WINGZGRUNT_PROJECTILE";
            m_190 = g_buteMgr.GetDwordDef(
                (char*)"Projectile",
                (char*)"WingzProjectileTimePerTile",
                0xbb8
            );
            LaunchSound("GRUNTZ_WINGZGRUNT_WINGZGRUNTLOOP");
            m_1d8 = 0;
            i32 ddx = (m_17c >> 5) - (owner->m_5c >> 5);
            if (ddx < 0) {
                ddx = -ddx;
            }
            i32 ddy = (m_180 >> 5) - (owner->m_60 >> 5);
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
    CProjSpriteMap& map = m_154->m_c->m_2c->m_10;
    void* out;
    out = 0;
    map.Lookup(key + "1", &out);
    m_1e0 = out;
    if (m_1e0 == 0) {
        return 0;
    }
    out = 0;
    map.Lookup(key + "2", &out);
    m_1e4 = out;
    out = 0;
    map.Lookup(key + "3", &out);
    m_1e8 = out;
    out = 0;
    map.Lookup(key + "4", &out);
    m_1ec = (i32)out;
    out = 0;
    map.Lookup(key + "5", &out);
    m_1f0 = (i32)out;
    out = 0;
    map.Lookup(key + "IMPACT", &out);
    m_1f4 = out;
    out = 0;
    map.Lookup(key + "FALL", &out);
    m_1f8 = out;

    m_15c = m_154->m_1b4;
    m_154->m_1a0.Setup(m_1e0);
    m_154->CacheFirstFrame(key + "_OBJECT");

    // Normalise the launch trajectory into the per-frame velocity + sign vectors.
    u32 totalTime = (u32)(count * m_190);
    double len = sqrt(dx * dx + dy * dy);
    double t = (double)totalTime;
    double vx = dx / len;
    m_188 = len;
    m_198 = len / t;
    m_1a0 = vx;
    m_1a8 = dy / len;
    m_1b0 = vx;
    m_1b8 = dy / len;
    // sign(dx): +0.5 / 0.0 / -0.5 stored as a double {lo=0, hi=+-0x3fe00000}
    m_1c0 = 0;
    if (vx > 0.0) {
        m_1c4 = 0x3fe00000;
    } else if (vx < 0.0) {
        m_1c4 = (i32)0xbfe00000;
    } else {
        m_1c4 = 0;
    }
    m_1c8 = 0;
    if (dy > 0.0) {
        m_1cc = 0x3fe00000;
    } else if (dy < 0.0) {
        m_1cc = (i32)0xbfe00000;
    } else {
        m_1cc = 0;
    }
    m_188 = len < 0.0 ? -len : len;
    m_1d0 = owner->m_5c;
    m_1d4 = owner->m_60;
    m_1dc = 0;

    // Spawn the LightFx shadow companion + activate its two frames.
    CProjSpriteFactory* factory = (CProjSpriteFactory*)g_gameReg->m_30->m_8;
    m_1fc = factory->CreateSprite(0, owner->m_5c, owner->m_60, 0xcf84f, "LightFx", 0x2040003);
    if (m_1fc != 0) {
        m_1fc->m_7c->Init(m_1fc);
        m_1fc->m_7c->m_18->Activate(key + "_SHADOW", key + "1", 5, 1);
    }

    // Latch the class act key ("A"): save the old registry node, then re-point it.
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
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

// The activation-collection methods (shared with the per-class registries):
//   Find  0x16da80 (__thiscall ret 8), Insert 0x16d850 (__thiscall ret 0xc),
//   ActAlloc 0x16d990, RegisterRange 0x408710 (via 0x3742 thunk).
struct CProjColl {
    i32 Find(i32 coord, i32 z);         // 0x16da80
    void RegisterRange(i32 lo, i32 hi); // 0x408710 (0x0df920 callee)
};
struct CProjColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850
};
extern "C" i32 ProjActAlloc(); // 0x16d990
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464 (shared alloc cache)
DATA(0x002bf428)
extern void* g_projActAllocResult; // 0x6bf428

// R1 - the shared type-name table (@0x6bf650).
struct CProjTypeEntry;
DATA(0x002bf658)
extern i32 g_projTypeLo;
DATA(0x002bf65c)
extern i32 g_projTypeHi;
DATA(0x002bf660)
extern char* g_projTypeBase;
DATA(0x002bf668)
extern i32 g_projTypeStride;
DATA(0x002bf664)
extern CProjTypeEntry* g_projTypeCur;
DATA(0x002bf670)
extern i32 g_projTypeCount;
DATA(0x002bf650)
extern CProjColl g_projTypeColl;
DATA(0x002bf654)
extern CProjColl2* g_projTypeColl2;
DATA(0x002bf66c)
extern void* g_projTypeNodes;
DATA(0x0021aea8)
extern i32 g_projTypeCounter; // 0x61aea8 (global type counter)

// R2 - the projectile's per-coordinate activation table (@0x64c758).
struct CProjActEntry;
DATA(0x0024c758)
extern CProjColl g_projActColl;

// The CString slot teardown (0x1b9b93 __thiscall) + name assign (0x1b9e74).
struct CProjStringNode {
    void* m_0;
    void Free(); // 0x1b9b93
};
struct CProjTypeEntryView {
    void Assign(const char* name); // 0x1b9e74
};

// The projectile's activation handler (LAB_00403896, an ILT thunk).
extern "C" void ProjActivationHandler(); // 0x403896

// R2 lookup (projectile activation table).
static inline CProjActEntry* ProjActLookup(i32 coord) {
    g_projActScratch = 0;
    if (coord >= g_projActLo && coord <= g_projActHi) {
        return (CProjActEntry*)(g_projActBase + (coord - g_projActLo) * g_projActStride);
    }
    if (g_projActColl.Find(coord, 0)) {
        return (CProjActEntry*)(g_projActBase + (coord - g_projActLo) * g_projActStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    g_projActColl2->Insert(&g_projActColl, item, 0xc);
    return g_projActCur;
}

// R1 lookup (shared type-name table).
static inline CProjTypeEntry* ProjTypeLookup(i32 key) {
    g_projTypeCount = 0;
    if (key >= g_projTypeLo && key <= g_projTypeHi) {
        return (CProjTypeEntry*)(g_projTypeBase + (key - g_projTypeLo) * g_projTypeStride);
    }
    if (g_projTypeColl.Find(key, 0)) {
        return (CProjTypeEntry*)(g_projTypeBase + (key - g_projTypeLo) * g_projTypeStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    g_projTypeColl2->Insert(&g_projTypeColl, item, 0xc);
    return g_projTypeCur;
}

// CProjectile::RegisterRange @0x0df920 - seed the projectile's activation table's
// fast-range bounds (RegisterRange(0x7d0, 0x7da)). A static initializer.
RVA(0x000df920, 0x15)
void CProjectile::RegisterRange() {
    g_projActColl.RegisterRange(0x7d0, 0x7da);
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
        CProjTypeEntry* slot = ProjTypeLookup(key);
        i32 cnt = g_projTypeCount;
        CProjStringNode* nodes = (CProjStringNode*)g_projTypeNodes;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    nodes->Free();
                }
                nodes++;
            } while (--cnt);
        }
        ((CProjTypeEntryView*)slot)->Assign("A");
        g_projTypeCounter++;
    }
    *(void**)ProjActLookup(id) = (void*)&ProjActivationHandler;
}

// ===========================================================================
// CProjectile::LoadProjectileEffects (0xdfd00) - per-frame trajectory advance +
// impact-effect select. Runs each frame until the projectile reaches its target
// tile (m_1d0/m_1d4 catch up to m_17c/m_180): integrate the parabola into the
// render position (m_1a0/m_1a8), clamp the muzzle-tracked position against the
// target, and for the arc kinds (m_1d8) select one of five impact-effect sprite
// tiers by the fractional distance-to-target. On arrival it stops the loop
// sound, runs a final hit-scan, then - by the destination terrain flags - spills
// the water / level death splash effect or installs the IMPACT/FALL sprite. The
// one-shot m_1dc latch (0 while in flight; set on arrival) gates re-entry.
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
// tier comparisons and pre-computes m_188*0.9 interleaved with the fsqrt, which
// is not steerable from C source - plus the effect-spawn regalloc (ecx vs edx for
// reg->m_30) and the unnamed engine-call relocs. Logic complete; parked.
// ===========================================================================
RVA(0x000dfd00, 0x6f5)
void CProjectile::LoadProjectileEffects() {
    if (m_1dc != 0) {
        return;
    }

    if (m_170 == 0x16) { // WINGZ: loop the flight sound while over the level
        CGameObject* owner = m_10;
        CGameRegistry* reg = g_gameReg;
        if (owner->m_5c < reg->m_144 && owner->m_5c >= reg->m_13c && owner->m_60 < reg->m_148
            && owner->m_60 >= reg->m_140) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_200 != 0) {
            m_200->StopAndRewind();
            m_200 = 0;
        }
    }

    if (m_1d0 != m_17c || m_1d4 != m_180) {
        // -- in flight: integrate one frame + select the impact tier ----------
        if (m_170 == 0x16) {
            ScanTargets(0);
        }
        m_1a0 = m_1a0 + (double)(u32)g_645584 * m_1b0 * m_198;
        m_1a8 = m_1a8 + (double)(u32)g_645584 * m_1b8 * m_198;
        i32 xRes = (i32)(*(double*)&m_1c0 + m_1a0);
        i32 yRes = (i32)(*(double*)&m_1c8 + m_1a8);
        i32 localX = xRes;
        if (m_1b0 > 0.0) {
            if (xRes > m_17c) {
                localX = m_17c;
                xRes = m_17c;
            }
        } else if (m_1b0 < 0.0) {
            if (xRes < m_17c) {
                localX = m_17c;
                xRes = m_17c;
            }
        }
        if (m_1b8 > 0.0) {
            if (yRes > m_180) {
                yRes = m_180;
            }
        } else if (m_1b8 < 0.0) {
            if (yRes < m_180) {
                yRes = m_180;
            }
        }
        m_1d0 = xRes;
        m_1d4 = yRes;
        i32 offX = 0;
        i32 offY = 0;
        if (m_1d8 != 0) {
            double dx = fabs((double)m_17c - m_1a0);
            double dy = fabs((double)m_180 - m_1a8);
            double dist = sqrt(dx * dx + dy * dy);
            double mag = m_188;
            if (dist >= mag * 0.9 || dist < mag * 0.1) {
                offX = 0x4;
                offY = -0x4;
                if (m_154->m_1b4 != (i32)m_1e0) {
                    m_15c = m_154->m_1b4;
                    m_154->m_1a0.Setup(m_1e0);
                    if (m_1fc != 0) {
                        m_1fc->m_1a0.Setup(m_1e0);
                    }
                }
            } else if (dist >= mag * 0.8 || dist < mag * 0.2) {
                offX = 0x8;
                offY = -0x8;
                if (m_154->m_1b4 != (i32)m_1e4) {
                    m_15c = m_154->m_1b4;
                    m_154->m_1a0.Setup(m_1e4);
                    if (m_1fc != 0) {
                        m_1fc->m_1a0.Setup(m_1e4);
                    }
                }
            } else if (dist >= mag * 0.7 || dist < mag * 0.3) {
                offX = 0xc;
                offY = -0xc;
                if (m_154->m_1b4 != (i32)m_1e8) {
                    m_15c = m_154->m_1b4;
                    m_154->m_1a0.Setup(m_1e8);
                    if (m_1fc != 0) {
                        m_1fc->m_1a0.Setup(m_1e8);
                    }
                }
            } else if (dist >= mag * 0.6 || dist < mag * 0.4) {
                offX = 0x10;
                offY = -0x10;
                if (m_154->m_1b4 != m_1ec) {
                    m_15c = m_154->m_1b4;
                    m_154->m_1a0.Setup((void*)m_1ec);
                    if (m_1fc != 0) {
                        m_1fc->m_1a0.Setup((void*)m_1ec);
                    }
                }
            } else {
                offX = 0x14;
                offY = -0x14;
                if (m_154->m_1b4 != m_1f0) {
                    m_15c = m_154->m_1b4;
                    m_154->m_1a0.Setup((void*)m_1f0);
                    if (m_1fc != 0) {
                        m_1fc->m_1a0.Setup((void*)m_1f0);
                    }
                }
            }
        }
        m_10->m_5c = offX + m_1d0;
        m_10->m_60 = offY + m_1d4;
        if (m_1fc != 0) {
            m_1fc->m_5c = localX;
            m_1fc->m_60 = yRes;
        }
        return;
    }

    // -- arrived at the target tile ------------------------------------------
    if (m_200 != 0) {
        m_200->StopAndRewind();
        m_200 = 0;
    }
    ScanTargets(0);
    if (m_1fc != 0) {
        m_1fc->m_08 |= 0x10000;
        m_1fc = 0;
    }
    m_1dc = 1;
    i32 tier = 0;
    if (m_170 != 0x16) {
        CGameRegistry* reg = g_gameReg;
        CTerrainPlane* plane = (CTerrainPlane*)reg->m_70;
        i32 tileX = m_17c >> 5;
        i32 tileY = m_180 >> 5;
        u32 flags;
        if ((u32)tileX >= (u32)plane->m_c || (u32)tileY >= (u32)plane->m_10) {
            flags = 1;
        } else {
            flags = plane->m_8[tileY][tileX].m_0;
        }
        if (flags & 0x900) {
            // water tile: spill a splash then hide the projectile
            if (m_17c < reg->m_144 && m_17c >= reg->m_13c && m_180 < reg->m_148
                && m_180 >= reg->m_140) {
                CProjRenderObj* fx =
                    ((CProjSoundCat*)reg->m_30)
                        ->m_8->CreateSprite(0, m_17c, m_180, 0xcf84f, "Particlez", 0x40003);
                if (fx != 0) {
                    fx->CacheFirstFrame("GAME_WATER");
                    fx->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
            m_154->m_08 |= 0x10000;
            return;
        }
        if (flags & 0x2) {
            if (flags & 0x40) {
                tier = 1;
            } else {
                switch (((CProjLevelInfo*)reg->m_2c)->m_20) {
                    case 4:
                    case 5:
                    case 8:
                        tier = 1;
                        break;
                    case 6:
                        break;
                    default:
                        // level death tile: spill the death-splash then hide
                        if (m_17c < reg->m_144 && m_17c >= reg->m_13c && m_180 < reg->m_148
                            && m_180 >= reg->m_140) {
                            CProjRenderObj* fx =
                                ((CProjSoundCat*)reg->m_30)
                                    ->m_8
                                    ->CreateSprite(0, m_17c, m_180, 0xcf84f, "Particlez", 0x40003);
                            if (fx != 0) {
                                fx->CacheFirstFrame("LEVEL_DEATHSPLASH");
                                fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                            }
                        }
                        m_154->m_08 |= 0x10000;
                        return;
                }
            }
        }
    }
    void* sprite = (tier != 0) ? m_1f8 : m_1f4;
    if (sprite == 0) {
        m_154->m_08 |= 0x10000;
        return;
    }
    m_15c = m_154->m_1b4;
    m_154->m_1a0.Setup(sprite);
}

// ---------------------------------------------------------------------------
// CProjectile::DetachRenderObj (0xe05e0) - clear the render object's bit-0 flag,
// re-target its animation to the current draw-delta, and (when the object is in
// the "active but un-anchored" state) raise its hide bit. Returns 0.
// ---------------------------------------------------------------------------
RVA(0x000e05e0, 0x4e)
i32 CProjectile::DetachRenderObj() {
    m_154->m_40 &= ~1u;
    m_154->m_1a0.SetAnim(g_6bf3bc);
    CProjRenderObj* r = m_154;
    if (r->m_1c8 != 0 && r->m_1c0 == 0) {
        r->m_08 |= 0x10000;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CProjectile::StepMotion (0xe08b0) - advance the projectile one frame. On the
// launch frame it snaps the render objects to the muzzle (m_17c/m_180); once past
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
    if (m_258 == 0) {
        if (m_250 > g_projPhase0) {
            // launch frame: snap render objects to the muzzle, mark launched.
            m_10->m_5c = m_17c;
            m_10->m_60 = m_180;
            if (m_1fc != 0) {
                m_1fc->m_5c = m_17c;
                m_1fc->m_60 = m_180;
            }
            m_258 = 1;
            goto step;
        }
    } else if (m_250 > g_projPhase1) {
        // past the terminal threshold: deliver the impact scan + expire.
        ScanTargets(1);
        if (m_1fc != 0) {
            m_1fc->m_08 |= 0x10000;
            m_1fc = 0;
        }
        m_154->m_08 |= 0x10000;
        return;
    }
step:
    ScanTargets(impact);
    // integrate the sin/cos parabola into the render position.
    double s = sin(m_250);
    double c = cos(m_250);
    double amp = (double)g_645584;
    double vx = -m_230;
    double vy = m_238;
    double px = m_240 + vy * m_198 * s - vx * amp * c + m_250;
    double py = m_248 + vy * amp * c + vx * m_198 * s;
    m_1a0 = px;
    m_1a8 = py;
    m_250 = px;
    m_10->m_5c = (i32)m_1a0;
    m_10->m_60 = (i32)m_1a8;
    if (m_1fc != 0) {
        m_1fc->m_5c = (i32)m_1a0;
        m_1fc->m_60 = (i32)m_1a8;
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
    i32 tileY = 0;                   // [esp+0x10]  outer (row) counter
    i32 projXlo = m_10->m_5c - 0x10; // [esp+0x1c]  m_10 = owner CGameObject
    i32 projYlo = m_10->m_60 - 0x10; // [esp+0x20]
    i32 projXhi = projXlo + 0x20;    // [esp+0x24]
    i32 projYhi = projYlo + 0x20;    // [esp+0x28]
    i32 rowBase = 0x1c;              // [esp+0x18]  row byte stride base
    i32 colOff;                      // [esp+0x14]
    i32 col;                         // ebp
    do {
        col = 0;
        colOff = rowBase;
        for (; col < 0xf; col++, colOff += 4) {
            CGruntTarget* g = *(CGruntTarget**)((char*)g_gameReg->m_68 + colOff);
            if (g == 0) {
                continue;
            }
            if (g->m_1fc == 0) {
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
            if (m_174 == tileY && m_178 == col) {
                // self cell
                if (impact != 0 && g->m_1fc != 0 && g->m_170 == 0) {
                    g->SelfImpact(2, 1, 0, 0);
                }
                return;
            }
            // already-tracked? walk the hit list for this grunt's cell key.
            i32 keyX = g->m_1ec;
            i32 keyY = g->m_1f0;
            for (POSITION pos = m_204.GetHeadPosition(); pos != NULL;) {
                CHitKey* k = (CHitKey*)m_204.GetNext(pos);
                if (k->m_0 == keyX && k->m_4 == keyY) {
                    return;
                }
            }
            // fresh hit: pull a key node off the free-list, record + deliver.
            CHitKey* slot = 0;
            CHitKey* p = (CHitKey*)g_freeList;
            if (p->m_0 != 0) {
                slot = (CHitKey*)&p->m_4;
                slot->m_0 = keyX;
                slot->m_4 = keyY;
                g_freeList = (void*)p->m_0;
            }
            m_204.AddTail((CObject*)slot);
            g->DeliverHit(m_170, 1, m_174, m_178, m_220, m_224, 1, 0);
        }
        rowBase += 0x3c;
        tileY++;
    } while (rowBase < 0x10c);
}

// ---------------------------------------------------------------------------
// CProjectile::LaunchSound (0xe2190) - lazily create + play the launch sound the
// first time. Look the effect up in the game-registry sound map by name, clone a
// sample off the matched entry, store it at m_200, and start it on the configured
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
    if (m_200 != 0) {
        return 0;
    }
    CGameRegistry* reg = g_gameReg;
    if (reg->m_10 == 0) {
        return 0;
    }
    CProjSoundCat* cat = (CProjSoundCat*)reg->m_30;
    CProjSoundEntry* entry = 0;
    cat->m_28->m_10.Lookup(key, &entry);
    if (entry == 0) {
        return 0;
    }
    if (entry->m_10 == 0) {
        return 0;
    }
    m_200 = entry->m_10->GetItem();
    if (m_200 == 0) {
        return 0;
    }
    m_200->Play(g_gameReg->m_11c, 0, 0, 1);
    return 1;
}

// ===========================================================================
// 0x0ade60 - per-coordinate projectile-action dispatch.  Resolves the activation
// entry for `coord` (R2 lookup, inlined); if the entry's leading handler slot is
// non-null, re-resolves the entry and invokes the handler (__thiscall) on this
// dispatcher object.  Same global-table-driven shape as ProjActLookup's callers.
// ===========================================================================
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
    CProjActEntry* e = ProjActLookup(coord);
    if (*(void**)e != 0) {
        CProjActEntry* e2 = ProjActLookup(coord);
        ProjActHandler h = *(ProjActHandler*)e2;
        (this->*h)();
    }
}
