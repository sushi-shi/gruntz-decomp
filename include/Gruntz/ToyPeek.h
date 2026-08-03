#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic, public CWapX {
public:
    RVA(0x000983e0, 0x98)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        if (Chain(ar, mode, typeId, pObj) == 0) {
            return 0;
        }

        switch (mode) {
            case SERIAL_SAVE:
                ar->Write(&m_startClock, sizeof(m_startClock));
                ar->Write(&m_countdown, sizeof(m_countdown));
                break;
            case SERIAL_LOAD:
                ar->Read(&m_startClock, sizeof(m_startClock));
                ar->Read(&m_countdown, sizeof(m_countdown));
                break;
        }
        return 1;
    }

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
