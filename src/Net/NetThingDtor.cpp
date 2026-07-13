// NetThingDtor.cpp - a /GX-framed C++ destructor (0xc5280): the body runs a
// this-cleanup that may throw (EH state 0), then the base-subobject destructor
// (EH state -1) unwinds the fs:0 frame. Built with the `eh` profile so the
// compiler emits the matching SEH scope table + state machine; the cleanup and
// base dtor are external (reloc-masked). Best-guess class; offsets/calls are what
// is load-bearing.
#include <rva.h>
// Base subobject whose destructor (0x1b48c6) the derived dtor chains to: the REAL MFC
// CPtrList. 0x1b48c6 is the band-A dtor, and band A's ctor (0x1b4867) stamps vtable
// 0x1eb054, whose slot-0 GetRuntimeClass returns the CRuntimeClass named "CPtrList" -
// so this is CPtrList, not CObList (whose own dtor is 0x1b5a2b, band B). Nothing is
// COMDAT-folded here: MSVC5 has no /OPT:ICF and retail carries three distinct list
// bands that merely share identical code. (Was a mis-guessed CInternetSession/
// CNetThingBase local placeholder, then a GzObList view.)
#include <Gruntz/GruntzCmdMgr.h>

// The keyed-list the dtor clears (Clear @0x379a0): the canonical CKeyedList (real
// shared struct, <Net/KeyedList.h>). 0xc5280 IS CKeyedList::~CKeyedList (Clear() + the
// ~CPtrList base chain @0x1b48c6); the reinterpret here is the CNetThing<->CKeyedList
// identity (both share the CPtrList/GzObList base) - full unification (CNetThing is
// referenced as an opaque child by netcmdslot/multistartdlgroster) is a cross-unit
// rename, deferred.
#include <Net/KeyedList.h>

#include <Net/NetThing.h> // THE shared CNetThing (was defined locally here AND, with a
                          // divergent non-polymorphic shape, in MultiStartDlgRoster.cpp)

RVA(0x000c5280, 0x49)
CNetThing::~CNetThing() {
    ((CKeyedList*)this)->Clear();
}
