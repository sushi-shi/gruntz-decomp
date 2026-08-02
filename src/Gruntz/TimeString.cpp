#include <rva.h>

#include <Mfc.h>

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

    CString result(buf);
    return result;
}
