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
// (ex-wall, RETIRED 2026-08-01 - code bytes exact; only the `atol` reloc NAME differs
// (Ghidra `atol` vs cl `_atol`). The "loop-carried-cursor regalloc wall / cl peels the
// first iteration's cursor into eax" note was a source bug: the body was spelled
// `if (*str == 0) return; do { ... } while (*str != 0);`, and cl rotates that do-while by
// DUPLICATING the leading strstr into the loop bottom - which is what forced the cursor
// out of ebx. The plain `while (*str != 0) { ... }` form gets retail's single bottom test
// (`cmp BYTE PTR [ebx],0x0; jne`) and pins the cursor in ebx from entry. 92.15 -> 99.87.)
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

// 0x184d50 - MONO-console newline: reset the column, advance the row and, when it
// runs past the last line (25), scroll the whole 80x25 word buffer up one line
// (0xa2-byte word copy) then blank the bottom line (0x0720), leaving the row at 24.
// @early-stop
// 98.6% (was 91.0). The "cl caches g_monoBuffer in a callee-saved reg / colours the
// scroll temporaries one register apart" note was wrong: the blank loop's pre-increment
// spelling (`i += 2;` first, then `buf + i - 2`) made cl HOIST the 0x720 constant into
// ecx for the loop and push the buffer into edx; MonoClear's post-increment spelling
// (`buf + i` ... `i += 2;`) keeps 0x720 an immediate and the buffer in ecx, exactly as
// retail. Residual is 3 rows of the SIB base/index coin-flip shared with MonoClear and
// the whole CHashBase family (`[ecx+eax*1+d]` vs `[eax+ecx*1+d]`) - measured
// TU-cumulative state, not a local spelling (8 spellings tested here, 7 in MonoClear).
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
            // same byte-addressed 2-byte-cell page (retail `[ecx+eax*1-0x2]`); the
            // post-increment spelling (as in MonoClear) is what keeps 0x720 an
            // IMMEDIATE - the pre-increment form makes cl hoist it into ecx.
            Pix16Ptr cell;
            cell.m_chars = g_monoBuffer + i;
            *cell.m_words = 0x720;
            i += 2;
        } while (i < 0xfa0);
        g_monoRow--;
    }
}

// 0x184db0 - MONO-console clear: blank the whole 80x25 word buffer (0x0720) and home
// the cursor (row 0, column 0).
// @early-stop
// one row left (99%): SIB role swap ([eax+edx] vs [edx+eax*1]) - operand order can't
// steer it (8 spellings tested 2026-08-01: &buf[i], i+buf, pre/post increment, u32 i,
// local buffer pointer, for-loop, row/col hoisted - all identical to the byte). Same
// family as CHashBase::Prev/Insert/Remove/Last, which is measured TU-cumulative state.
// The old ~60% "codegen-alias wall" note was stale TU state: it cleared when
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
