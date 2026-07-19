// RezItmOwner.h - the recover/keep-going gate a Rez file/leaf node polls on an
// I/O failure (slot-2 Retry returns nonzero to retry an op, zero to give up).
// Modeled polymorphic with the slot indices kept so `mov eax,[ecx]; call [eax+8]`
// falls out (reloc-masked indirect call). Shared by CRezItmBase (RezMgr.h) and
// CRezFileMgr (RezFile.h).
//
// IDENTITY (proven 2026-07-19): this is the caller-side view of CSymParser. The
// parent every rez node is constructed with IS the parser (`new CRezDir(this, ..)`
// / `new CRezItm(this)` inside CSymParser::ParseBuffer, SymTab.cpp), and the
// vtable shape is CSymParser's 3-slot ??_7 @0x1ef750 (SymParser.h): slot 0
// `i32 V0(i32)` 0x13b9f0, slot 1 `void V1(i32)` 0x13ba00, slot 2 `i32 V2()`
// 0x13ba10 - the inert `xor eax,eax; ret` default whose 0 IS this gate's
// "give up". Slot signatures below mirror the byte-proven canonical ones (the ex
// 0-arg `v0()/v1()` pair was invented). Full fold (typing m_parent CSymParser*)
// deferred: it drags <Bute/SymParser.h> into the Rez include closure for zero
// byte gain (the dispatch is already reloc-masked identical).
#ifndef REZ_REZITMOWNER_H
#define REZ_REZITMOWNER_H
#include <rva.h>

#include <Ints.h>

class CRezItmOwner {
public:
    virtual i32 V0(i32 a);  // +0x00  == CSymParser::V0 (0x13b9f0; role unrecovered)
    virtual void V1(i32 a); // +0x04  == CSymParser::V1 (0x13ba00; role unrecovered)
    virtual i32 Retry();    // +0x08  == CSymParser::V2 (0x13ba10; 0 = give up)
};

// --- vtable catalog ---

#endif // REZ_REZITMOWNER_H
