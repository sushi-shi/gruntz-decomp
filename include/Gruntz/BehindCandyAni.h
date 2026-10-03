#ifndef GRUNTZ_CBEHINDCANDYANI_H
#define GRUNTZ_CBEHINDCANDYANI_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CBehindCandyAni : public CUserLogic, public CWapX {
public:
public:
    CBehindCandyAni() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CBehindCandyAni(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    static void RegisterActs();
    i32 AdvanceAnim();

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_BEHINDCANDYANI;
    }

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }
};

#endif
