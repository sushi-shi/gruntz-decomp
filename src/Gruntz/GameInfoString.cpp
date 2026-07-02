// GameInfoString.cpp - CGameInfo::FormatGameInfoString (0x1183b0), re-homed from
// src/Stub/Backlog.cpp. Builds a URL/POST query string describing a saved game into
// the global accumulator g_infoMaster, by sprintf'ing each piece into g_infoScratch
// and strcat-appending it, then URL-encodes spaces to '+'. Frameless __thiscall on
// the save-info record; the time sub-object lives at this+0xb8. CGameInfo is the
// recovered/placeholder identity (no RTTI vtable; a non-virtual record - Ghidra shows
// no static callers, the class is inferred from its self-consistent field/method set).
// Only offsets / code bytes are load-bearing; sprintf/strcat/strlen/memset are
// reloc-masked CRT intrinsics and Check1/Validate/DecodeGameTime are reloc-masked
// engine helpers.
#include <rva.h>
#include <stdio.h>  // sprintf (reloc-masked)
#include <string.h> // strlen/strcat/memset (inlined /O2)

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
// FUN_00118310 __cdecl(&time) -> validity flag; FUN_00119210 __cdecl(ts, &a, &b, &c)
// -> decompose the timestamp into three out-values.
i32 ValidateGameTime(CGameInfoTime* t);                       // 0x118310
void DecodeGameTime(i32 ts, i32* outA, i32* outB, i32* outC); // 0x119210
SIZE_UNKNOWN(CGameInfo);
class CGameInfo {
public:
    i32 Check1();               // FUN_001182f0 __thiscall (ready/dirty gate)
    i32 FormatGameInfoString(); // 0x1183b0

    char m_pad0[0x8];
    u32 m_8; // +0x08  Version (%lu)
    char m_pad0c[0x14 - 0xc];
    char m_14[0x36 - 0x14]; // +0x14  Name buffer
    char m_36[0xb8 - 0x36]; // +0x36  Location buffer
    CGameInfoTime m_b8;     // +0xb8
    u32 m_d4;               // +0xd4  Type (%i)
};
DATA(0x0024ecf8)
extern char g_infoMaster[0x800]; // 0x64ecf8  query accumulator
DATA(0x0024ebf8)
extern char g_infoScratch[0x100]; // 0x64ebf8  per-piece scratch

RVA(0x001183b0, 0x211)
i32 CGameInfo::FormatGameInfoString() {
    char* name = m_14;
    if (name == 0) {
        return 0;
    }
    if (strlen(name) == 0) {
        return 0;
    }
    if (!Check1()) {
        return 0;
    }

    g_infoMaster[0] = 0;
    sprintf(g_infoScratch, "Name=%s&Type=%i&Location=%s&Version=%lu", name, m_d4, m_36, m_8);
    strcat(g_infoMaster, g_infoScratch);

    CGameInfoTime* t = &m_b8;
    if (t == 0) {
        return 0;
    }
    if (!ValidateGameTime(t)) {
        memset(t, 0, 28);
    }

    i32 a = 0, b = 0, c = 0;
    DecodeGameTime(t->m_8, &a, &b, &c);
    sprintf(g_infoScratch, "&S=%lu&H=%i&M=%02i&SE=%02i", t->m_4, a, b, c);
    strcat(g_infoMaster, g_infoScratch);

    sprintf(g_infoScratch, "&Month=%i&Day=%i&Year=%i", t->m_c, t->m_10, t->m_14);
    strcat(g_infoMaster, g_infoScratch);

    i32 chk = (69 * (b * a) + 1) * c + b + a + t->m_c + t->m_14 + t->m_10 + t->m_4;
    sprintf(g_infoScratch, "&Checksum=%lu", chk);
    strcat(g_infoMaster, g_infoScratch);

    if (g_infoMaster[0] != 0) {
        for (char* p = g_infoMaster; *p != 0; p++) {
            if (*p == ' ') {
                *p = '+';
            }
        }
    }
    return 0;
}
