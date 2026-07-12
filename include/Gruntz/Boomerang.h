// Boomerang.h - CBoomerang : CProjectile (C:\Proj\Gruntz), the returning-projectile
// leaf of the CUserBase/CUserLogic/CMovingLogic/CProjectile subtree.
//
// Hierarchy (RTTI in GRUNTZ.EXE):
//     CProjectile : CMovingLogic   vftable 0x5e798c  (sizeof 0x228)
//       +-- CBoomerang : CProjectile  vftable 0x5e792c  (sizeof 0x260)
//
// CBoomerang adds the return-trajectory state at +0x228..+0x258 (the launch origin,
// the arc direction/origin, the phase parameter, the launched latch) over the 0x228
// CProjectile body -> sizeof == 0x260. Proven: the ctor @0xe0650 chains
// ??0CProjectile@@QAE@PAUCGameObject@@@Z (thunk 0x37d8) then stamps ??_7CBoomerang@@6B@;
// LogicDispatchBoomerang @0xde9e0 `new CBoomerang` pushes 0x260. The trajectory fields
// are touched ONLY by CBoomerang methods (StepMotion / LoadProjectileSprites /
// SerializeMove), which is why they moved out of CProjectile.
#ifndef GRUNTZ_BOOMERANG_H
#define GRUNTZ_BOOMERANG_H

#include <Gruntz/Projectile.h> // real CProjectile base (pulls CMovingLogic/CUserLogic, CGruntArchive, LogicTypeId)
#include <rva.h>

// CBoomerang : CProjectile - 18 slots (vftable 0x5e792c). Overrides slots
// 0(scalar-dtor)/1(SerializeMove)/2(GetTypeTag)/16(MovingSlot16 = the boomerang motion
// step, @0xe08b0)/17(LoadProjectileSprites, @0xe0690); the rest are inherited from
// CProjectile/CUserLogic. cl emits ??_7CBoomerang@@6B@ from CProjectile's vtable with
// these overrides applied (all reloc-masked); the ctor stamps the vptr implicitly.
SIZE(CBoomerang, 0x260);
class CBoomerang : public CProjectile {
public:
    CBoomerang(CGameObject* owner); // 0xe0650 (chains CProjectile(owner))
    // The five slots CBoomerang overrides over CProjectile's vtable (declared-only
    // unless a body is bound below; their vftable references reloc-mask).
    virtual ~CBoomerang() OVERRIDE; // slot 0  (origin CUserBase)
    virtual i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4)
        OVERRIDE;                              // slot 1 @0xe15d0
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2  (origin CUserBase)
    virtual void MovingSlot16() OVERRIDE;      // slot 16 @0xe08b0 - the boomerang motion step
    virtual i32 LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1)
        OVERRIDE; // slot 17 @0xe0690 (origin CProjectile)

    // Return-trajectory state (+0x228..+0x258). Placeholders; OFFSETS are load-bearing.
    i32 m_launchX, m_launchY;    // +0x228/+0x22c  owner launch screen pos (return origin)
    double m_dirX, m_dirY;       // +0x230/+0x238  trajectory direction basis
    double m_originX, m_originY; // +0x240/+0x248  trajectory origin (arc midpoint)
    double m_phase;              // +0x250  trajectory parameter (sin/cos arg; phase gate)
    i32 m_launched;              // +0x258  launched latch
};
VTBL(CBoomerang, 0x1e792c);

#endif // GRUNTZ_BOOMERANG_H
