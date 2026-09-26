#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    RVA(0x00011bf0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOYPEEK;
    }

    virtual void FireActivation(i32 id) OVERRIDE;

public:
    CToyPeek() {}
    CToyPeek(CGameObject* obj);

    ClockInterval m_countdownTiming;
};

#endif // GRUNTZ_CTOYPEEK_H
