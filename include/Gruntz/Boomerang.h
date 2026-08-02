#ifndef GRUNTZ_BOOMERANG_H
#define GRUNTZ_BOOMERANG_H

#include <rva.h>

#include <Gruntz/Projectile.h>

class CBoomerang : public CProjectile {
public:
    CBoomerang() {}
    CBoomerang(CGameObject* owner);

    virtual i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) OVERRIDE;
    RVA(0x000129b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BOOMERANG;
    }
    virtual void AdvanceMotion() OVERRIDE;
    virtual i32 LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1)
        OVERRIDE;

    i32 m_launchX, m_launchY;
    double m_dirX, m_dirY;
    double m_originX, m_originY;
    double m_phase;
    i32 m_launched;
};
SIZE(0x260);

extern const double g_boomHalf;
extern const double g_boomTimeScale;
extern const double g_boomRetC3;
extern const double g_boomRetC4;

extern const double g_projPhase0;
#endif // GRUNTZ_BOOMERANG_H
