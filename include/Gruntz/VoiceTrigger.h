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
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }
    virtual void FireActivation(i32 actionId) OVERRIDE;
    static void RegisterActs();

    i32 Tick();
};

#endif // GRUNTZ_CVOICETRIGGER_H
