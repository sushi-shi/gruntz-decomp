// PlayPlaneScan.h - the PlayPlaneScan TU's external declarations.
#ifndef GRUNTZ_PLAYPLANESCAN_H
#define GRUNTZ_PLAYPLANESCAN_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

extern "C" void PlaneType_Rock();    // 0x40137a
extern "C" void PlaneType_Covered(); // 0x403d0f
extern "C" void PlaneQuadA(); // 0x4017e4
extern "C" void PlaneQuadB(); // 0x40192e
extern "C" void PlaneQuadC(); // 0x403148
extern "C" void PlaneQuadD(); // 0x401087
extern "C" void PlaneQuadE(); // 0x40164f
extern "C" void PlaneQuadF(); // 0x4019bf (the 4-corner permute type)

#endif // GRUNTZ_PLAYPLANESCAN_H
