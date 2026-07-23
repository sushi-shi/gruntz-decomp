#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

class CLightFx; // folded CProjShadowActivate

#include <Mfc.h>                // CPtrList (+0x204 member)
#include <Gruntz/MovingLogic.h> // CMovingLogic base (pulls UserLogic.h) + bound externs
#include <rva.h>

class DirectSoundMgr; // <Dsndmgr/DirectSoundMgr.h> - the pooled DirectSound buffer

class CProjectile : public CMovingLogic, public CWapX {
public:
    // slot 1 @0xe0d40 (body below, ex the CProjLoadRec::Load view - the fold the
    // ProjLoadRec header deferred): dual-mode (4=write/7=read) serialize of the
    // trajectory block, the 7 sprite frames by registry key, the shadow by object
    // id, the hit list via the coord pool, then the CMovingLogic chain + the CWapX
    // record tail.
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE;
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

    // +0x140..+0x14f (m_140/m_144/m_148/m_14c) belong to the CMovingLogic base;
    // +0x150..+0x16f is the CWapX SECOND BASE (see the class note above) - what used
    // to be spelled here as m_150/m_sprite/m_158/m_savedFrameGeo/m_pad160 are its
    // m_34/m_38/m_3c/m_value/m_blob. CProjectile's own data starts at +0x170.
    i32 m_kind;                 // +0x170  projectile kind (ProjectileKind; 0x16=WINGZ)
    i32 m_srcRow, m_srcCol;     // +0x174/+0x178  launcher grid cell (row/col)
    i32 m_targetX, m_targetY;   // +0x17c/+0x180  destination tile-centre screen pos
    i32 m_184;                  // +0x184  (unreferenced in this TU)
    double m_flightDist;        // +0x188  launch distance sqrt(dx^2+dy^2), kept as fabs
    i32 m_timePerTile;          // +0x190  <Kind>ProjectileTimePerTile (GetDwordDef ms)
    i32 m_194;                  // +0x194  (unreferenced in this TU)
    double m_velScale;          // +0x198  per-frame velocity scale (dist / totalTime)
    double m_posX;              // +0x1a0  render position X (double)
    double m_posY;              // +0x1a8  render position Y (double)
    double m_velX;              // +0x1b0  velocity X basis (unit dir)
    double m_velY;              // +0x1b8  velocity Y basis (unit dir)
    i32 m_roundXLo, m_roundXHi; // +0x1c0/+0x1c4  X round-bias double {lo,hi} (0.0/+-0.5)
    i32 m_roundYLo, m_roundYHi; // +0x1c8/+0x1cc  Y round-bias double {lo,hi}
    i32 m_curX, m_curY;         // +0x1d0/+0x1d4  current screen pos (init = owner muzzle)
    i32 m_isArcing;             // +0x1d8  arced trajectory (per-type; drives 5-tier sprites)
    i32 m_arrived;              // +0x1dc  one-shot arrival latch (gates LoadProjectileEffects)
    // +0x1e0..+0x1f8  the 7 sprite frames, an ARRAY (SerializeMove round-trips
    // them in one 7-iteration registry-key loop - retail's loop proves the array):
    // [0..4] = "<base>1".."<base>5" (the 5-tier arc sprites), [PF_IMPACT] =
    // "<base>IMPACT", [PF_FALL] = "<base>FALL".
    enum {
        PF_IMPACT = 5,
        PF_FALL = 6
    };
    CAniElement* m_frames[7];
    CWwdGameObjectA* m_shadow; // +0x1fc  LightFx shadow render companion (A-kind)
    DirectSoundMgr* m_sound;   // +0x200  launch sound sample (pooled DirectSound buffer)
    CPtrList m_hitList;        // +0x204  tracked-hit list (block size 10)
    i32 m_targetId, m_ownerId; // +0x220/+0x224  target/owner ids passed to DeliverHit
    // sizeof(CProjectile) == 0x228 (proven: LogicDispatchE @0xde8a0 `new CProjectile`
    // pushes 0x228). The boomerang return-trajectory fields (+0x228..+0x258) belong to
    // the derived CBoomerang (<Gruntz/Boomerang.h>), NOT here - see StepMotion.
};

typedef i32 (CUserLogic::*ProjActHandler)();
struct CProjActEntry {
    ProjActHandler m_fn;
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

extern const double g_movingLogicMax; // 0x1f04b8 (2147483646.0)

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void ProjActivationHandler(); // 0x403896

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
#include <Gruntz/ActReg.h>
#include <Gruntz/HaznColl.h>

extern const double g_projPhase1;
#endif // GRUNTZ_PROJECTILE_H
