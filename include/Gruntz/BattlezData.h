// BattlezData.h - CBattlezData, the per-game multiplayer (Battlez) progress /
// score tracker held at CGameReg+0x7c (g_gameReg->m_7c). The game manager
// new[]s a 0x388-byte instance during Init (push 0x388; call new) and runs the
// ctor on it; LoadLevelByMode re-runs the ctor each level. The methods cluster
// around two 4x4 int grids and an array of per-map records:
//
//   +0x00  m_records : pointer to an array of m_04 records, 0x40 bytes each
//                      (each record's win/score field at +0x28). Init binds it
//                      to a level-table slice ([gameReg.m_58]+0x24).
//   +0x04  m_count   : number of records (a map count, range-checked to <=0x24)
//   +0x08            : spare (zeroed, serialized)
//   +0x0c  m_allDone : "every record in the current group of 4 is populated" flag
//   +0x10..+0x44     : 14 scalar progress/bound fields (serialized one-by-one).
//                      0fcd70 gates on m_scoreValue then compares the m_30..m_40 band
//                      against the m_14..m_2c band (a "within bounds" test).
//   +0x48  m_48[4]   : a 4-int scalar band (serialized as a run of 4).
//   +0x58  m_wins[16]: 4x4 head-to-head win matrix. 0fcc50 bumps [y][x] for
//                      y!=x; 0fccb0 sums row y; 0fcc90 clears it.
//   +0x98  m_flags[16]: 4x4 flag matrix. 0fcb50 sets [y][x]=1; 0fcbc0 sums all
//                      16; 0fcc10 reads [x][y]; 0fcb90 clears it.
//   +0xd8  m_band_d8[88], +0x238 m_band_238[40], +0x2d8 m_band_2d8[28],
//          +0x348 m_band_348[16] : large zeroed/serialized scalar bands.
//
// Non-polymorphic (no vptr, no RTTI). Field names are placeholders; only the
// offsets + code bytes are load-bearing.
#ifndef GRUNTZ_BATTLEZDATA_H
#define GRUNTZ_BATTLEZDATA_H

#include <Ints.h>

#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)

// The per-map record the m_records array points at (0x40 bytes / record): 16
// ints mirroring the owner's m_10..m_scoreValue progress/bound band (see FillRecord's
// rec[0..15] stores). +0x00 = populated flag; +0x28 = win/score value. The
// proximity accessors below sum/test these per-record fields over the records
// in the "current group of 4" (index (m_count-1)/4*4) or over all 0x20.
struct BattlezRecord {
    i32 m_populated; // +0x00  populated flag (0fcad0 tests, 0fd330 sets to 1)
    i32 m_04;        // +0x04  from gameReg.m_118 (0fd330)
    i32 m_08, m_0c, m_10, m_14, m_18, m_1c, m_20, m_24;
    i32 m_scoreValue; // +0x28  win/score value (0fced0 returns; 0fd330 fills)
    i32 m_2c, m_30, m_34, m_38, m_3c;
};

// The serialization sink passed to Serialize is the shared WAP32 CSerialArchive
// (Read @ vtable +0x2c / Write @ +0x30), now the one modeled class in
// <Gruntz/SerialArchive.h> - the former local `BattlezStream` view is folded away.

class CBattlezData {
public:
    i32 InitWithRecords(void* records);                       // 0xfc9c0
    void Init();                                              // 0xfca10
    void SetCount(i32 count);                                 // 0xfcad0
    void MarkFlag(i32 y, i32 x);                              // 0xfcb50
    void ClearFlags();                                        // 0xfcb90
    i32 SumFlags(i32 y);                                      // 0xfcbc0
    i32 GetFlag(i32 x, i32 y);                                // 0xfcc10
    void BumpWin(i32 y, i32 x);                               // 0xfcc50
    void ClearWins();                                         // 0xfcc90
    i32 SumWinRow(i32 y);                                     // 0xfccb0
    i32 InBounds(i32 unused);                                 // 0xfcd70
    i32 AllRecordsInBounds();                                 // 0xfccf0
    float GroupRatio();                                       // 0xfce00
    i32 GroupAllScored();                                     // 0xfce80
    i32 SumGroupField0c();                                    // 0xfcf20
    i32 SumGroupField2c();                                    // 0xfcf70
    i32 SumGroupField10();                                    // 0xfcfc0
    i32 SumGroupField30();                                    // 0xfd010
    i32 SumGroupField1c();                                    // 0xfd060
    i32 SumGroupField34();                                    // 0xfd0b0
    i32 SumGroupField20();                                    // 0xfd100
    i32 SumGroupField38();                                    // 0xfd150
    i32 SumGroupField24();                                    // 0xfd1a0
    i32 SumGroupField3c();                                    // 0xfd1f0
    i32 SumGroupField18();                                    // 0xfd240
    i32 SumGroupField14();                                    // 0xfd290
    i32 SumGroupField08();                                    // 0xfd2e0
    i32 GetRecordValue(i32 b);                                // 0xfced0
    void FillRecord(i32 index, i32 phase);                    // 0xfd330
    i32 Serialize(CSerialArchive* s, i32 op, i32 a2, i32 a3); // 0xfd3f0

    BattlezRecord* m_records;               // +0x00
    i32 m_count;                            // +0x04
    i32 m_08;                               // +0x08
    i32 m_allDone;                          // +0x0c
    i32 m_10, m_14, m_18, m_1c, m_20, m_24; // +0x10..
    i32 m_28, m_2c, m_30, m_34, m_38, m_3c, m_40, m_scoreValue;
    i32 m_48[4];        // +0x48
    i32 m_wins[16];     // +0x58  4x4
    i32 m_flags[16];    // +0x98  4x4
    i32 m_band_d8[88];  // +0xd8
    i32 m_band_238[40]; // +0x238
    i32 m_band_2d8[28]; // +0x2d8
    i32 m_band_348[16]; // +0x348
};

#endif // GRUNTZ_BATTLEZDATA_H
