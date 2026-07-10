// DebugPrintf.cpp - parse the DPRINTF env var into the debug-output mode (RVA
// 0x185000). Reads %DPRINTF%, upper-cases it, and maps the first matching device
// keyword to the global mode word (g_6bf8dc); the parsed string is then handed to
// the sink-configure helper. __thiscall, returns this. Field names are
// placeholders; only offsets + code bytes are load-bearing.
#include <rva.h>

#include <stdlib.h>
#include <string.h>
#include <Globals.h>
#include <Gruntz/RangeSet.h> // canonical CRangeSet (the debug-channel set viewed on the sink)

// The debug-output sink object; its first dword is reset, then it is configured
// from the parsed keyword string.
DATA(0x006bf850)
extern CRangeSet g_6bf850;

// ---------------------------------------------------------------------------
// 0x184e00 - debug-gated assert/printf (re-homed from src/Stub/EngineExternFns.cpp):
// when the debug mode (g_6bf8dc) is armed and channel 0 is enabled in the debug-
// channel set (g_6bf850, viewed as a CRangeSet), vsprintf the varargs into a
// 256-byte stack buffer and hand it to the sink (0x184df0). Body byte-identical (the
// vsprintf / sink call relocs are reloc-masked). va_list is spelled `(char*)(&fmt+1)`
// to avoid <stdarg.h>. Contains is external - only its mangled name is load-bearing.
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
}

SIZE_UNKNOWN(CDebugConfig);
class CDebugConfig {
public:
    CDebugConfig* InitFromEnv(); // 0x185000
};

// The debug-config singleton (VA 0x6bf848); InitFromEnv drives its members g_6bf850
// (the +0x8 CRangeSet) and g_6bf8dc (the +0x94 mode word), which this TU also reaches
// as free globals.
DATA(0x006bf848)
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
