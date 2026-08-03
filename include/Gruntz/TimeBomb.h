#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CTimeBomb : public CUserLogic, public CWapX {
public:
    RVA(0x000e2080, 0xc1)
    virtual i32
    SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (g_gameReg->m_world == 0) {
            return 0;
        }
        CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
        switch (mode) {
            case SERIAL_LOAD:
                sa->Read(&m_startTime, 8);
                sa->Read(&m_duration, 8);
                break;
            case SERIAL_SAVE:
                sa->Write(&m_startTime, 8);
                sa->Write(&m_duration, 8);
                break;
        }
        switch (mode) {
            case SERIAL_LOAD:
                sa->Read(&m_fastPhase, 4);
                break;
            case SERIAL_SAVE:
                sa->Write(&m_fastPhase, 4);
                break;
        }
        if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(sa, mode, typeId, pObj) ? 1 : 0;
    }
    RVA(0x00012a20, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TIMEBOMB;
    }

public:
    CTimeBomb() {}
    CTimeBomb(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 LoadAttributes();

    i32 m_fastPhase;
    union {
        struct {
            i64 m_startTime;
            i64 m_duration;
        };
        CPairRecord m_timing;
    };
};
SIZE(0x68);

#endif // GRUNTZ_CTIMEBOMB_H
