#ifndef GRUNTZ_CEXITTRIGGER_H
#define GRUNTZ_CEXITTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CWarlord;

class CExitTrigger : public CUserLogic, public CWapX {
public:
public:
    CExitTrigger() {}
    CExitTrigger(CGameObject* obj);

    RVA(0x00010870, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EXITTRIGGER;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();

    CWarlord* m_warlordLogic;
    i32 m_resolved;
};
SIZE(0x5c);

#endif // GRUNTZ_CEXITTRIGGER_H
