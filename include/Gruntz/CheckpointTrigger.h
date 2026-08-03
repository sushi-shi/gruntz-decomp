#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic, public CWapX {
public:
    RVA(0x0010f9a0, 0x8f)
    virtual i32
    SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
        switch (mode) {
            case SERIAL_LOAD:
                sa->Read(m_state, 0x3c);
                sa->Read(&m_firstEmpty, 4);
                break;
            case SERIAL_SAVE:
                sa->Write(m_state, 0x3c);
                sa->Write(&m_firstEmpty, 4);
                break;
        }
        if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(sa, mode, typeId, pObj) ? 1 : 0;
    }
    RVA(0x00011430, 0x6)
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
SIZE(0x94);

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
