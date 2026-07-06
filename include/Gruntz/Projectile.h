// Projectile.h - the CMovingLogic / CProjectile game-object subtree
// (C:\Proj\Gruntz), continuing the CUserBase/CUserLogic hierarchy in
// <Gruntz/UserLogic.h>.
//
// Hierarchy (RTTI in GRUNTZ.EXE):
//     CUserBase                         vftable 0x5e70b4
//       +-- CUserLogic : CUserBase      vftable 0x5e705c
//             +-- CMovingLogic          vftable 0x5e87ac  (17 virtuals)
//                   +-- CProjectile     vftable 0x5e798c  (18 virtuals)
//
// CMovingLogic adds the projectile/moving-object motion state: a band of
// per-axis bookkeeping ints (+0x38..+0x10c) plus twelve `double` coordinate
// bounds (+0xa8..+0x13f) seeded to a default [-INF,+INF]-style box. Its ctor
// (out-of-line 0x13940) is also INLINED into the leaf ctors; modeled inline here
// so MSVC folds it into CProjectile::CProjectile.
//
// CProjectile adds the CObList at +0x204 (the projectile's tracked-hit list, MFC
// block size 10) and its own most-derived vftable. The no-arg ctor (0x126e0)
// folds the whole CMovingLogic init, constructs the list, and stamps its vptr.
#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

class CLightFx; // folded CProjShadowActivate

#include <Mfc.h>                // CObList (+0x204 member)
#include <Gruntz/MovingLogic.h> // CMovingLogic base (pulls UserLogic.h) + bound externs
#include <rva.h>

// The animation sub-object embedded in a render object at +0x1a0. Tick
// (0x15c360, __thiscall, 1 arg) advances the active animation by the draw clock
// (every call site feeds g_6bf3bc) and returns the anim state (2 = the fire/cue
// point - the grunt fire step gates on ==2; the detach path discards it). Setup
// (0x15c2d0, __thiscall, 1 arg) installs the resolved frame-0 sprite. The two
// state gates the fire step's finish tail reads live at +0x20/+0x28 of this
// sub-object == CProjRenderObj::m_1c0/m_1c8.
SIZE_UNKNOWN(CProjAnim);
struct CProjAnim {
}; // Setup=CDDrawBlitParam::Setup_15c2d0, Tick=CAniAdvanceCursor::Advance_15c360; cast at calls

// The name->sprite geometry map the sprite object owns (the CMapStringToOb the
// loaders Lookup by frame name). Reached via m_154->m_c->m_2c, map embedded @+0x10.
SIZE_UNKNOWN(CProjSpriteMap);
struct CProjSpriteMap {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at each call
SIZE_UNKNOWN(CProjSpriteMgr);
struct CProjSpriteMgr {
    char m_pad00[0x10];
    CProjSpriteMap m_10; // +0x10  the lookup map
};
SIZE_UNKNOWN(CProjResMgr);
struct CProjResMgr {
    char m_pad00[0x2c];
    CProjSpriteMgr* m_2c; // +0x2c
};

// A render object the projectile owns/points at (the +0x154 sprite/animation and
// the +0x1fc shadow companion). Only the offsets the reconstructed methods touch
// are modeled: +0x08 flag word, +0x0c resource host (frame lookup), +0x40 flag
// word, +0x5c/+0x60 screen position, +0x1a0 animation sub-object, +0x1b4 geometry
// word, +0x1c0/+0x1c8 state gates.
SIZE_UNKNOWN(CProjRenderObj);
struct CProjRenderObj {
    char m_pad00[0x08];
    u32 m_08;         // +0x08  flag word (|= 0x10000)
    CProjResMgr* m_c; // +0x0c  resource host (name->sprite map via m_2c)
    char m_pad10[0x40 - 0x10];
    u32 m_40; // +0x40  flag word (&= ~1)
    char m_pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char m_pad64[0x7c - 0x64];
    struct CProjShadowSub* m_7c; // +0x7c  shadow sub-table (Init @+0x10, host @+0x18)
    char m_pad80[0x1a0 - 0x80];
    CProjAnim m_1a0; // +0x1a0  animation sub-object (SetAnim(g_6bf3bc))
    char m_pad1a4[0x1b4 - 0x1a4];
    i32 m_1b4; // +0x1b4  geometry word (saved into CProjectile::m_savedFrameGeo)
    char m_pad1b8[0x1c0 - 0x1b8];
    i32 m_1c0; // +0x1c0
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8

