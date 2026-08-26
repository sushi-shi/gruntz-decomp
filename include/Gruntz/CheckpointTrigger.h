#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00011440, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_CHECKPOINTTRIGGER;
    }

public:
    CCheckpointTrigger() {}
    CCheckpointTrigger(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 Act();
    i32 AdvanceCheckpointAnimation();
    i32 m_state[15];
    i32 m_firstEmpty;
};

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
