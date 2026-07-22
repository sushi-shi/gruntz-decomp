// LevelPlane.h
#ifndef GRUNTZ_DDRAWMGR_LEVELPLANE_H_H
#define GRUNTZ_DDRAWMGR_LEVELPLANE_H_H

#include <Ints.h>


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern i32 __stdcall PlaneSaveVia(void* stream); // 0x163780 == CDDrawWorkerHost::Save entry
extern i32 __stdcall PlaneLoadVia(void* stream); // 0x1638c0 == CDDrawWorkerHost::Load entry

#endif // GRUNTZ_DDRAWMGR_LEVELPLANE_H_H
