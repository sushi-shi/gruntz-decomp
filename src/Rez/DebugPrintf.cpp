#include <rva.h>

#include <Rez/DebugPrintf.h>

#include <Gruntz/RangeSet.h>
#include <Pix16.h>
#include <Rez/DebugConfig.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002bf84c)
char* g_monoBuffer = 0;
DATA(0x002bf850)
CRangeSet g_debugChannels = {0};
DATA(0x002bf8d4)
i32 g_monoRow = 0;
DATA(0x002bf8d8)
i32 g_monoCol = 0;
DATA(0x002bf8dc)
i32 g_debugPrintMode = 0;
DATA(0x002bf8e0)
FILE* g_debugLogFile = 0;

DATA(0x002bf848)
CDebugConfig g_debugConfig;

RVA(0x00184ba0, 0x33)
bool CRangeSet::Contains(u32 value) {
    for (u32 i = 0; i < m_count; i++) {
        if (value >= m_pairs[i].lo && value <= m_pairs[i].hi) {
            return true;
        }
    }
    return false;
}

RVA(0x00184be0, 0x24)
void CRangeSet::AddRange(u32 lo, u32 hi) {
    if (m_count + 1 < 16) {
        m_pairs[m_count].lo = lo;
        m_pairs[m_count].hi = hi;
        m_count = m_count + 1;
    }
}

RVA(0x00184c10, 0x136)
void CRangeSet::AddFromString(char* str) {
    char buf[0x100];
    while (*str != 0) {
        char* x = strstr(str, "X");
        if (x == 0) {
            return;
        }
        str = strpbrk(x, "0123456789");
        if (str == 0) {
            return;
        }
        strcpy(buf, str);
        char* q = buf;
        while (*q != 0) {
            char c = *q;
            if (c >= '0' && c <= '9') {
                q++;
                str++;
            } else {
                *q = 0;
            }
        }
        i32 lo = atol(buf);
        i32 hi;
        if (*str == '-') {
            str = strpbrk(str, "0123456789");
            if (str == 0) {
                return;
            }
            strcpy(buf, str);
            q = buf;
            while (*q != 0) {
                char c = *q;
                if (c >= '0' && c <= '9') {
                    q++;
                    str++;
                } else {
                    *q = 0;
                }
            }
            hi = atol(buf);
        } else {
            hi = lo;
        }
        AddRange(lo, hi);
    }
}

// @early-stop
RVA(0x00184d50, 0x5f)
void MonoNewline() {
    g_monoCol = 0;
    if (++g_monoRow == 25) {
        i32 i = 0xa0;
        do {
            i += 2;

            Pix16Ptr dst;
            Pix16Ptr src;
            dst.m_chars = (g_monoBuffer + i - 0xa2);
            src.m_chars = (g_monoBuffer + i - 2);
            *dst.m_words = *src.m_words;
        } while (i < 0xfa0);
        i = 0xf00;
        do {

            Pix16Ptr cell;
            cell.m_chars = g_monoBuffer + i;
            *cell.m_words = 0x720;
            i += 2;
        } while (i < 0xfa0);
        g_monoRow--;
    }
}

// @early-stop
RVA(0x00184db0, 0x28)
void MonoClear() {
    i32 i = 0;
    do {

        Pix16Ptr cell;
        cell.m_chars = g_monoBuffer + i;
        *cell.m_words = 0x720;
        i += 2;
    } while (i < 0xfa0);
    g_monoRow = 0;
    g_monoCol = 0;
}

// retail: a 1-byte ret - debug output literally discarded
RVA(0x00184df0, 0x1)
void DiscardDebugOutput(char* line) {}

RVA(0x00184e00, 0x55)
void RezAssertFail(char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(0)) {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

RVA(0x00184e60, 0x6d)
void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(0)) {
        DebugSetCursorXY(x, y);
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

RVA(0x00184ed0, 0x5b)
void RezDebugPrintfCh(i32 channel, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(channel)) {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

RVA(0x00184f30, 0x73)
void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(channel)) {
        DebugSetCursorXY(x, y);
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

RVA(0x00184fb0, 0x15)
void DebugSetCursorXY(i32 a, i32 b) {
    DebugSetCursor(0, a, b);
}

RVA(0x00184fd0, 0x1)
void DebugSetCursor(i32, i32, i32) {}

RVA(0x00185000, 0x1a6)
CDebugConfig::CDebugConfig() {
    char buf[256];
    g_debugChannels.m_count = 0;
    g_debugPrintMode = 1;
    char* env = getenv("DPRINTF");
    if (env != 0) {
        strcpy(buf, env);
        _strupr(buf);
        if (strstr(buf, "MONO")) {
            g_debugPrintMode = 2;
        }
        if (strstr(buf, "FILE")) {
            g_debugPrintMode = 5;
        }
        if (strstr(buf, "FILEAPPEND")) {
            g_debugPrintMode = 6;
        }
        if (strstr(buf, "COM1")) {
            g_debugPrintMode = 3;
        }
        if (strstr(buf, "COM2")) {
            g_debugPrintMode = 4;
        }
        if (strstr(buf, "STDOUT")) {
            g_debugPrintMode = 7;
        }
        if (strstr(buf, "LPT1")) {
            g_debugPrintMode = 8;
        }
        if (strstr(buf, "LPT2")) {
            g_debugPrintMode = 8;
        }
        if (strstr(buf, "PRN")) {
            g_debugPrintMode = 10;
        }
        g_debugChannels.AddFromString(buf);
    }
    g_debugPrintMode = 2;
}

RVA(0x001851b0, 0x23)
CDebugConfig::~CDebugConfig() {
    if (g_debugPrintMode == 5 || (g_debugPrintMode > 7 && g_debugPrintMode <= 10)) {
        fclose(g_debugLogFile);
    }
}
