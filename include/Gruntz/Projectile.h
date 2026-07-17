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
// CProjectile adds the CPtrList at +0x204 (the projectile's tracked-hit list, MFC
// block size 10) and its own most-derived vftable. The no-arg ctor (0x126e0)
// folds the whole CMovingLogic init, constructs the list, and stamps its vptr.
#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

class CLightFx; // folded CProjShadowActivate

#include <Mfc.h>                // CPtrList (+0x204 member)
#include <Gruntz/MovingLogic.h> // CMovingLogic base (pulls UserLogic.h) + bound externs
#include <rva.h>

// The animation sub-object embedded in a render object at +0x1a0. It bridges the two
// (The former CProjAnim / CProjSpriteMgr / CProjResMgr / CProjRenderObj views are
// ARE canonical CGameObjects (<Gruntz/UserLogic.h>) - m_08==m_flags, m_c==m_0c
// (the CDDrawSurfaceMgr; the frame map is m_0c->m_animRegistry->m_10, the mfc_class-proven
// CMapStringToPtr band 0x1b8438), m_40==m_stateFlags, m_5c/m_60==m_screenX/Y,
// +0x1a0 the embedded CAniAdvanceCursor (SetGeometry==Setup_15c2d0, Advance
// @0x15c360), m_1b4==m_1a0.m_14 and the +0x1c0/+0x1c8 gates its m_20/m_28.)

// (The former CProjShadowSub/CProjShadowActivate views of the shadow's +0x7c
// is m_notify and the "+0x18 activation host" the bound CLightFx leaf at
// m_logic - its real Activate is CLightFx::Activate @0x9d520.)

// The sound sample object the projectile launches (+0x200) IS a DirectSoundMgr: the
// cue-mgr's GetItem hands back the pooled DirectSound buffer, and StopAndRewind
// (0x135380) / ApplyAndPlay are DirectSoundMgr methods.
class DirectSoundMgr; // <Dsndmgr/DirectSoundMgr.h> - the pooled DirectSound buffer

// ---------------------------------------------------------------------------
// CProjectile : CMovingLogic - 18 virtuals (vftable 0x5e798c). Adds the
// tracked-hit CPtrList at +0x204 plus the projectile's render/motion state
// (+0x140..+0x224); sizeof == 0x228. Field names are placeholders; the OFFSETS +
// code bytes are the load-bearing facts.
// ---------------------------------------------------------------------------
SIZE(CProjectile, 0x228);
class CProjectile : public CMovingLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012960, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PROJECTILE;
    } // slot 2
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

    static void RegisterRange(); // 0xdf920 (seed the activation-table fast range)
    // slot-4 (UserLogicVfunc2) per-coordinate activation dispatch @0xdf9a0. The fat
    // base models slot 4 with the no-arg UserLogicVfunc2() placeholder, so the int-arg
    // real shape is a plain method (the leaf vtable slot stays base-attributed).
    virtual void FireActivation(i32 id) OVERRIDE; // 0xdf9a0
    static void RegisterType();                   // 0xdfb00 (level-load class registrar)
    // (ReleaseDeferred @0x13c70 is GONE from here: it is CMovingLogic::FinalizeStep,
    //  the inherited slot-5 override - see MovingLogic.h / Projectile.cpp.)
    i32 DetachRenderObj();                // 0xe05e0  (clear +0x154's flag, detach, gate hide)
    void ScanTargets(i32 impact);         // 0xe0b10  (15x15 grid hit-scan against nearby grunts)
    i32 LaunchSound(const char* key);     // 0xe2190 (create + play the launch CSample)
    virtual void MovingSlot16() OVERRIDE; // slot 16 @0xdfd00 (impact/particle effects)

    // +0x140..+0x14f (m_140/m_144/m_148/m_14c) belong to the CMovingLogic base
    // now (its Update/Serialize round-trip touches them); CProjectile's own data
    // resumes at +0x150.
    i32 m_150;                    // +0x150  (= owner; write-only here, role unproven)
    CGameObject* m_sprite;        // +0x154  primary sprite/render object (== owner)
    i32 m_158;                    // +0x158  (= owner->m_7c; write-only here)
    CAniElement* m_savedFrameGeo; // +0x15c  saved m_sprite->m_1a0.m_14 before each anim Setup
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
    CAniElement* m_frame1;        // +0x1e0  sprite frame "<base>1" (resolved geometry)
    CAniElement* m_frame2;        // +0x1e4  sprite frame "<base>2"
    CAniElement* m_frame3;        // +0x1e8  sprite frame "<base>3"
    CAniElement *m_frame4, *m_frame5; // +0x1ec/+0x1f0  sprite frames "<base>4"/"5"
    CAniElement* m_impactSprite;      // +0x1f4  "<base>IMPACT" sprite
    CAniElement* m_fallSprite;        // +0x1f8  "<base>FALL" sprite
    CGameObject* m_shadow;            // +0x1fc  LightFx shadow render companion
    DirectSoundMgr* m_sound;          // +0x200  launch sound sample (pooled DirectSound buffer)
    CPtrList m_hitList;               // +0x204  tracked-hit list (block size 10)
    i32 m_targetId, m_ownerId;        // +0x220/+0x224  target/owner ids passed to DeliverHit
    // sizeof(CProjectile) == 0x228 (proven: LogicDispatchE @0xde8a0 `new CProjectile`
    // pushes 0x228). The boomerang return-trajectory fields (+0x228..+0x258) belong to
    // the derived CBoomerang (<Gruntz/Boomerang.h>), NOT here - see StepMotion.
};
VTBL(CProjectile, 0x1e798c);

// The projectile activation entry: its first dword is the registered class handler,
// stored by the registrar as a free-fn ptr but dispatched __thiscall on `this` - a
// 4-byte single-inheritance PMF gives the plain `mov ecx,this; call [entry]` code.
// (Was the .cpp-local CProjActEntry view; CProjectile is complete above so the PMF
// stays 4 bytes - pmf-complete-class-4byte.)
typedef i32 (CProjectile::*ProjActHandler)();
struct CProjActEntry {
    ProjActHandler m_fn;
};
SIZE_UNKNOWN(CProjActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_PROJECTILE_H
