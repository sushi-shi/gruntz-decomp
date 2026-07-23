#ifndef GRUNTZ_BOOMERANG_H
#define GRUNTZ_BOOMERANG_H

#include <Gruntz/Projectile.h> // real CProjectile base (pulls CMovingLogic/CUserLogic, CFileMemBase, LogicTypeId)
#include <rva.h>

class CBoomerang : public CProjectile {
public:
    CBoomerang(CGameObject* owner); // 0xe0650 (chains CProjectile(owner))
    // The five slots CBoomerang overrides over CProjectile's vtable (declared-only
    // unless a body is bound below; their vftable references reloc-mask).
    // The destructor is implicit: retail's compiler-generated body is only a
    // tail jump to CProjectile::~CProjectile, with no derived-vptr restamp.
    virtual i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, i32 a4)
        OVERRIDE; // slot 1 @0xe15d0
    RVA(0x000129b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE { // slot 2  (origin CUserBase)
        return LOGIC_BOOMERANG;
    }
    virtual void MovingSlot16() OVERRIDE; // slot 16 @0xe08b0 - the boomerang motion step
    virtual i32 LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1)
        OVERRIDE; // slot 17 @0xe0690 (origin CProjectile)

    // Return-trajectory state (+0x228..+0x258). Placeholders; OFFSETS are load-bearing.
    i32 m_launchX, m_launchY;    // +0x228/+0x22c  owner launch screen pos (return origin)
    double m_dirX, m_dirY;       // +0x230/+0x238  trajectory direction basis
    double m_originX, m_originY; // +0x240/+0x248  trajectory origin (arc midpoint)
    double m_phase;              // +0x250  trajectory parameter (sin/cos arg; phase gate)
    i32 m_launched;              // +0x258  launched latch
};
SIZE(0x260);

extern const double g_boomHalf;      // 0x5eaad8  midpoint scale
extern const double g_boomTimeScale; // 0x5eaae0
extern const double g_boomRetC3;     // 0x5eaaf0
extern const double g_boomRetC4;     // 0x5eaaf8

extern const double g_projPhase0;
#endif // GRUNTZ_BOOMERANG_H
