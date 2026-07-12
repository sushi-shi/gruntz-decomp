// DebugPrintf.cpp - the engine debug-output config + sinks AND the debug-channel
// range set (C:\Proj\...): ONE original TU (wave1-E; interval dossier 0x1832d0
// pocket, sub-obj [0x184b70..0x1851d3]). Proven by the private-globals oracle:
// CRangeSet::AddFromString's literals (0x224eb4/0x224ec0) and CDebugConfig::
// InitFromEnv's keyword literals (0x224ec4..0x224f0c) interleave in ONE contiguous
// .data run (one obj's contribution), and the TU's single $E init fragment
// (@0x184b60) heads the block. The debug config parses DPRINTF range strings
// through the set - same file. Strict retail-RVA order.
//
// InitFromEnv (0x185000) reads %DPRINTF%, upper-cases it, and maps the first
// matching device keyword to the global mode word (g_6bf8dc); the parsed string is
// then handed to the debug-channel set. Field names are placeholders; only offsets
// + code bytes are load-bearing.
#include <rva.h>

#include <stdlib.h> // atol / getenv
#include <string.h> // inline strcpy (rep movs / repne scasb), strpbrk, strstr
#include <Globals.h>
#include <Gruntz/RangeSet.h> // canonical CRangeSet + CRange (the debug-channel set)

// The debug-output sink object; its first dword is reset, then it is configured
// from the parsed keyword string. (RVA = VA 0x6bf850 - image base 0x400000.)
DATA(0x002bf850)
extern CRangeSet g_6bf850;

// The open debug-output FILE handle (mode 5/8/9/10); DebugClose fcloses it.
DATA(0x002bf8e0)
extern void* g_6bf8e0;

// The MONO-mode (DPRINTF=MONO) text-console state: the 80x25 word buffer base
// (0xfa0 bytes, 0x0720 = grey space), the current row (0..24) and column. Reached
// as free globals like the rest of the debug config.
DATA(0x002bf84c)
extern char* g_monoBuffer;
DATA(0x002bf8d4)
extern i32 g_monoRow;
DATA(0x002bf8d8)
extern i32 g_monoCol;

// The cursor-position forwarder (0x184fb0 -> 0x184fd0(0, x, y), defined below - the
// RezDebugPrintf*XY variants position the cursor through it) and the CRT fclose
// (0x11f780) the printf variants / DebugClose reach; external no-body so their
// call relocs mask.
void Fwd_184fb0(i32 x, i32 y);
// @reloc-TODO: Sub_184fd0 @0x184fd0 (cursor-set-with-mode) is unreconstructed
// (Ghidra mis-sizes it 1 B); @rva-symbol only binds obj-DEFINED thunks, so Fwd_184fb0's
// rel32 CALL stays UNBOUND until this is reconstructed in its own TU band.
void Sub_184fd0(i32, i32, i32);  // 0x184fd0 (cursor set with mode; reloc-masked)
extern "C" i32 fclose(void* fp); // 0x11f780 (CRT fclose, library row _fclose)

SIZE_UNKNOWN(CDebugConfig);
class CDebugConfig {
public:
    CDebugConfig* InitFromEnv(); // 0x185000
};

// The debug-config singleton (VA 0x6bf848 -> RVA 0x2bf848); InitFromEnv drives its
// members g_6bf850 (the +0x8 CRangeSet) and g_6bf8dc (the +0x94 mode word), which this
// TU also reaches as free globals.
DATA(0x002bf848)
extern CDebugConfig g_debugConfig;

// 0x184b70 - re-initialise the debug config from %DPRINTF% (tail-forward to InitFromEnv
// on the singleton). Free __cdecl wrapper. (Re-homed from src/Stub/BoundaryUpper2.cpp;
// the "ClearHash_184b70" name + the CHashTail/Obj1397a0/CSymParser proximity views were
// a mis-attribution - the byte pattern `mov ecx,&g_debugConfig; jmp InitFromEnv` proves
// it is a CDebugConfig re-init, not a hash clear. Byte-exact.)
RVA(0x00184b70, 0xa)
void RezDebugInit() {
    g_debugConfig.InitFromEnv();
}

// ===========================================================================
// 0x184ba0 - CRangeSet::Contains(value): true if any stored [lo,hi] range
// (unsigned) includes value. Walks m_pairs[0..m_count).
// ===========================================================================
RVA(0x00184ba0, 0x33)
bool CRangeSet::Contains(u32 value) {
    for (u32 i = 0; i < m_count; i++) {
        if (value >= m_pairs[i].lo && value <= m_pairs[i].hi) {
            return true;
        }
    }
    return false;
}

// ===========================================================================
// 0x184be0 - CRangeSet::AddRange(lo, hi): append a pair if there is room
// (count+1 < 16). m_count is re-read after each store (the member array may
// alias it).
// ===========================================================================
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
            *(u16*)(g_monoBuffer + i - 0xa2) = *(u16*)(g_monoBuffer + i - 2);
        } while (i < 0xfa0);
        i = 0xf00;
        do {
            i += 2;
            *(u16*)(g_monoBuffer + i - 2) = 0x720;
        } while (i < 0xfa0);
        g_monoRow--;
    }
}

