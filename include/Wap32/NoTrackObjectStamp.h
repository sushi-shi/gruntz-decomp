// NoTrackObjectStamp.h - CNoTrackObjectStamp, the standalone ??_7CNoTrackObject
// base-vptr restamp leaf (0x11d100).
//
// Modeled as a polymorphic placeholder whose OWN vtable IS the retail NAFXCW library
// vtable ??_7CNoTrackObject@@6B@ (0x1ec26c); RELOC_VTBL binds the compiler-emitted
// vptr-restamp reference so NO Vtbl_<hex> extern is needed (the ex `&Vtbl_5ec26c` hack is
// gone). Nothing constructs it, so no ??_G is emitted; the explicit empty dtor body IS the
// 7-byte restamp (`mov [ecx],??_7; ret`). Same shape as the L_13400 leaf. CNoTrackObject
// itself is MFC library code (NAFXCW); body in src/Wap32/WapMisc.cpp.
#ifndef WAP32_NOTRACKOBJECTSTAMP_H
#define WAP32_NOTRACKOBJECTSTAMP_H

#include <rva.h>

struct CNoTrackObjectStamp {
    virtual ~CNoTrackObjectStamp(); // 0x11d100 - the ??_7CNoTrackObject base-vptr restamp
};
SIZE_UNKNOWN(CNoTrackObjectStamp);
RELOC_VTBL(CNoTrackObjectStamp, 0x001ec26c); // == ??_7CNoTrackObject@@6B@ (NAFXCW library vtable)

#endif // WAP32_NOTRACKOBJECTSTAMP_H
