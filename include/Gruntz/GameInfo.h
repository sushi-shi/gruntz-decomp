#ifndef GRUNTZ_GAMEINFO_H
#define GRUNTZ_GAMEINFO_H

#include <Ints.h>
#include <rva.h>

struct CGameInfoTime { // this+0xb8 (0x1c bytes; zeroed on a failed validate)
    i32 m_0;           // +0x00 (this+0xb8)
    u32 m_4;           // +0x04 (this+0xbc)  S (seconds, %lu)
    u32 m_8;           // +0x08 (this+0xc0)  timestamp (UNSIGNED: Update/CopyIfLarger compare
                       //        it unsigned + it feeds SplitMillisToHMS(u32))
    i32 m_c;           // +0x0c (this+0xc4)  Month
    i32 m_10;          // +0x10 (this+0xc8)  Day
    i32 m_14;          // +0x14 (this+0xcc)  Year
    i32 m_18;          // +0x18 (this+0xd0)
};
SIZE_UNKNOWN();

class CGameInfo {
public:
    // The name/record setters (NameRecord.cpp; __thiscall, RVA-bound).
    i32 SetNames(char* name, char* name2, i32 unused); // 0x118040 (set Name + secondary/Location)
    i32 CopyBody(char* body);                          // 0x118130 (deserialize the 212-byte body)
    i32 Update(i32 s, i32 timestamp, i32 type);     // 0x1181d0 (set the time box + Type if newer)
    i32 CopyIfLarger(CGameInfoTime* src, i32 type); // 0x118260 (copy the time box if newer + Type)
    i32 Check1();               // 0x1182f0 __thiscall (ready/dirty gate: return m_8 == 1)
    i32 FormatGameInfoString(); // 0x1183b0

    char m_00[4]; // +0x00
    i32 m_04;     // +0x04  head of the 212-byte body SetNames zeroes / CopyBody fills
    u32 m_8;      // +0x08  Version (%lu) / ready flag (== 1)
    char m_pad0c[0x14 - 0xc];
    char m_14[0x36 - 0x14]; // +0x14  Name buffer (<= 16 chars)
    char m_36[0xb8 - 0x36]; // +0x36  Location buffer / SetNames secondary string
    CGameInfoTime m_b8;     // +0xb8
    u32 m_d4;               // +0xd4  Type (%i)
};
SIZE_UNKNOWN();

i32 BuildGameDate(CGameInfoTime* out); // 0x118330

#endif // GRUNTZ_GAMEINFO_H
