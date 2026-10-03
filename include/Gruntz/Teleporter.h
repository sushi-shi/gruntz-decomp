#ifndef GRUNTZ_CTELEPORTER_H
#define GRUNTZ_CTELEPORTER_H

#include <Ints.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

GZ_ENUM_BEGIN(TeleporterKind)
    TELEPORTER_NORMAL = 0,
    TELEPORTER_SINGLE_USE = 1,
    TELEPORTER_SECRET = 2
GZ_ENUM_END(TeleporterKind)

class CTeleporter : public CUserLogic, public CWapX {
public:
public:
    CTeleporter() {}
    CTeleporter(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    void LoadColors();
    i32 ReapplyConfig();

    i32 Begin();

    i32 Update();

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_TELEPORTER;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    b32 m_armed;

    ClockInterval m_armTiming;
    b32 m_tickHandled;
};

#endif
