#ifndef GRUNTZ_CTILETRIGGERSWITCH_H
#define GRUNTZ_CTILETRIGGERSWITCH_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CTileTriggerSwitch : public CUserLogic, public CWapX {
    RVA(0x00011030, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILETRIGGERSWITCH;
    }

public:
    RVA(0x00011050, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, pObj)
    }

public:
    CTileTriggerSwitch() {}
    CTileTriggerSwitch(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif // GRUNTZ_CTILETRIGGERSWITCH_H
