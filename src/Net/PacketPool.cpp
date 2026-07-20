#include <rva.h>
#include <Mfc.h> // CPtrList

#include <Net/CmdPool.h>
extern int g_poolCount; // 0x64acb4 (CANONICAL name, shared with Globals.cpp)

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
