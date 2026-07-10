// DebugTiming.cpp - two low-level engine free helpers that sit contiguously in retail
// .text just before the image-cache/CDDSurface code (0x13dfe0..0x13e042):
//   ActiveWait (0x13dfe0)  - a timeGetTime busy-wait (no Sleep; spins). The engine
//                            frame-pacing primitive; ~10 cross-module callers.
//   DebugTrace (0x13e010)  - a printf-style OutputDebugStringA formatter (__cdecl
//                            varargs). Inlined at every call site, so this is the
//                            orphan out-of-line copy.
// Recovered out of the fake grab-bag src/Utils/WinAPI.cpp (which spanned 1.1 MB of
// unrelated modules and has been deleted). They form part of the small free-helper
// TU whose other visible members are WaitKeyEdge (0x13df30) and ClearImageCache
// (0x13e070); a future pass may consolidate all four here.
//
// <Mfc.h> brings <windows.h> (WINMM timeGetTime, KERNEL32 OutputDebugStringA, DWORD).
#include <Mfc.h>
#include <rva.h>
#include <stdarg.h> // va_list for the DebugTrace varargs formatter
#include <stdio.h>  // vsprintf

// -------------------------------------------------------------------------
// ActiveWait (0x13dfe0)
// Busy-waits ~milliseconds using timeGetTime (no Sleep; spins).
RVA(0x0013dfe0, 0x21)
void ActiveWait(u32 milliseconds) {
    DWORD target = timeGetTime() + milliseconds;
    while (timeGetTime() < target)
        ;
}

// -------------------------------------------------------------------------
// DebugTrace (0x13e010)
// printf-style formatter into a 256-byte stack buffer, emitted via OutputDebugStringA.
// __cdecl varargs. Orphan copy (inlined at all call sites).
RVA(0x0013e010, 0x32)
void DebugTrace(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
}
