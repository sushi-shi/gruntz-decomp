#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    RVA(0x00011c00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOYPEEK;
    }

    virtual void FireActivation(i32 id) OVERRIDE;

public:
    CToyPeek() {
        m_startClock.m_v = 0;
        m_countdown.m_v = 0;
    }
    CToyPeek(CGameObject* obj);

    char m_pad54[0x58 - 0x54];

    Clock64 m_startClock;
    Clock64 m_countdown;
};

#endif // GRUNTZ_CTOYPEEK_H
