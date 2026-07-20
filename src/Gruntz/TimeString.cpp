#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c / copy 0x1b9ba3 / dtor 0x1b9cde)
#include <rva.h>

#include <stdio.h>

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
