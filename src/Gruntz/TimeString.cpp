#include <rva.h>

#include <Mfc.h>

#include <Utils/MillisPer.h>

#include <stdio.h>

RVA(0x001190f0, 0xda)
CString FormatElapsedTime(u32 ms) {
    u32 hours = ms / MILLIS_PER_HOUR;
    ms = ms - hours * MILLIS_PER_HOUR;
    u32 minutes = ms / MILLIS_PER_MINUTE;
    ms = ms - minutes * MILLIS_PER_MINUTE;
    u32 seconds = ms / MILLIS_PER_SECOND;
    char buf[64];
    sprintf(buf, "%i:%02i:%02i", hours, minutes, seconds);

    CString result(buf);
    return result;
}
