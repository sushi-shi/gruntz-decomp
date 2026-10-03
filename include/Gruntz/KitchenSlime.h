#ifndef GRUNTZ_CKITCHENSLIME_H
#define GRUNTZ_CKITCHENSLIME_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CKitchenSlime : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_KITCHENSLIME;
    }

public:
    static void RegisterType();
    virtual void FireActivation(i32 id)  ;
    i32 Tick();
    i32 LoadSprites();
    CKitchenSlime() {}
    CKitchenSlime(CGameObject* obj);

    CGameObject* Level() {
        return m_object;
    }
    CWwdSpriteObject* Anim() {
        return m_wwdObject;
    }
    double m_speed;
    double m_posX;
    double m_posY;
    double m_dirX;
    double m_dirY;
    Coord m_tilePosition;
    double m_stepMag;
};

#endif
