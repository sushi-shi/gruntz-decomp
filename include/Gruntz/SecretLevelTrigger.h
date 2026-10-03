#ifndef GRUNTZ_CSECRETLEVELTRIGGER_H
#define GRUNTZ_CSECRETLEVELTRIGGER_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSecretLevelTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_SECRETLEVELTRIGGER;
    }

public:
    CSecretLevelTrigger();
    CSecretLevelTrigger(CGameObject* obj);
    static void RegisterActs();
    virtual void FireActivation(i32 id)  ;
    i32 Tick();
};

#endif
