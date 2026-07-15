// RezAlloc.h - the engine's global allocator pair (the Rez memory bracket).
//
// RezAlloc (0x1b9b46) IS `operator new`: it forwards the byte count to malloc (0x120b60)
// and runs the new-handler retry loop on failure. RezFree (0x1b9b82) IS `operator delete`:
// it forwards the pointer to free (0x120c30). Both are __cdecl, extern "C" (their C-linkage
// names `_RezAlloc`/`_RezFree` are reloc-masked in objdiff), reconstructed nowhere here -
// they are library/CRT entry points every subsystem's node allocators call.
//
// ABI settled from the retail definitions (disasm): one 4-byte argument each - RezAlloc
// takes the size (unsigned), RezFree the pointer. The u32/i32/unsigned-int/size/n/sz decl
// spellings that used to be scattered per-TU are all the SAME C symbol and caller-neutral;
// unified here to the one canonical form so consumers stop re-declaring them.
#ifndef INCLUDE_REZ_REZALLOC_H
#define INCLUDE_REZ_REZALLOC_H
#include <Ints.h>

extern "C" void* RezAlloc(u32 size); // 0x1b9b46  operator new  (-> malloc 0x120b60)
extern "C" void RezFree(void* p);    // 0x1b9b82  operator delete (-> free 0x120c30)

#endif // INCLUDE_REZ_REZALLOC_H
