#ifndef GRUNTZ_CFRONTCANDYANI_H
#define GRUNTZ_CFRONTCANDYANI_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFrontCandyAni : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_FRONTCANDYANI;
    }

public:
    CFrontCandyAni() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CFrontCandyAni(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif
