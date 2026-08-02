#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/SerialRecords.h>

class CTimeBomb : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
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
