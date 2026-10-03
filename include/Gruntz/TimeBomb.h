#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <Ints.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CTimeBomb : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_TIMEBOMB;
    }

public:
    CTimeBomb() {}
    CTimeBomb(CGameObject* obj);
    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();

    i32 UpdateCountdown();

    b32 m_fastPhase;
    ClockInterval m_timing;
};

#endif
