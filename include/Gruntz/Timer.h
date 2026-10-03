#ifndef GRUNTZ_GRUNTZ_TIMER_H
#define GRUNTZ_GRUNTZ_TIMER_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>

class CDDrawSurfacePair;

class CTimer {
public:
    CTimer();
    i32 LoadTimerSprite(i32 originX, i32 originY);
    void Reset();
    i32 Tick(i32 elapsedMs);
    i32 Draw(CDDrawSurfacePair* target, b32 forceVisible);
    void SetTime(i32 minutes, i32 seconds);
    void AddTime(i32 minutes, i32 seconds);
    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);

    void Stop() {
        m_stamp.m_intervalLo = 0;
        m_stamp.m_intervalHi = 0;
        m_countdown.m_intervalLo = 0;
        m_countdown.m_intervalHi = 0;
        m_running = false;
        m_currentMs = 0;
    }

    Coord m_basePosition;
    CDDrawWorker* m_sprite;
    b32 m_active;

    CImage* m_frameMinTens;
    CImage* m_frameMinOnes;
    CImage* m_frameSecTens;
    CImage* m_frameSecOnes;
    CImage* m_frameColon;

    ClockInterval m_countdown;
    ClockInterval m_stamp; // interval: only 0/-1 sentinel writes; never read
    b32 m_running;
    i32 m_currentMs;
};

#define RESET_TIMER_SPRITES                                                                        \
    m_sprite = NULL;                                                                               \
    m_frameMinTens = NULL;                                                                         \
    m_frameMinOnes = NULL;                                                                         \
    m_frameColon = NULL;                                                                           \
    m_frameSecTens = NULL;                                                                         \
    m_frameSecOnes = NULL;                                                                         \
    m_active = false

#endif // GRUNTZ_GRUNTZ_TIMER_H
