#include <Rez/DebugPrintf.h> // C-linkage decls for the ex-wrapped defs
#include <rva.h>
#include <Pix16.h> // the byte-cursor / 16bpp-cell pointer pair

#include <stdarg.h>          // va_list / va_start - the real spelling of `(char*)(&fmt+1)`
#include <stdlib.h>          // atol / getenv
#include <string.h>          // inline strcpy (rep movs / repne scasb), strpbrk, strstr
#include <Gruntz/RangeSet.h> // canonical CRangeSet + CRange (the debug-channel set)
#include <Rez/DebugConfig.h> // canonical CDebugConfig (the debug-output config singleton)

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

// ===========================================================================
// 0x184c10 - CRangeSet::AddFromString(str): parse a marker-delimited number/range
// list ("X<n>" or "X<lo>-<hi>" tokens) and AddRange each. For every 'X' marker
// found, skip to the first digit, extract the leading digit run into a scratch
// buffer (truncating at the first non-digit while advancing the cursor in
// parallel), atol it, and - if a '-' follows - parse the upper bound the same way;
// otherwise the range is a single value. Stops at end-of-string or a missing
// marker/digit.
// ===========================================================================
// @early-stop
// ~92.15% loop-carried-cursor regalloc wall (docs/patterns/shrink-wrapped-callee-
// save-push.md / this-spilled-to-local-for-loop-seed.md family): the ENTIRE loop
// body (inline strcpy, digit-scan, atol, the '-' range branch, AddRange) is
// byte-identical. The only residual is the prologue - retail pins the cursor in
// ebx from entry (`push ebx; mov ebx,[esp+0x10c]`) and keeps it there across every
// iteration, while cl peels the first iteration's cursor into eax (the param is
// dead after the first strstr). Both `char* s = str;` and reusing the
// `str` param produce the identical eax split; not source-steerable. The lone
// `atol` (0x11ff10) rel32 is also reloc-masked (Ghidra names it `atol`, cl emits
// `_atol`). Logic complete; parked for the final sweep.
RVA(0x00184c10, 0x136)
void CRangeSet::AddFromString(char* str) {
    char buf[0x100];
    if (*str == 0) {
        return;
    }
    do {
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
    } while (*str != 0);
}

// 0x184d50 - MONO-console newline: reset the column, advance the row and, when it
// runs past the last line (25), scroll the whole 80x25 word buffer up one line
// (0xa2-byte word copy) then blank the bottom line (0x0720), leaving the row at 24.
// @early-stop
// codegen-alias wall (~91%): logic + the two do-while word loops + the 25-line scroll
// gate are byte-faithful, but retail reloads g_monoBuffer into a volatile reg each
// iteration where cl caches it in a callee-saved reg (adds a push/pop), and colours
// the scroll temporaries one register apart. Not source-steerable.
RVA(0x00184d50, 0x5f)
void MonoNewline() {
    g_monoCol = 0;
    if (++g_monoRow == 25) {
        i32 i = 0xa0;
        do {
            i += 2;
            // The MDA text page is a byte-addressed 80x25 grid of 2-byte (char,attr)
            // cells; the scroll runs on its 0xa0-byte LINE stride. Retail indexes it
            // *1 (`[ecx+eax*1-0xa2]`) where a u16[] model would emit *2, so the
            // cursor stays a byte cursor and Pix16Ptr names the cell it addresses.
            Pix16Ptr dst;
            Pix16Ptr src;
            dst.m_chars = (g_monoBuffer + i - 0xa2);
            src.m_chars = (g_monoBuffer + i - 2);
            *dst.m_words = *src.m_words;
        } while (i < 0xfa0);
        i = 0xf00;
        do {
            i += 2;
            // same byte-addressed 2-byte-cell page (retail `[ecx+eax*1-0x2]`)
            Pix16Ptr cell;
            cell.m_chars = (g_monoBuffer + i - 2);
            *cell.m_words = 0x720;
        } while (i < 0xfa0);
        g_monoRow--;
    }
}

// 0x184db0 - MONO-console clear: blank the whole 80x25 word buffer (0x0720) and home
// the cursor (row 0, column 0).
// @early-stop
// one row left (99%): SIB role swap ([eax+edx] vs [edx+eax*1]) - operand order can't
// steer it. The old ~60% "codegen-alias wall" note was stale TU state: it cleared when
// the Rez*Printf family stopped hand-rolling `(char*)(&fmt+1)` and used va_start.
RVA(0x00184db0, 0x28)
void MonoClear() {
    i32 i = 0;
    do {
        // byte-addressed MDA page, 2-byte cells, retail indexes *1
        Pix16Ptr cell;
        cell.m_chars = g_monoBuffer + i;
        *cell.m_words = 0x720;
        i += 2;
    } while (i < 0xfa0);
    g_monoRow = 0;
    g_monoCol = 0;
}

RVA(0x00184e00, 0x55)
void RezAssertFail(char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(0)) {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DebugSink_184df0(buf);
    }
}

// 0x184e60 - channel-0 debug printf that first positions the cursor (x,y).
RVA(0x00184e60, 0x6d)
void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(0)) {
        DebugSetCursorXY(x, y);
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DebugSink_184df0(buf);
    }
}

// 0x184ed0 - debug printf gated on a caller-supplied channel.
RVA(0x00184ed0, 0x5b)
void RezDebugPrintfCh(i32 channel, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(channel)) {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DebugSink_184df0(buf);
    }
}

// 0x184f30 - channel-gated debug printf that positions the cursor (x,y) first.
RVA(0x00184f30, 0x73)
void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...) {
    char buf[256];
    if (g_debugPrintMode != 1 && g_debugPrintMode != 0
        && !(static_cast<CRangeSet*>(&g_debugChannels))->Contains(channel)) {
        DebugSetCursorXY(x, y);
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        DebugSink_184df0(buf);
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
