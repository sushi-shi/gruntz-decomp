// GameInfo.h - the saved-game info record CGameInfo (C:\Proj\Gruntz). A non-virtual
// record (no RTTI vtable) whose FormatGameInfoString (0x1183b0, GameInfoString.cpp)
// builds the URL/POST query string and Check1 (0x1182f0) is the ready/dirty gate
// (`return m_8 == 1`). The time sub-object lives at this+0xb8.
//
// CNameRecord (NameRecord.cpp) is a FACET of this same record - it shares the +0x08
// ready flag and calls Check1 on its own `this` (retail 0x1182f0). Its +0x36 tail
// diverges (a single name buffer vs this class's Location/Time/Type split), so the
// two are not yet unified; NameRecord reaches Check1 through the shared facet. Only
// offsets / code bytes are load-bearing.
#ifndef GRUNTZ_GAMEINFO_H
#define GRUNTZ_GAMEINFO_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CGameInfoTime);
struct CGameInfoTime { // this+0xb8 (0x1c bytes; zeroed on a failed validate)
    i32 m_0;           // +0x00 (this+0xb8)
    u32 m_4;           // +0x04 (this+0xbc)  S (seconds, %lu)
    i32 m_8;           // +0x08 (this+0xc0)  timestamp fed to DecodeGameTime
    i32 m_c;           // +0x0c (this+0xc4)  Month
    i32 m_10;          // +0x10 (this+0xc8)  Day
    i32 m_14;          // +0x14 (this+0xcc)  Year
    i32 m_18;          // +0x18 (this+0xd0)
};

SIZE_UNKNOWN(CGameInfo);
class CGameInfo {
public:
    i32 Check1();               // 0x1182f0 __thiscall (ready/dirty gate: return m_8 == 1)
    i32 FormatGameInfoString(); // 0x1183b0

    char m_pad0[0x8];
    u32 m_8; // +0x08  Version (%lu)
    char m_pad0c[0x14 - 0xc];
    char m_14[0x36 - 0x14]; // +0x14  Name buffer
    char m_36[0xb8 - 0x36]; // +0x36  Location buffer
    CGameInfoTime m_b8;     // +0xb8
    u32 m_d4;               // +0xd4  Type (%i)
};

#endif // GRUNTZ_GAMEINFO_H
