#include <Mfc.h>  // real MFC CTime (GetCurrentTime / GetLocalTm) - the BuildGameDate clock
#include <time.h> // struct tm (GetLocalTm's return record)
#include <rva.h>
#include <stdio.h>           // sprintf (reloc-masked)
#include <string.h>          // strlen/strcat/memset (inlined /O2)
#include <Gruntz/GameInfo.h> // the shared CGameInfo / CGameInfoTime record (NameRecord shares it)

i32 ValidateGameTime(CGameInfoTime* t);                  // 0x118310
void SplitMillisToHMS(u32 n, u32* hh, u32* mm, u32* ss); // 0x119210
DATA(0x0024ebf8)
char g_infoScratch[0x100] = {0}; // 0x64ebf8  per-piece scratch
DATA(0x0024ecf8)
char g_infoMaster[0x800] = {0}; // 0x64ecf8  query accumulator

RVA(0x001182f0, 0xc)
i32 CGameInfo::Check1() {
    return m_8 == 1;
}

RVA(0x00118310, 0xc)
i32 ValidateGameTime(CGameInfoTime* t) {
    return t != 0;
}

// ---------------------------------------------------------------------------
// BuildGameDate (0x118330; RVA-homed from src/Stub/BoundaryTail.cpp) - fill a
// calendar-date record from the current local time: sample CTime::GetCurrentTime()
// then read its broken-down struct tm three times (CTime::GetLocalTm @0x1b30f0), so
// month = tm_mon+1, day = tm_mday, year = tm_year+1900. Fails (0) if out is null.
// __cdecl, 1 stack arg. The out record's +0xc/+0x10/+0x14 IS CGameInfoTime's
// Month/Day/Year, so it is typed there.
// ORPHAN: only caller is a boundarylowermethods placeholder (C1181d0::Update).
// @early-stop
// regalloc/scheduling wall (~92%): the logic + the reloc-masked GetCurrentTime/
// GetLocalTm calls are byte-faithful, but retail keeps each tm field in eax (reusing
// the GetLocalTm return reg) and defers the out-store past the next call's lea-ecx
// setup, while our /O2 stages it in ecx/edx and stores before the lea.
RVA(0x00118330, 0x57)
i32 BuildGameDate(CGameInfoTime* out) {
    if (out == 0) {
        return 0;
    }
    CTime now = CTime::GetCurrentTime();
    out->m_c = now.GetLocalTm(0)->tm_mon + 1;
    out->m_10 = now.GetLocalTm(0)->tm_mday;
    out->m_14 = now.GetLocalTm(0)->tm_year + 1900;
    return 1;
}

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

    u32 a = 0, b = 0, c = 0;
    SplitMillisToHMS(t->m_8, &a, &b, &c);
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
