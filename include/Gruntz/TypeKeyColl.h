#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Gruntz/TypeCollRuntime.h>
#include <Bute/ButeTree.h> // CButeTree (for the extern below)

extern CTypeCollRuntime g_typeColl;

extern i32 g_typeCounter;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 g_recCount23;
extern CButeTree g_buteTree;

extern "C" i32 g_helperRefCount; // 0x2bf400 owner def in TypeKeyColl.cpp (C linkage)

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

// The DEFAULT CVariantSlot callback (0x16e220) - the "C++ Tools error handler".
// CVariantSlot's label ctor seeds slot +0x00 with it (retail 0x16e1a0 stores
// `mov [eax],0x56e220` outright), so its declaration has to precede that ctor.
void TmErrorHandler(char* prefix, i32 errNum); // 0x16e220

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
