#ifndef WAP32_NOTRACKOBJECTSTAMP_H
#define WAP32_NOTRACKOBJECTSTAMP_H

#include <rva.h>

struct CNoTrackObjectStamp {
    virtual ~CNoTrackObjectStamp(); // 0x11d100 - the ??_7CNoTrackObject base-vptr restamp
};
SIZE_UNKNOWN(CNoTrackObjectStamp);
RELOC_VTBL(CNoTrackObjectStamp, 0x001ec26c); // == ??_7CNoTrackObject@@6B@ (NAFXCW library vtable)

#endif // WAP32_NOTRACKOBJECTSTAMP_H
