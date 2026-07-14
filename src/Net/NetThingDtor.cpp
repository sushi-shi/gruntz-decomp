// NetThingDtor.cpp - CKeyedList::~CKeyedList (0xc5280): a /GX-framed C++ destructor
// that runs Clear() (EH state 0, may throw), then the base-subobject ~CPtrList (EH state
// -1) unwinds the fs:0 frame. Built with the `eh` profile so the compiler emits the
// matching SEH scope table + state machine; Clear and the base dtor are external
// (reloc-masked). The former CNetThing view (a separate CPtrList-derived shape that hosted
// this dtor while CKeyedList had none) is DISSOLVED: 0xc5280 is CKeyedList's own dtor, so
// it lives on the real container - CLatencyList::~CLatencyList and CMultiStartDlg::
// DestroyWindow now reach it cast-free (byte-neutral: adding the CKeyedList dtor emits no
// container vtable, so the BuildSlotList ctor 0xc1e60 stays EXACT). NetThing.h is deleted.
#include <rva.h>
// Base subobject whose destructor (0x1b48c6) the derived dtor chains to: the REAL MFC
// CPtrList. 0x1b48c6 is the band-A dtor, and band A's ctor (0x1b4867) stamps vtable
// 0x1eb054, whose slot-0 GetRuntimeClass returns the CRuntimeClass named "CPtrList" -
// so this is CPtrList, not CObList (whose own dtor is 0x1b5a2b, band B). Nothing is
// COMDAT-folded here: MSVC5 has no /OPT:ICF and retail carries three distinct list
// bands that merely share identical code.
#include <Gruntz/GruntzCmdMgr.h>

// The keyed-list the dtor clears (Clear @0x379a0): the canonical CKeyedList
// (<Net/KeyedList.h>), which now declares this dtor.
#include <Net/KeyedList.h>

RVA(0x000c5280, 0x49)
CKeyedList::~CKeyedList() {
    Clear();
}
