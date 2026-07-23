#include <rva.h>
#include <Mfc.h> // CPtrList

#include <Net/CmdPool.h>

RVA(0x000bf530, 0x3b)
void* Unmatched_bf530(int bClear) {
    CPtrList& freeList = CPtrListPool<CNetCmdPacket>::s_freeList;
    if (freeList.GetCount()) {
        void* p = freeList.RemoveTail();
        if (bClear) {
            memset(p, 0, 0x410);
        }
        return p;
    }
    return ::operator new(0x410);
}
