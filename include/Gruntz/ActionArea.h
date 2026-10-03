#ifndef GRUNTZ_CACTIONAREA_H
#define GRUNTZ_CACTIONAREA_H

#include <Ints.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CActionArea : public CUserLogic, public CWapX {
public:
public:
    CActionArea() {}
    CActionArea(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    i32 ApplyColor(i32 owner);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_ACTIONAREA;
    }

    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    i32 Tick();

    i32 m_phase;
    ClockInterval m_timing;
};

#endif
