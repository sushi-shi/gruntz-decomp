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

    RVA(0x000e15d0, 0x155)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (g_gameReg->m_world == 0) {
            return 0;
        }
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(&m_launchX, 4);
                ar->Read(&m_launchY, 4);
                ar->Read(&m_dirX, 8);
                ar->Read(&m_dirY, 8);
                ar->Read(&m_originX, 8);
                ar->Read(&m_originY, 8);
                ar->Read(&m_phase, 8);
                ar->Read(&m_launched, 4);
                break;
            case SERIAL_SAVE:
                ar->Write(&m_launchX, 4);
                ar->Write(&m_launchY, 4);
                ar->Write(&m_dirX, 8);
                ar->Write(&m_dirY, 8);
                ar->Write(&m_originX, 8);
                ar->Write(&m_originY, 8);
                ar->Write(&m_phase, 8);
                ar->Write(&m_launched, 4);
                break;
        }
        return CProjectile::SerializeMove(ar, mode, typeId, pObj) ? 1 : 0;
    }
    RVA(0x000129b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BOOMERANG;
    }
    virtual void AdvanceMotion() OVERRIDE;
    virtual i32 LoadProjectileSprites(PickupType kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1)
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
