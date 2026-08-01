#ifndef GRUNTZ_CTILETRIGGERSWITCH_H
#define GRUNTZ_CTILETRIGGERSWITCH_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CTileTriggerSwitch : public CUserLogic, public CWapX {
    virtual LogicTypeId GetTypeTag() OVERRIDE;

public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;

public:
    CTileTriggerSwitch() {}
    CTileTriggerSwitch(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};
SIZE(0x54);

#endif // GRUNTZ_CTILETRIGGERSWITCH_H
