#ifndef GRUNTZ_CPATHHAZARD_H
#define GRUNTZ_CPATHHAZARD_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

struct CPathWaypoint {
    i32 m_x;
    i32 m_y;
};

#include <Rez/FrameClock.h>

class CPathHazard : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;

public:
    CPathHazard();
    CPathHazard(CUserLogic::EInlineBase) {}
    CPathHazard(CGameObject* obj);

    RVA(0x00013210, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PATHHAZARD;
    }

    virtual i32 UpdateMovement();
    virtual i32 UpdateWaypointPause();

    virtual i32 AdvanceWaypoint();

    virtual i32 StartWaypointMovement();

    RVA(0x00013230, 0x8)
    virtual i32 OnGruntContact(i32 playerIndex, i32 unitIndex) {
        return 1;
    }

    i32 HandleMovementAct();
    i32 HandlePauseAct();

    double m_speed;
    double m_posX;
    double m_posY;
    double m_unitX;
    double m_unitY;
    double m_roundBiasX;
    double m_roundBiasY;
    CPathWaypoint m_waypoints[13];
    i32 m_waypointIndex;
    i32 m_targetX;
    i32 m_targetY;
    i32 m_waypointCount;

    ClockInterval m_waypointPauseTimer;
    b32 m_flashActive;
    ClockInterval m_flashTimer;
};

#endif // GRUNTZ_CPATHHAZARD_H
