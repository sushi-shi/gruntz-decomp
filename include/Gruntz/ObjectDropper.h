#ifndef GRUNTZ_COBJECTDROPPER_H
#define GRUNTZ_COBJECTDROPPER_H

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

GZ_ENUM_BEGIN(ObjectDropScope)
    OBJECT_DROP_ALL_PLAYERS = 0,
    OBJECT_DROP_PLAYER_ZERO_ONLY = 1
GZ_ENUM_END(ObjectDropScope)

class CObjectDropper : public CUserLogic, public CWapX {
public:
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_OBJECTDROPPER;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
    CObjectDropper() {}
    CObjectDropper(CGameObject* obj);

    i32 Update();
    virtual void FireActivation(i32 id)  ;

    static void RegisterActs();

    double m_speed;
    double m_posX;
    double m_posY;
    i32 m_travelDx;
    i32 m_travelDy;
    i32 m_lastDropPlayerIndex;
    i32 m_lastDropUnitIndex;
    ObjectDropScope m_scrollMode;
    ClockInterval m_dropTiming;
};

#endif
