#ifndef GRUNTZ_CTELEPORTER_H
#define GRUNTZ_CTELEPORTER_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/LogicTypeId.h>
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
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;

    i32 m_armed;

    i64 m_armClock;
    i64 m_interval;
    i32 m_tickHandled;
    char m_pad6c[0x70 - 0x6c];
};
SIZE(0x70);

SIZE_UNKNOWN();

#endif // GRUNTZ_CTELEPORTER_H
