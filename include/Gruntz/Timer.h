#ifndef GRUNTZ_GRUNTZ_TIMER_H
#define GRUNTZ_GRUNTZ_TIMER_H

#include <Ints.h>
#include <Image/CImage.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (HandleEvent/Serialize stream)
#include <Gruntz/Sprite.h>        // CDDrawWorker (the looked-up "GAME_TIMER" sprite set)

class CDDrawSurfacePair;

class CTimer {
public:
    CTimer* Init();
    i32 LoadTimerSprite(i32 a, i32 b);
    void Reset();
    i32 Tick(i32 dt);
    i32 Draw(CDDrawSurfacePair* target, i32 force);
    void SetTime(i32 a, i32 b);
    void AddTime(i32 seconds, i32 minutes);
    i32 HandleEvent(CFileMemBase* ar, i32 kind, i32 a3, i32 a4); // 0x9c1c0
    i32 Serialize(CFileMemBase* ar);                             // 0x9c2e0 (SpriteLoaders cluster)
    i32 Deserialize(CFileMemBase* ar); // 0x9c650 (external, declared-not-defined)

    i32 m_baseX;            // +0x00 base x (screen origin)
    i32 m_baseY;            // +0x04 base y
    CDDrawWorker* m_sprite; // +0x08 the looked-up "GAME_TIMER" sprite set
    i32 m_active;           // +0x0c visible/active flag
    // The five cached MM:SS frames, laid out L->R by Draw at x-0x22..x+0x22 and
    // reassigned per Tick's digit decode: [MinTens][MinOnes][:][SecTens][SecOnes].
    CImage* m_frameMinTens; // +0x10 tens-of-minutes digit frame
    CImage* m_frameMinOnes; // +0x14 units-of-minutes digit frame
    CImage* m_frameSecTens; // +0x18 tens-of-seconds digit frame
    CImage* m_frameSecOnes; // +0x1c units-of-seconds digit frame
    CImage* m_frameColon;   // +0x20 colon frame (static frame 11, drawn centre)
    char m_pad24[0x28 - 0x24];
    i64 m_baseTime; // +0x28 base (limit) time (i64)
    i64 m_accum; // +0x30 accumulated added-time (i64; the 0x8107 cheat zeroes it)
    // +0x38:+0x3c is the level/lap START STAMP - a 64-bit game-clock value held as two
    // dword halves (CGruntzMgr::AccrueScoreTime subtracts the pair from the 64-bit clock
    // with a sub/sbb; CTimer::HandleEvent streams it as one 8-byte field).
    i64 m_38;        // +0x38  level/lap start stamp
    i64 m_40;        // +0x40  (cleared on expiry and by the 0x8107 cheat)
    i32 m_running;   // +0x48 running flag (0x8107 cheat zeroes)
    i32 m_currentMs; // +0x4c decoded current/remaining value (ms within hour;
                     //        0x8107 cheat zeroes)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_TIMER_H
