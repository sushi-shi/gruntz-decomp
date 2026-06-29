// m5_DebugPrintf.cpp - parse the DPRINTF env var into the debug-output mode (RVA
// 0x185000). Reads %DPRINTF%, upper-cases it, and maps the first matching device
// keyword to the global mode word (g_6bf8dc); the parsed string is then handed to
// the sink-configure helper. __thiscall, returns this. Field names are
// placeholders; only offsets + code bytes are load-bearing.
#include <rva.h>

#include <stdlib.h>
#include <string.h>

// The debug-output sink object; its first dword is reset, then it is configured
// from the parsed keyword string.
struct CDebugSink {
    i32 m_0;                 // +0x00
    void Configure(char* s); // 0x184c10
};
DATA(0x006bf850)
extern CDebugSink g_6bf850;

extern i32 g_6bf8dc; // debug-output mode

class CDebugConfig {
public:
    CDebugConfig* InitFromEnv(); // 0x185000
};

RVA(0x00185000, 0x1a6)
CDebugConfig* CDebugConfig::InitFromEnv() {
    char buf[256];
    g_6bf850.m_0 = 0;
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
        g_6bf850.Configure(buf);
    }
    g_6bf8dc = 2;
    return this;
}
