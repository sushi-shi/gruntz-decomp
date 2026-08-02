#ifndef GRUNTZ_CKITCHENSLIME_H
#define GRUNTZ_CKITCHENSLIME_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CKitchenSlime : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x000130b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_KITCHENSLIME;
    }

public:
    static void RegisterType();
    virtual void FireActivation(i32 id) OVERRIDE;
    i32 Tick();
    i32 LoadSprites();
    CKitchenSlime() {}
    CKitchenSlime(CGameObject* obj);

    CGameObject* Level() {
        return m_object;
    }
    CWwdGameObjectA* Anim() {
        return m_wwdObject;
    }
    char m_pad54[0x58 - 0x54];
    double m_speed;
    double m_posX;
    double m_posY;
    double m_dirX;
    double m_dirY;
    Coord m_tilePosition;
    double m_stepMag;
};
SIZE(0x90);

extern const double g_slimeSpeedNum;
#endif // GRUNTZ_CKITCHENSLIME_H
