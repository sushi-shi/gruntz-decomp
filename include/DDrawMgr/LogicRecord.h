// LogicRecord.h
#ifndef GRUNTZ_DDRAWMGR_LOGICRECORD_H_H
#define GRUNTZ_DDRAWMGR_LOGICRECORD_H_H

#include <Ints.h>

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" void Engine_Delete(void* p);

#endif // GRUNTZ_DDRAWMGR_LOGICRECORD_H_H