// 0x184db0 - MONO-console clear: blank the whole 80x25 word buffer (0x0720) and home
// the cursor (row 0, column 0).
// @early-stop
// codegen-alias wall (~60%): the blank loop + cursor-home are correct, but retail
// reloads g_monoBuffer into edx each iteration (cl caches it in esi -> push/pop) and
// stores the 0x0720 immediate directly where cl hoists it into cx; the row/col=0 pair
// reuses retail's zero reg. Same alias/hoist family as MonoNewline.
RVA(0x00184db0, 0x28)
void MonoClear() {
    i32 i = 0;
    do {
        i += 2;
        *(u16*)(g_monoBuffer + i - 2) = 0x720;
    } while (i < 0xfa0);
    g_monoRow = 0;
    g_monoCol = 0;
}

// ---------------------------------------------------------------------------
// 0x184e00 - debug-gated assert/printf (re-homed from src/Stub/EngineExternFns.cpp):
// when the debug mode (g_6bf8dc) is armed and channel 0 is enabled in the debug-
// channel set (g_6bf850), vsprintf the varargs into a 256-byte stack buffer and
// hand it to the sink (0x184df0). Body byte-identical (the vsprintf / sink call
// relocs are reloc-masked). va_list is spelled `(char*)(&fmt+1)` to avoid <stdarg.h>.
// ---------------------------------------------------------------------------
extern "C" {
    int vsprintf(char* buf, const char* fmt, char* va); // 0x121770 (CRT)
    void DebugSink_184df0(char* line);                  // 0x184df0 (1-byte sink)

    RVA(0x00184e00, 0x55)
    SYMBOL(_RezAssertFail)
    void RezAssertFail(char* fmt, ...) {
        char buf[256];
        if (g_6bf8dc != 1 && g_6bf8dc != 0 && !((CRangeSet*)&g_6bf850)->Contains(0)) {
            vsprintf(buf, fmt, (char*)(&fmt + 1));
            DebugSink_184df0(buf);
        }
    }

    // 0x184e60 - channel-0 debug printf that first positions the cursor (x,y).
    RVA(0x00184e60, 0x6d)
    SYMBOL(_RezDebugPrintfXY)
    void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...) {
        char buf[256];
        if (g_6bf8dc != 1 && g_6bf8dc != 0 && !((CRangeSet*)&g_6bf850)->Contains(0)) {
            Fwd_184fb0(x, y);
            vsprintf(buf, fmt, (char*)(&fmt + 1));
            DebugSink_184df0(buf);
        }
    }

    // 0x184ed0 - debug printf gated on a caller-supplied channel.
    RVA(0x00184ed0, 0x5b)
    SYMBOL(_RezDebugPrintfCh)
    void RezDebugPrintfCh(i32 channel, char* fmt, ...) {
        char buf[256];
        if (g_6bf8dc != 1 && g_6bf8dc != 0 && !((CRangeSet*)&g_6bf850)->Contains(channel)) {
            vsprintf(buf, fmt, (char*)(&fmt + 1));
            DebugSink_184df0(buf);
        }
    }

    // 0x184f30 - channel-gated debug printf that positions the cursor (x,y) first.
    RVA(0x00184f30, 0x73)
    SYMBOL(_RezDebugPrintfChXY)
    void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...) {
        char buf[256];
        if (g_6bf8dc != 1 && g_6bf8dc != 0 && !((CRangeSet*)&g_6bf850)->Contains(channel)) {
            Fwd_184fb0(x, y);
            vsprintf(buf, fmt, (char*)(&fmt + 1));
            DebugSink_184df0(buf);
        }
    }
}

// 0x184fb0 - the cursor-position forwarder the XY printf variants above call:
// `Sub_184fd0(0, x, y)` (mode 0). __cdecl. Re-homed from src/Stub/BoundaryUpper.cpp.
RVA(0x00184fb0, 0x15)
void Fwd_184fb0(i32 a, i32 b) {
    Sub_184fd0(0, a, b);
}

RVA(0x00185000, 0x1a6)
CDebugConfig* CDebugConfig::InitFromEnv() {
    char buf[256];
    g_6bf850.m_count = 0;
    g_6bf8dc = 1;
    char* env = getenv("DPRINTF");
    if (env != 0) {
        strcpy(buf, env);
        _strupr(buf);
        if (strstr(buf, "MONO")) {
            g_6bf8dc = 2;
        }
        if (strstr(buf, "FILE")) {
            g_6bf8dc = 5;
        }
        if (strstr(buf, "FILEAPPEND")) {
            g_6bf8dc = 6;
        }
        if (strstr(buf, "COM1")) {
            g_6bf8dc = 3;
        }
        if (strstr(buf, "COM2")) {
            g_6bf8dc = 4;
        }
        if (strstr(buf, "STDOUT")) {
            g_6bf8dc = 7;
        }
        if (strstr(buf, "LPT1")) {
            g_6bf8dc = 8;
        }
        if (strstr(buf, "LPT2")) {
            g_6bf8dc = 8;
        }
        if (strstr(buf, "PRN")) {
            g_6bf8dc = 10;
        }
        g_6bf850.AddFromString(buf);
    }
    g_6bf8dc = 2;
    return this;
}

// 0x1851b0 - close the debug-output file if the current mode owns an open handle
// (mode 5 = FILE, or 8/9/10 = LPT1/LPT2/PRN).
RVA(0x001851b0, 0x23)
void DebugClose() {
    if (g_6bf8dc == 5 || (g_6bf8dc > 7 && g_6bf8dc <= 10)) {
        fclose(g_6bf8e0);
    }
}
