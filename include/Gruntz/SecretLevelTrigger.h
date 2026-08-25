#ifndef GRUNTZ_CSECRETLEVELTRIGGER_H
#define GRUNTZ_CSECRETLEVELTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSecretLevelTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00010b90, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SECRETLEVELTRIGGER;
    }

public:
    CSecretLevelTrigger();
    CSecretLevelTrigger(CGameObject* obj);
    static void RegisterActs();
    virtual void FireActivation(i32 id) OVERRIDE;
    i32 Tick();
};

#endif // GRUNTZ_CSECRETLEVELTRIGGER_H
