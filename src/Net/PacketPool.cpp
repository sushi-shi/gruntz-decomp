// PacketPool.cpp - a free-list pool for a 0x410-byte record (anonymous class).
//
// g_pool_64aca8 is the CObList free list, g_count_64acb4 its count.  The two
// functions here are the pool's preallocate helper (0xbef10) and its custom
// allocator (0xbf530): when the free list is non-empty pop the tail (optionally
// zeroing the record), else fall back to ::operator new(0x410).
#include <rva.h>
#include <Mfc.h>

// Pool free list (modeled minimally; only its address is referenced -> reloc-masked).
struct PacketPool {
    void AddCount(int n); // 0x1b4867 - CObList helper taking a count
    void* RemoveTail();   // 0x1b4a27 - CObList::RemoveTail
};
SIZE_UNKNOWN(PacketPool); // method-only CObList view; retail size TBD
extern PacketPool g_pool_64aca8;
extern int g_count_64acb4;

// Preallocate 10 records into the free list.
RVA(0x000bef10, 0xd)
void Unmatched_bef10() {
    g_pool_64aca8.AddCount(10);
}

// Custom allocator: reuse a freed record (optionally cleared) or
// allocate a fresh 0x410-byte block.
RVA(0x000bf530, 0x3b)
void* Unmatched_bf530(int bClear) {
    if (g_count_64acb4) {
        void* p = g_pool_64aca8.RemoveTail();
        if (bClear) {
            memset(p, 0, 0x410);
        }
        return p;
    }
    return ::operator new(0x410);
}
