#include <rva.h>
#include <Mfc.h> // CPtrList

#include <Net/CmdPool.h>
#include <Net/PacketPool.h> // ex Globals.h

DATA(0x0024acb4)
i32 g_poolCount;

RVA(0x000bef10, 0xd)
void Unmatched_bef10() {
    g_pool.CPtrList::CPtrList(0xa);
}

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
