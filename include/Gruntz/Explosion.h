#ifndef GRUNTZ_CEXPLOSION_H
#define GRUNTZ_CEXPLOSION_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CExplosion : public CUserLogic, public CWapX {
public:

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
         {
            SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
        }
    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_EXPLOSION;
    }

public:
    CExplosion() {}
    CExplosion(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    i32 Update();
};

#endif
