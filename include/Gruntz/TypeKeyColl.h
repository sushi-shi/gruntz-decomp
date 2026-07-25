#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Gruntz/TypeCollRuntime.h>
#include <Bute/ButeTree.h>                                    // CButeTree (for the extern below)

class CAnimNameRecord {
public:
    char* m_name; // +0x00
};
SIZE_UNKNOWN();

extern CTypeCollRuntime g_typeColl;

extern i32 g_typeCounter;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 g_recCount23;
extern "C" void Format_18d0f0(char* buf, i32 value, i32 cap); // 0x18d0f0
extern CButeTree g_buteTree;

extern "C" i32 g_helperRefCount; // 0x2bf400 owner def in TypeKeyColl.cpp (C linkage)

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

#endif                           // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
