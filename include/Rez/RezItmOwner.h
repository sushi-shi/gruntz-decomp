// RezItmOwner.h - the recover/keep-going gate a Rez file/leaf node polls on an
// I/O failure (slot-2 Retry returns nonzero to retry an op, zero to give up).
// Modeled polymorphic with the slot indices kept so `mov eax,[ecx]; call [eax+8]`
// falls out (reloc-masked indirect call); slots 0/1 are placeholders. Shared by
// CRezItmBase (RezMgr.h) and CRezFileMgr (RezFile.h).
#ifndef REZ_REZITMOWNER_H
#define REZ_REZITMOWNER_H

#include <Ints.h>

class CRezItmOwner {
public:
    virtual void v0();   // +0x00
    virtual void v1();   // +0x04
    virtual i32 Retry(); // +0x08  (slot 2)
};

#endif // REZ_REZITMOWNER_H