    // CacheFirstFrame @0x150540 / ApplyLookupGeometry @0x1505b0 ARE CGruntSprite's; cast at the calls.
};

// The shadow companion's post-create sub-table (m_1fc->m_7c): an Init fn-ptr at
// +0x10 (fired with the shadow) and an "activation host" at +0x18 whose Activate
// (0x9d520) installs the shadow's two frame names.
SIZE_UNKNOWN(CProjShadowActivate);
struct CProjShadowActivate {
    void Activate(const char* shadowName, const char* baseName, i32 a, i32 b); // 0x9d520
};
SIZE_UNKNOWN(CProjShadowSub);
struct CProjShadowSub {
    char m_pad00[0x10];
    void (*Init)(CProjRenderObj* self); // +0x10
    char m_pad14[0x18 - 0x14];
    CLightFx* m_18; // +0x18
};

// The CSample-like sound sample object the projectile launches (+0x200). Its
// StopAndRewind (0x135380) is reached as an out-of-line engine method.
SIZE_UNKNOWN(CProjSample);
struct CProjSample {};

// ---------------------------------------------------------------------------
// CProjectile : CMovingLogic - 18 virtuals (vftable 0x5e798c). Adds the
// tracked-hit CObList at +0x204 plus the projectile's render/motion state
// (+0x140..+0x258). Field names are placeholders; the OFFSETS + code bytes are
// the load-bearing facts.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CProjectile);
class CProjectile : public CMovingLogic {
public:
    CProjectile();                   // 0x126e0 (no-arg)
    CProjectile(CGameObject* owner); // 0xdec60 (1-arg spawn ctor)
    virtual ~CProjectile() OVERRIDE; // most-derived dtor (0xdef60)
    // slot 17 (+0x44) - the ONE added virtual (anchors the new vftable; retail slot
    // holds thunk 0x13bb -> 0xdf050, and 0xdf050's only direct caller IS that thunk,
    // so every call is a virtual dispatch). Spawn-time load: resolve the projectile's
    // per-type sprite frames + launch trajectory (/GX). The grunt fire step
    // (ProjectileUpdate.cpp @0x61cb0) dispatches it on the fresh CreateSprite
    // result's aux->m_18 setup object: `mov eax,[ecx]; call [eax+0x44]`. Args: the
    // launcher grid cell (a,b), the target pixel pos (sx,sy; tile-snapped into
    // m_targetX/m_targetY), and t0/t1 -> m_targetId/m_ownerId.
    virtual i32
    LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1); // 0xdf050

    static void RegisterRange();   // 0xdf920 (seed the activation-table fast range)
    static void RegisterType();    // 0xdfb00 (level-load class registrar)
    void ReleaseDeferred(i32 arg); // 0x13c70 (fire/release the two queued callbacks; arg ignored)
    i32 DetachRenderObj();         // 0xe05e0  (clear +0x154's flag, detach, gate hide)
    void StepMotion();             // 0xe08b0  (advance the parabolic motion + render pos)
    void ScanTargets(i32 impact);  // 0xe0b10  (15x15 grid hit-scan against nearby grunts)
    i32 LaunchSound(const char* key); // 0xe2190 (create + play the launch CSample)
    void LoadProjectileEffects();     // 0xdfd00 (impact/particle effects)

    // +0x140..+0x14f (m_140/m_144/m_148/m_14c) belong to the CMovingLogic base
    // now (its Update/Serialize round-trip touches them); CProjectile's own data
    // resumes at +0x150.
    i32 m_150;                    // +0x150  (= owner; write-only here, role unproven)
    CProjRenderObj* m_sprite;     // +0x154  primary sprite/render object (== owner)
    i32 m_158;                    // +0x158  (= owner->m_7c; write-only here)
    i32 m_savedFrameGeo;          // +0x15c  saved m_sprite->m_1b4 before each anim Setup
    char m_pad160[0x170 - 0x160]; //
    i32 m_kind;                   // +0x170  projectile kind (ProjectileKind; 0x16=WINGZ)
    i32 m_srcRow, m_srcCol;       // +0x174/+0x178  launcher grid cell (row/col)
    i32 m_targetX, m_targetY;     // +0x17c/+0x180  destination tile-centre screen pos
    i32 m_184;                    // +0x184  (unreferenced in this TU)
    double m_flightDist;          // +0x188  launch distance sqrt(dx^2+dy^2), kept as fabs
    i32 m_timePerTile;            // +0x190  <Kind>ProjectileTimePerTile (GetDwordDef ms)
    i32 m_194;                    // +0x194  (unreferenced in this TU)
    double m_velScale;            // +0x198  per-frame velocity scale (dist / totalTime)
    double m_posX;                // +0x1a0  render position X (double)
    double m_posY;                // +0x1a8  render position Y (double)
    double m_velX;                // +0x1b0  velocity X basis (unit dir)
    double m_velY;                // +0x1b8  velocity Y basis (unit dir)
    i32 m_roundXLo, m_roundXHi;   // +0x1c0/+0x1c4  X round-bias double {lo,hi} (0.0/+-0.5)
    i32 m_roundYLo, m_roundYHi;   // +0x1c8/+0x1cc  Y round-bias double {lo,hi}
    i32 m_curX, m_curY;           // +0x1d0/+0x1d4  current screen pos (init = owner muzzle)
    i32 m_isArcing;               // +0x1d8  arced trajectory (per-type; drives 5-tier sprites)
    i32 m_arrived;                // +0x1dc  one-shot arrival latch (gates LoadProjectileEffects)
    void* m_frame1;               // +0x1e0  sprite frame "<base>1"
    void* m_frame2;               // +0x1e4  sprite frame "<base>2"
    void* m_frame3;               // +0x1e8  sprite frame "<base>3"
    void *m_frame4, *m_frame5;    // +0x1ec/+0x1f0  sprite frames "<base>4"/"5"
    void* m_impactSprite;         // +0x1f4  "<base>IMPACT" sprite
    void* m_fallSprite;           // +0x1f8  "<base>FALL" sprite
    CProjRenderObj* m_shadow;     // +0x1fc  LightFx shadow render companion
    CProjSample* m_sound;         // +0x200  launch sound sample
    CObList m_hitList;            // +0x204  tracked-hit list (block size 10)
    i32 m_targetId, m_ownerId;    // +0x220/+0x224  target/owner ids passed to DeliverHit
    char m_pad228[0x230 - 0x228]; //
    double m_dirX, m_dirY;        // +0x230/+0x238  trajectory direction basis
    double m_originX, m_originY;  // +0x240/+0x248  trajectory origin (base position)
    double m_phase;               // +0x250  trajectory parameter (sin/cos arg; phase gate)
    i32 m_launched;               // +0x258  launched flag
};

#endif // GRUNTZ_PROJECTILE_H
