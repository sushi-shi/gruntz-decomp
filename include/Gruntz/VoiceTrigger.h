#ifndef GRUNTZ_CVOICETRIGGER_H
#define GRUNTZ_CVOICETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
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
    RVA(0x000134e0, 0x47)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        return Chain(ar, tag, c, d) != 0;
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 Tick();
};

#endif // GRUNTZ_CVOICETRIGGER_H
