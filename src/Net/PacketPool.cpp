// PacketPool.cpp - a free-list pool for a 0x410-byte record (anonymous class).
//
// g_pool_64aca8 is the CObList free list, g_poolCount (0x64acb4) its count.  The two
// functions here are the pool's preallocate helper (0xbef10) and its custom
// allocator (0xbf530): when the free list is non-empty pop the tail (optionally
// zeroing the record), else fall back to ::operator new(0x410).
#include <rva.h>
#include <Mfc.h> // CPtrList

// The free list is the canonical CPtrList g_pool (0x64aca8, defined in NetCmdSlot.cpp);
// g_poolCount (0x64acb4) is its live-record count.
extern CPtrList g_pool; // 0x64aca8
extern int g_poolCount; // 0x64acb4 (CANONICAL name, shared with Globals.cpp)

// Preallocate: construct the pool free list with a block size of 10 (explicit ctor
// call - the raw `push 0xa; mov ecx,&g_pool; call ??0CPtrList` the retail initializer is).
RVA(0x000bef10, 0xd)
void Unmatched_bef10() {
    g_pool.CPtrList::CPtrList(0xa);
}

// Custom allocator: reuse a freed record (optionally cleared) or
// allocate a fresh 0x410-byte block.
RVA(0x000bf530, 0x3b)
void* Unmatched_bf530(int bClear) {
    if (g_poolCount) {
        void* p = g_pool.RemoveTail();
        if (bClear) {
            memset(p, 0, 0x410);
        }
        return p;
    }
    return ::operator new(0x410);
}
