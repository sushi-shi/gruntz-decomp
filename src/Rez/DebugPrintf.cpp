#include <rva.h>

#include <Rez/DebugPrintf.h>

#include <Win32.h>

#include <Gruntz/RangeSet.h>
#include <Rez/DebugConfig.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002bf84c)
u16* g_monoBuffer = NULL;
DATA(0x002bf850)
CRangeSet g_debugChannels = {0};
DATA(0x002bf8d4)
i32 g_monoRow = 0;
DATA(0x002bf8d8)
i32 g_monoCol = 0;
DATA(0x002bf8dc)
DebugPrintMode g_debugPrintMode = DEBUG_PRINT_DISABLED;
DATA(0x002bf8e0)
FILE* g_debugLogFile = NULL;

RVA_DYNINIT(0x00184b60, 0xa, g_debugConfig)
RVA_DYNINIT(0x00184b70, 0xa, g_debugConfig)
RVA_DYNINIT(0x00184b80, 0xe, g_debugConfig)
RVA_DYNINIT(0x00184b90, 0xa, g_debugConfig)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184d50, 0x5f)
void MonoNewline() {
    g_monoCol = 0;
    if (++g_monoRow == DEBUG_MONO_ROW_COUNT) {
        i32 i;
        for (i = DEBUG_MONO_COLUMN_COUNT; i < DEBUG_MONO_COLUMN_COUNT * DEBUG_MONO_ROW_COUNT; i++) {
            g_monoBuffer[i - DEBUG_MONO_COLUMN_COUNT] = g_monoBuffer[i];
        }
        for (i = DEBUG_MONO_COLUMN_COUNT * (DEBUG_MONO_ROW_COUNT - 1);
             i < DEBUG_MONO_COLUMN_COUNT * DEBUG_MONO_ROW_COUNT;
             i++) {
            g_monoBuffer[i] = 0x720;
        }
        g_monoRow--;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184db0, 0x28)
void MonoClear() {
    i32 i;
    for (i = 0; i < DEBUG_MONO_COLUMN_COUNT * DEBUG_MONO_ROW_COUNT; i++) {
        g_monoBuffer[i] = 0x720;
    }
    g_monoRow = 0;
    g_monoCol = 0;
}

// @identity-TODO: the OutputDebugStringA forwarding behavior is proven; the name is inferred.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184de0, 0xc)
void DebugOutputString(char* line) {
    OutputDebugStringA(line);
}

RVA(0x00184df0, 0x1)
void DiscardDebugOutput(char* line) {}

RVA(0x00184e00, 0x55)
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
RVA(0x00184e60, 0x6d)
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
RVA(0x00184ed0, 0x5b)
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
RVA(0x00184f30, 0x73)
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

RVA(0x00184fb0, 0x15)
void DebugSetCursorXY(i32 x, i32 y) {
    DebugSetCursor(0, x, y);
}

RVA(0x00184fd0, 0x1)
void DebugSetCursor(i32 channel, i32 x, i32 y) {}

// @identity-TODO: the default-channel wrapper relationship is proven; the names are inferred.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184fe0, 0xb)
void DebugClear() {
    DebugClearChannel(0);
}

RVA(0x00184ff0, 0x1)
void DebugClearChannel(i32 channel) {}

RVA(0x00185000, 0x1a6)
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

RVA(0x001851b0, 0x23)
CDebugConfig::~CDebugConfig() {
    if (g_debugPrintMode == DEBUG_PRINT_FILE
        || (IDX(g_debugPrintMode) > IDX(DEBUG_PRINT_STDOUT)
            && IDX(g_debugPrintMode) <= IDX(DEBUG_PRINT_PRN))) {
        fclose(g_debugLogFile);
    }
}
