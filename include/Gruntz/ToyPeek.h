#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;

    RVA(0x00011bf0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOYPEEK;
    }

    virtual void FireActivation(i32 id) OVERRIDE;

public:
    CToyPeek() {}
    CToyPeek(CGameObject* obj);

    char m_pad54[0x58 - 0x54];

    Clock64 m_startClock;
    Clock64 m_countdown;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CTOYPEEK_H
