#ifndef GRUNTZ_GRUNTZ_TIMER_H
#define GRUNTZ_GRUNTZ_TIMER_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>

class CDDrawSurfacePair;

class CLevelTimer {
public:
    CLevelTimer();
    i32 LoadTimerSprite(i32 originX, i32 originY);
    void Reset();
    i32 Tick(i32 elapsedMs);
    i32 Draw(CDDrawSurfacePair* target, b32 forceVisible);
    void SetTime(i32 minutes, i32 seconds);
    void AddTime(i32 minutes, i32 seconds);
    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);

    void Start() {
        m_stamp.m_interval = 0xffffffff;
        if (m_remainingMs != 0) {
            m_running = true;
            m_stamp.m_start = static_cast<u32>(g_frameTime);
            m_countdown.Start(m_remainingMs);
        } else {
            m_stamp.m_start = static_cast<u32>(g_frameTime);
        }
    }

    void Stop() {
        m_stamp.m_intervalLo = 0;
        m_stamp.m_intervalHi = 0;
        m_countdown.m_intervalLo = 0;
        m_countdown.m_intervalHi = 0;
        m_running = false;
        m_remainingMs = 0;
    }

    i32 m_baseX;
    i32 m_baseY;
    CImageSet* m_sprite;
    b32 m_active;

    CImage* m_frameMinTens;
    CImage* m_frameMinOnes;
    CImage* m_frameSecTens;
    CImage* m_frameSecOnes;
    CImage* m_frameColon;

    ClockInterval m_countdown;
    ClockInterval m_stamp; // interval: only 0/-1 sentinel writes; never read
    b32 m_running;
    i32 m_remainingMs;
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
