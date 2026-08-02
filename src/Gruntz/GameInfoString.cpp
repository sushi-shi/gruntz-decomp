#include <Gruntz/GameInfoString.h>
#include <Mfc.h>
#include <time.h>
#include <rva.h>
#include <stdio.h>
#include <string.h>
#include <Gruntz/GameInfo.h>

DATA(0x0024ebf8)
char g_infoScratch[0x100] = {0};
DATA(0x0024ecf8)
char g_infoMaster[0x800] = {0};

RVA(0x001182f0, 0xc)
i32 CGameInfo::HasSupportedVersion() {
    return m_body.m_version == 1;
}

RVA(0x00118310, 0xc)
i32 ValidateGameTime(CGameInfoTime* t) {
    return t != 0;
}

// @early-stop
RVA(0x00118330, 0x57)
i32 BuildGameDate(CGameInfoTime* out) {
    if (out == 0) {
        return 0;
    }
    CTime now = CTime::GetCurrentTime();
    out->m_month = now.GetLocalTm(0)->tm_mon + 1;
    out->m_day = now.GetLocalTm(0)->tm_mday;
    out->m_year = now.GetLocalTm(0)->tm_year + 1900;
    return 1;
}

RVA(0x001183b0, 0x211)
i32 CGameInfo::FormatGameInfoString() {
    char* name = m_body.m_name;
    if (name == 0) {
        return 0;
    }
    if (strlen(name) == 0) {
        return 0;
    }
    if (!HasSupportedVersion()) {
        return 0;
    }

    g_infoMaster[0] = 0;
    sprintf(
        g_infoScratch,
        "Name=%s&Type=%i&Location=%s&Version=%lu",
        name,
        m_body.m_type,
        m_body.m_location,
        m_body.m_version
    );
    strcat(g_infoMaster, g_infoScratch);

    CGameInfoTime* t = &m_body.m_time;
    if (t == 0) {
        return 0;
    }
    if (!ValidateGameTime(t)) {
        memset(t, 0, 28);
    }

    u32 a = 0, b = 0, c = 0;
    SplitMillisToHMS(t->m_timeMs, &a, &b, &c);
    sprintf(g_infoScratch, "&S=%lu&H=%i&M=%02i&SE=%02i", t->m_score, a, b, c);
    strcat(g_infoMaster, g_infoScratch);

    sprintf(g_infoScratch, "&Month=%i&Day=%i&Year=%i", t->m_month, t->m_day, t->m_year);
    strcat(g_infoMaster, g_infoScratch);

    i32 chk = (69 * (b * a) + 1) * c + b + a + t->m_month + t->m_year + t->m_day + t->m_score;
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
