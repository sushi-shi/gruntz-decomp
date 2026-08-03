#ifndef GRUNTZ_CTELEPORTER_H
#define GRUNTZ_CTELEPORTER_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

extern "C" u32 g_engineFrameDelta;

extern "C" u32 g_frameTime;

class CTeleporter : public CUserLogic, public CWapX {
public:
public:
    CTeleporter() {}
    CTeleporter(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    void LoadColors();
    i32 ReapplyConfig();

    i32 Begin();

    i32 Update();

    RVA(0x00010d80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TELEPORTER;
    }
    RVA(0x00041350, 0xee)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }
        if (tag != SERIAL_SAVE) {
            if (tag == SERIAL_LOAD) {
                ar->Read(&m_armClock, 8);
                ar->Read(&m_interval, 8);
            }
        } else {
            ar->Write(&m_armClock, 8);
            ar->Write(&m_interval, 8);
        }
        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_armed, 4);
                ar->Write(&m_tickHandled, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_armed, 4);
                ar->Read(&m_tickHandled, 4);
                break;
            case SERIAL_POSTLOAD:
                LoadColors();
                break;
        }
        return 1;
    }

    i32 m_armed;

    i64 m_armClock;
    i64 m_interval;
    i32 m_tickHandled;
    char m_pad6c[0x70 - 0x6c];
};
SIZE(0x70);

SIZE_UNKNOWN();

#endif // GRUNTZ_CTELEPORTER_H
