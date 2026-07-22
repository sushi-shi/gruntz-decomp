#ifndef GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H
#define GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H

#include <Ints.h>
#include <rva.h>

#include <Image/RasterVtx.h>
SIZE_UNKNOWN();

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" ClipVtx g_rasterEdgeR[]; // 0x6856f8 (ascending-edge table; fill reads +0x10)
extern "C" ClipVtx g_rasterEdgeL[]; // 0x6a2cf0 (descending-edge table; fill reads +0x10)

#endif // GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H
