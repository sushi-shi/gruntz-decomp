#ifndef GRUNTZ_CSECRETLEVELTRIGGER_H
#define GRUNTZ_CSECRETLEVELTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSecretLevelTrigger : public CUserLogic, public CWapX {
public:
    RVA(0x00010bb0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
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
SIZE(0x54);

#endif // GRUNTZ_CSECRETLEVELTRIGGER_H
