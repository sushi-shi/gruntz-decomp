#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
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
    CTimeBomb() {}
    CTimeBomb(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 UpdateCountdown();

    b32 m_fastPhase;
    ClockInterval m_timing;
};

#endif // GRUNTZ_CTIMEBOMB_H
