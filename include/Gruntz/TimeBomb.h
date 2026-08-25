#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CTimeBomb : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00012a20, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TIMEBOMB;
    }

public:
    CTimeBomb() {
        m_startTime = 0;
        m_duration = 0;
    }
    CTimeBomb(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 UpdateCountdown();

    i32 m_fastPhase;
    union {
        struct {
            i64 m_startTime;
            i64 m_duration;
        };
        CPairRecord m_timing;
    };
};

#endif // GRUNTZ_CTIMEBOMB_H
