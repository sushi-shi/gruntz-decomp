#include <rva.h>

#include <Rez/DebugPrintf.h>

#include <Win32.h>

#include <Gruntz/RangeSet.h>
#include <Pix16.h>
#include <Rez/DebugConfig.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002c07a4)
char* g_monoBuffer = NULL;
DATA(0x002c07a8)
CRangeSet g_debugChannels = {0};
DATA(0x002c082c)
i32 g_monoRow = 0;
DATA(0x002c0830)
i32 g_monoCol = 0;
DATA(0x002c0834)
DebugPrintMode g_debugPrintMode = DEBUG_PRINT_DISABLED;
DATA(0x002c0838)
FILE* g_debugLogFile = NULL;

RVA_DYNINIT(0x00184e40, 0xa, g_debugConfig)
RVA_DYNINIT(0x00184e50, 0xa, g_debugConfig)
RVA_DYNINIT(0x00184e60, 0xe, g_debugConfig)
RVA_DYNINIT(0x00184e70, 0xa, g_debugConfig)
DATA(0x002c07a0)
CDebugConfig g_debugConfig;

RVA(0x00184e80, 0x33)
bool CRangeSet::Contains(u32 value) {
    for (u32 i = 0; i < m_count; i++) {
        if (value >= m_pairs[i].lo && value <= m_pairs[i].hi) {
            return true;
        }
    }
    return false;
}

RVA(0x00184ec0, 0x24)
void CRangeSet::AddRange(u32 lo, u32 hi) {
    if (m_count + 1 < 16) {
        m_pairs[m_count].lo = lo;
        m_pairs[m_count].hi = hi;
        m_count = m_count + 1;
    }
}

RVA(0x00184ef0, 0x136)
void CRangeSet::AddFromString(char* str) {
    char buf[0x100];
    while (*str != 0) {
        char* x = strstr(str, "X");
        if (x == NULL) {
            return;
        }
        str = strpbrk(x, "0123456789");
        if (str == NULL) {
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
            if (str == NULL) {
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00185030, 0x5f)
void MonoNewline() {
    g_monoCol = 0;
    if (++g_monoRow == DEBUG_MONO_ROW_COUNT) {
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00185090, 0x28)
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

// @identity-TODO: the OutputDebugStringA forwarding behavior is proven; the name is inferred.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001850c0, 0xc)
void DebugOutputString(char* line) {
    OutputDebugStringA(line);
}

RVA(0x001850d0, 0x1)
void DiscardDebugOutput(char* line) {}

RVA(0x001850e0, 0x55)
void RezAssertFail(char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != DEBUG_PRINT_DISCARD && g_debugPrintMode != DEBUG_PRINT_DISABLED
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(0)) {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00185140, 0x6d)
void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != DEBUG_PRINT_DISCARD && g_debugPrintMode != DEBUG_PRINT_DISABLED
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(0)) {
        DebugSetCursorXY(x, y);
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001851b0, 0x5b)
void RezDebugPrintfCh(i32 channel, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != DEBUG_PRINT_DISCARD && g_debugPrintMode != DEBUG_PRINT_DISABLED
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(channel)) {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00185210, 0x73)
void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != DEBUG_PRINT_DISCARD && g_debugPrintMode != DEBUG_PRINT_DISABLED
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(channel)) {
        DebugSetCursorXY(x, y);
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DiscardDebugOutput(buf);
    }
}

RVA(0x00185290, 0x15)
void DebugSetCursorXY(i32 x, i32 y) {
    DebugSetCursor(0, x, y);
}

RVA(0x001852b0, 0x1)
void DebugSetCursor(i32 channel, i32 x, i32 y) {}

// @identity-TODO: the default-channel wrapper relationship is proven; the names are inferred.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001852c0, 0xb)
void DebugClear() {
    DebugClearChannel(0);
}

RVA(0x001852d0, 0x1)
void DebugClearChannel(i32 channel) {}

RVA(0x001852e0, 0x1a6)
CDebugConfig::CDebugConfig() {
    char buf[256];
    g_debugChannels.m_count = 0;
    g_debugPrintMode = DEBUG_PRINT_DISCARD;
    char* env = getenv("DPRINTF");
    if (env != NULL) {
        strcpy(buf, env);
        _strupr(buf);
        if (strstr(buf, "MONO")) {
            g_debugPrintMode = DEBUG_PRINT_MONO;
        }
        if (strstr(buf, "FILE")) {
            g_debugPrintMode = DEBUG_PRINT_FILE;
        }
        if (strstr(buf, "FILEAPPEND")) {
            g_debugPrintMode = DEBUG_PRINT_FILE_APPEND;
        }
        if (strstr(buf, "COM1")) {
            g_debugPrintMode = DEBUG_PRINT_COM1;
        }
        if (strstr(buf, "COM2")) {
            g_debugPrintMode = DEBUG_PRINT_COM2;
        }
        if (strstr(buf, "STDOUT")) {
            g_debugPrintMode = DEBUG_PRINT_STDOUT;
        }
        if (strstr(buf, "LPT1")) {
            g_debugPrintMode = DEBUG_PRINT_LPT;
        }
        if (strstr(buf, "LPT2")) {
            g_debugPrintMode = DEBUG_PRINT_LPT;
        }
        if (strstr(buf, "PRN")) {
            g_debugPrintMode = DEBUG_PRINT_PRN;
        }
        g_debugChannels.AddFromString(buf);
    }
    g_debugPrintMode = DEBUG_PRINT_MONO;
}

RVA(0x00185490, 0x23)
CDebugConfig::~CDebugConfig() {
    if (g_debugPrintMode == DEBUG_PRINT_FILE
        || (IDX(g_debugPrintMode) > IDX(DEBUG_PRINT_STDOUT)
            && IDX(g_debugPrintMode) <= IDX(DEBUG_PRINT_PRN))) {
        fclose(g_debugLogFile);
    }
}
