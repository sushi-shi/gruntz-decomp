// TimeString.cpp - the elapsed-time -> "h:mm:ss" CString formatter (RVA 0x1190f0).
//
// A __cdecl free helper that turns a millisecond count into a CString of the
// form "%i:%02i:%02i" (hours:minutes:seconds). The hour/minute/second split is
// the engine's sequential remainder reduction (ms/3600000, then the running
// remainder /60000, then /1000), each an unsigned magic-multiply divide; sprintf
// fills a stack buffer and the CString is returned by value (hidden return-slot
// arg, MFC CString copy-ctor from the stack temp -> /GX EH frame). Field names
// are placeholders; only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c / copy 0x1b9ba3 / dtor 0x1b9cde)
#include <rva.h>

#include <stdio.h>

// 0x1190f0: split `ms` into h:m:s by the sequential remainder reduction and
// format it as "h:mm:ss".
RVA(0x001190f0, 0xda)
CString FormatElapsedTime(u32 ms) {
    u32 hours = ms / 3600000;
    ms = ms - hours * 3600000;
    u32 minutes = ms / 60000;
    ms = ms - minutes * 60000;
    u32 seconds = ms / 1000;
    char buf[64];
    sprintf(buf, "%i:%02i:%02i", hours, minutes, seconds);
    // MSVC5 has no NRVO: the named local forces construct-local + copy-into-the
    // return slot + destroy-local (the /GX EH frame wraps the destructible temp).
    CString result(buf);
    return result;
}
