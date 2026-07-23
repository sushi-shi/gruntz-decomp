#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Gruntz/TypeCollRuntime.h>

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
#include <Bute/ButeTree.h>                                    // CButeTree (for the extern below)
extern CButeTree g_buteTree;

extern "C" i32 g_helperRefCount; // 0x2bf400 owner def in TypeKeyColl.cpp (C linkage)
#endif                           // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
