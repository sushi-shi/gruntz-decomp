#ifndef GRUNTZ_CVOICETRIGGER_H
#define GRUNTZ_CVOICETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CVoiceTrigger : public CUserLogic, public CWapX {
public:
public:
    CVoiceTrigger();
    CVoiceTrigger(CGameObject* obj);

    RVA(0x00013550, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_VOICETRIGGER;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 Tick();
};
SIZE(0x54);

#endif // GRUNTZ_CVOICETRIGGER_H
