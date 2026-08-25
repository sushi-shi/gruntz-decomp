#ifndef GRUNTZ_CTELEPORTER_H
#define GRUNTZ_CTELEPORTER_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

// The Teleporter WWD object's Smarts field selects its lifetime and destination
// behavior. The shared WWD field remains raw because other logic classes give
// Smarts unrelated meanings.
GZ_ENUM_BEGIN(TeleporterKind)
    TELEPORTER_NORMAL = 0,
    TELEPORTER_SINGLE_USE = 1,
    TELEPORTER_SECRET = 2
GZ_ENUM_END(TeleporterKind)

class CTeleporter : public CUserLogic, public CWapX {
public:
public:
    CTeleporter() {
        m_armClock = 0;
        m_interval = 0;
    }
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
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    i32 m_armed;

    i64 m_armClock;
    i64 m_interval;
    i32 m_tickHandled;
    char m_pad6c[0x70 - 0x6c];
};

#endif // GRUNTZ_CTELEPORTER_H
