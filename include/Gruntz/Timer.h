#ifndef GRUNTZ_GRUNTZ_TIMER_H
#define GRUNTZ_GRUNTZ_TIMER_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>

class CDDrawSurfacePair;

class CTimer {
public:
    CTimer();
    // (originX, originY) - stored straight into m_baseX/m_baseY.
    i32 LoadTimerSprite(i32 originX, i32 originY);
    void Reset();
    i32 Tick(i32 elapsedMs);
    i32 Draw(CDDrawSurfacePair* target, i32 forceVisible);
    void SetTime(i32 minutes, i32 seconds);
    void AddTime(i32 minutes, i32 seconds);
    i32 HandleEvent(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 Serialize(CFileMemBase* ar);
    i32 Deserialize(CFileMemBase* ar);

    i32 m_baseX;
    i32 m_baseY;
    CDDrawWorker* m_sprite;
    i32 m_active;

    CImage* m_frameMinTens;
    CImage* m_frameMinOnes;
    CImage* m_frameSecTens;
    CImage* m_frameSecOnes;
    CImage* m_frameColon;
    char m_pad24[0x28 - 0x24];

    Clock64 m_baseTime;
    Clock64 m_accum;
    Clock64 m_startStamp;
    Clock64 m_unusedStamp; // only 0/-1 sentinel writes; never read
    i32 m_running;
    i32 m_currentMs;
};

#endif // GRUNTZ_GRUNTZ_TIMER_H
