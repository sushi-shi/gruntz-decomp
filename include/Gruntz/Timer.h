#ifndef GRUNTZ_GRUNTZ_TIMER_H
#define GRUNTZ_GRUNTZ_TIMER_H

#include <Ints.h>
#include <Image/CImage.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (HandleEvent/Serialize stream)
#include <Gruntz/Sprite.h>        // CSprite (the looked-up "GAME_TIMER" sprite set)

SIZE_UNKNOWN(CTimer);
class CTimer {
public:
    CTimer* Init();
    i32 LoadTimerSprite(i32 a, i32 b);
    void Reset();
    i32 Tick(i32 dt);
    i32 Draw(i32 x, i32 pSurf);
    void SetTime(i32 a, i32 b);
    void AddTime(i32 seconds, i32 minutes);
    i32 HandleEvent(CSerialArchive* ar, i32 kind, i32 a3, i32 a4); // 0x9c1c0
    i32 Serialize(CSerialArchive* ar);   // 0x9c2e0 (SpriteLoaders cluster)
    i32 Deserialize(CSerialArchive* ar); // 0x9c650 (external, declared-not-defined)

    i32 m_baseX;       // +0x00 base x (screen origin)
    i32 m_baseY;       // +0x04 base y
    CSprite* m_sprite; // +0x08 the looked-up "GAME_TIMER" sprite set
    i32 m_active;      // +0x0c visible/active flag
    // The five cached MM:SS frames, laid out L->R by Draw at x-0x22..x+0x22 and
    // reassigned per Tick's digit decode: [MinTens][MinOnes][:][SecTens][SecOnes].
    CImage* m_frameMinTens; // +0x10 tens-of-minutes digit frame
    CImage* m_frameMinOnes; // +0x14 units-of-minutes digit frame
    CImage* m_frameSecTens; // +0x18 tens-of-seconds digit frame
    CImage* m_frameSecOnes; // +0x1c units-of-seconds digit frame
    CImage* m_frameColon;   // +0x20 colon frame (static frame 11, drawn centre)
    char m_pad24[0x28 - 0x24];
    i32 m_baseTimeLo; // +0x28 base (limit) time lo
    i32 m_baseTimeHi; // +0x2c base (limit) time hi
    i32 m_accumLo;    // +0x30 accumulated added-time lo (0x8107 cheat zeroes)
    i32 m_accumHi;    // +0x34 accumulated added-time hi (0x8107 cheat zeroes)
    // +0x38:+0x3c is the level/lap START STAMP - a 64-bit game-clock value held as two
    // dword halves (CGruntzMgr::AccrueScoreTime subtracts the pair from the 64-bit clock
    // with a sub/sbb; CTimer::HandleEvent streams it as one 8-byte field). It stays two
    // i32s because CTimer::Init INTERLEAVES the halves with the +0x40 pair
    // (m_38, m_40, m_3c, m_44) - a single i64 member cannot emit that store order. Read
    // it 64-bit the way the +0x30 pair already is: `*reinterpret_cast<i64*>(&t)->m_38`.
    i32 m_38;        // +0x38  level/lap start stamp, lo
    i32 m_3c;        // +0x3c  ... hi
    i32 m_40;        // +0x40  (serialized 64-bit pair m_40:m_44; cleared on expiry
    i32 m_44;        // +0x44   and by the 0x8107 cheat)
    i32 m_running;   // +0x48 running flag (0x8107 cheat zeroes)
    i32 m_currentMs; // +0x4c decoded current/remaining value (ms within hour;
                     //        0x8107 cheat zeroes)
};

#endif // GRUNTZ_GRUNTZ_TIMER_H
