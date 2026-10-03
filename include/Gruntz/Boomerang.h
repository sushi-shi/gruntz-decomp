#ifndef GRUNTZ_BOOMERANG_H
#define GRUNTZ_BOOMERANG_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/SerialArchive.h>

class CBoomerang : public CProjectile {
public:
    CBoomerang() {}
    CBoomerang(CGameObject* owner);

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
         ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_BOOMERANG;
    }
    virtual void AdvanceMotion()  ;
    virtual i32 LoadProjectileSprites(
        PickupType kind,
        i32 sourcePlayerIndex,
        i32 sourceUnitIndex,
        i32 targetPxX,
        i32 targetPxY,
        i32 sourcePxX,
        i32 sourcePxY
    )  ;

    i32 m_launchX, m_launchY;
    double m_dirX, m_dirY;
    double m_originX, m_originY;
    double m_phase;
    b32 m_launched;
};
#endif
