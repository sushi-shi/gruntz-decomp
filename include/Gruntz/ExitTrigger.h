#ifndef GRUNTZ_CEXITTRIGGER_H
#define GRUNTZ_CEXITTRIGGER_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWarlord;

class CExitTrigger : public CUserLogic, public CWapX {
public:
public:
    CExitTrigger() {}
    CExitTrigger(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_EXITTRIGGER;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
    i32 AdvanceAnim();

    CWarlord* m_warlordLogic;
    b32 m_resolved;
};

#endif
