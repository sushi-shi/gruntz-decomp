#ifndef GRUNTZ_BOOMERANG_H
#define GRUNTZ_BOOMERANG_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/SerialArchive.h>

class CBoomerang : public CProjectile {
public:
    CBoomerang() {}
    CBoomerang(CGameObject* owner);

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE;
    RVA(0x000129b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BOOMERANG;
    }
    virtual void AdvanceMotion() OVERRIDE;
    virtual i32 LoadProjectileSprites(
        PickupType kind,
        i32 sourcePlayerIndex,
        i32 sourceUnitIndex,
        i32 targetPxX,
        i32 targetPxY,
        i32 sourcePxX,
        i32 sourcePxY
    ) OVERRIDE;

    i32 m_launchX, m_launchY;
    double m_dirX, m_dirY;
    double m_originX, m_originY;
    double m_phase;
    b32 m_launched;
};

extern const double g_boomerangMidpointScale;
extern const double g_boomerangPixelToTileScale;
extern const double g_boomerangHoldScale;
extern const double g_boomerangHoldBiasMs;

extern const double g_boomerangHalfTurnRadians;
extern const double g_boomerangFullTurnRadians;
#endif // GRUNTZ_BOOMERANG_H
