#ifndef GRUNTZ_CPATHHAZARD_H
#define GRUNTZ_CPATHHAZARD_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

struct CPathWaypoint {
    i32 x;
    i32 y;
};
SIZE_UNKNOWN();

struct CHazardTimer {
    i64 m_deadline;
    i64 m_window;
    CHazardTimer() : m_deadline(0), m_window(0) {}
};
SIZE_UNKNOWN();

#include <Rez/FrameClock.h>

class CPathHazard : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    virtual void FireActivation(i32 id) OVERRIDE;

public:
    RVA(0x00013170, 0x7b)
    CPathHazard() {}
    CPathHazard(CGameObject* obj);

    RVA(0x00013210, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PATHHAZARD;
    }

    virtual i32 Tick();
    virtual i32 SiblingTick();

    virtual i32 Arrive();

    virtual i32 BeginLeg();

    RVA(0x00013230, 0x8)
    virtual i32 HitTest(i32, i32) {
        return 1;
    }

    i32 ForwardTick();
    i32 ForwardSiblingTick();

    char m_pad54[0x58 - 0x54];
    double m_speed;
    double m_posX;
    double m_posY;
    double m_unitX;
    double m_unitY;
    double m_roundBiasX;
    double m_roundBiasY;
    CPathWaypoint m_wp[13];
    i32 m_wpIndex;
    i32 m_wpX;
    i32 m_wpY;
    i32 m_wpCount;

    CHazardTimer m_leg;
    i32 m_strikeArmed;
    char m_pad11c[0x120 - 0x11c];
    CHazardTimer m_strike;
};
SIZE(0x130);

#endif // GRUNTZ_CPATHHAZARD_H
