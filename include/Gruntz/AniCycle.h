#ifndef GRUNTZ_CANICYCLE_H
#define GRUNTZ_CANICYCLE_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CAniCycle : public CUserLogic, public CWapX {
public:
public:
    CAniCycle() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CAniCycle(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_ANICYCLE;
    }

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }

    virtual void FireActivation(i32 id)  ;

    static void RegisterActs();

    i32 AdvanceAnim();
};

#endif
