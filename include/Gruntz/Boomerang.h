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
    virtual i32 LaunchProjectile(
        PickupType weaponType,
        i32 sourcePlayerIndex,
        i32 sourceUnitIndex,
        i32 targetPxX,
        i32 targetPxY,
        i32 sourcePxX,
        i32 sourcePxY
    ) OVERRIDE;

    i32 m_launchX, m_launchY;
    double m_orbitRadiusX, m_orbitRadiusY;
    double m_orbitCenterX, m_orbitCenterY;
    double m_orbitAngle;
    b32 m_returning;
};
#endif // GRUNTZ_BOOMERANG_H
