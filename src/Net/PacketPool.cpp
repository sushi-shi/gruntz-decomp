#include <rva.h>

#include <Mfc.h>

#include <Net/CmdPool.h>
#include <Net/NetMgr.h>

RVA(0x000bf530, 0x3b)
void* AllocateGruntRecord(int bClear) {
    CPtrList& freeList = CPtrListPool<GruntRec>::s_freeList;
    if (freeList.GetCount()) {
        void* p = freeList.RemoveTail();
        if (bClear) {
            memset(p, 0, sizeof(GruntRec));
        }
        return p;
    }
    return new GruntRec;
}
