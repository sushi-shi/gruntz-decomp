#ifndef GRUNTZ_BATTLEZDATA_H
#define GRUNTZ_BATTLEZDATA_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)

struct BattlezRecord {
    i32 m_populated; // +0x00  populated flag (0fcad0 tests, 0fd330 sets to 1)
    i32 m_04;        // +0x04  from gameReg.m_118 (0fd330)
    i32 m_08, m_0c, m_10, m_14, m_18, m_1c, m_20, m_24;
    i32 m_scoreValue; // +0x28  win/score value (0fced0 returns; 0fd330 fills)
    i32 m_2c, m_30, m_34, m_38, m_3c;
};
SIZE_UNKNOWN();

class CBattlezData {
public:
    i32 InitWithRecords(void* records);    // 0xfc9c0
    ~CBattlezData();                       // 0xfc9f0
    void Init();                           // 0xfca10
    void SetCount(i32 count);              // 0xfcad0
    void MarkFlag(i32 y, i32 x);           // 0xfcb50
    void ClearFlags();                     // 0xfcb90
    i32 SumFlags(i32 y);                   // 0xfcbc0
    i32 GetFlag(i32 x, i32 y);             // 0x0fcc10 (out-of-line: bounds-checked m_flags[x][y])
    void BumpWin(i32 y, i32 x);            // 0xfcc50
    void ClearWins();                      // 0xfcc90
    i32 SumWinRow(i32 y);                  // 0xfccb0
    i32 InBounds(i32 unused);              // 0xfcd70
    i32 AllRecordsInBounds();              // 0xfccf0
    float GroupRatio();                    // 0xfce00
    i32 GroupAllScored();                  // 0xfce80
    i32 SumGroupField0c();                 // 0xfcf20
    i32 SumGroupField2c();                 // 0xfcf70
    i32 SumGroupField10();                 // 0xfcfc0
    i32 SumGroupField30();                 // 0xfd010
    i32 SumGroupField1c();                 // 0xfd060
    i32 SumGroupField34();                 // 0xfd0b0
    i32 SumGroupField20();                 // 0xfd100
    i32 SumGroupField38();                 // 0xfd150
    i32 SumGroupField24();                 // 0xfd1a0
    i32 SumGroupField3c();                 // 0xfd1f0
    i32 SumGroupField18();                 // 0xfd240
    i32 SumGroupField14();                 // 0xfd290
    i32 SumGroupField08();                 // 0xfd2e0
    i32 GetRecordValue(i32 b);             // 0xfced0
    void FillRecord(i32 index, i32 phase); // 0xfd330
    i32 Serialize(CFileMemBase* s, i32 op, i32 a2, i32 a3); // 0xfd3f0

    BattlezRecord* m_records; // +0x00
    i32 m_count;              // +0x04
    i32 m_08;                 // +0x08
    i32 m_allDone;            // +0x0c
    // +0x10 / +0x48: names migrated from the now-deleted CTmScoreBoard fake view
    // (TriggerMgrViews.h), which modeled this same +0x7c object at the same offsets.
    i32 m_score;       // +0x10  score accumulator
    i32 m_toyzCount;   // +0x14  toyz picked up (LoadPickupSprites bumps; HUD stat)
    i32 m_weaponCount; // +0x18  weaponz picked up (WARPSTONE excluded)
    i32 m_1c, m_20;
    i32 m_powerupCount; // +0x24  powerupz picked up
    i32 m_28, m_2c, m_30, m_34, m_38, m_3c, m_40, m_scoreValue;
    i32 m_counts[4]; // +0x48  per-row placed-object counters
    i32 m_wins[16];  // +0x58  4x4
    i32 m_flags[16]; // +0x98  4x4
    // Per-owner x per-pickup-type counters (LoadPickupSprites bumps
    // [N*owner + (type - band-base-type)]; Serialize round-trips each as 4 x N).
    // The retail displacements fold the PickupType id base into the array base:
    // 0xd8 == 0xd4 + 4*PICKUP_BOMB, 0x238 == 0x1dc + 4*PICKUP_BABYWALKER,
    // 0x2d8 == 0x200 + 4*PICKUP_GHOST, 0x348 == 0x254 + 4*PICKUP_RANDOMCOLORZ,
    // and 0x348 + 4*4*4 == 0x388 == sizeof(CBattlezData) (the `new` size) exactly.
    i32 m_weaponPickupz[88];  // +0xd8   [22*owner + (type - PICKUP_BOMB)]
    i32 m_toyPickupz[40];     // +0x238  [10*owner + (type - PICKUP_BABYWALKER)]
    i32 m_powerupPickupz[28]; // +0x2d8  [7*owner + (type - PICKUP_GHOST)]
    i32 m_miscPickupz[16];    // +0x348  [4*owner + (type - PICKUP_RANDOMCOLORZ)]
};
SIZE_UNKNOWN();

extern float g_zeroF;
#endif // GRUNTZ_BATTLEZDATA_H
