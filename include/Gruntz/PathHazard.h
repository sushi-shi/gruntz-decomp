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
    RVA(0x000b4d30, 0x287)
    virtual i32 SerializeMove(CFileMemBase* stream, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        CFileMemBase* s = stream;
        if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
            return 0;
        }
        if (Chain(static_cast<CFileMemBase*>(stream), tag, c, d) == 0) {
            return 0;
        }
        SerQuadPair(s, tag, &m_leg);
        SerQuadPair(s, tag, &m_strike);
        if (tag != SERIAL_SAVE) {
            if (tag == SERIAL_LOAD) {
                s->Read(&m_speed, 8);
                s->Read(&m_posX, 8);
                s->Read(&m_posY, 8);
                s->Read(&m_unitX, 8);
                s->Read(&m_unitY, 8);
                s->Read(&m_roundBiasX, 8);
                s->Read(&m_roundBiasY, 8);
                CPathWaypoint* p = m_wp;
                i32 n = 13;
                do {
                    s->Read(p, 8);
                    p += 1;
                } while (--n != 0);
                s->Read(&m_wpIndex, 4);
                s->Read(&m_wpX, 4);
                s->Read(&m_wpY, 4);
                s->Read(&m_wpCount, 4);
                s->Read(&m_strikeArmed, 4);
            }
        } else {
            s->Write(&m_speed, 8);
            s->Write(&m_posX, 8);
            s->Write(&m_posY, 8);
            s->Write(&m_unitX, 8);
            s->Write(&m_unitY, 8);
            s->Write(&m_roundBiasX, 8);
            s->Write(&m_roundBiasY, 8);
            CPathWaypoint* p = m_wp;
            i32 n = 13;
            do {
                s->Write(p, 8);
                p += 1;
            } while (--n != 0);
            s->Write(&m_wpIndex, 4);
            s->Write(&m_wpX, 4);
            s->Write(&m_wpY, 4);
            s->Write(&m_wpCount, 4);
            s->Write(&m_strikeArmed, 4);
        }
        return 1;
    }

    virtual void FireActivation(i32 id) OVERRIDE;

public:
    CPathHazard();
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
