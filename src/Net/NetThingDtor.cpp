// NetThingDtor.cpp - a /GX-framed C++ destructor (0xc5280): the body runs a
// this-cleanup that may throw (EH state 0), then the base-subobject destructor
// (EH state -1) unwinds the fs:0 frame. Built with the `eh` profile so the
// compiler emits the matching SEH scope table + state machine; the cleanup and
// base dtor are external (reloc-masked). Best-guess class; offsets/calls are what
// is load-bearing.
#include <rva.h>
// Base subobject whose destructor (0x1b48c6) the derived dtor chains to: the canonical
// GzObList (== the COMDAT-folded ~CObList, library_labels ??1GzObList@@QAE@XZ). The real
// shared struct, not a local view - CNetThing derives it and the chained base dtor binds/
// exempts. (Was a mis-guessed CInternetSession/CNetThingBase local placeholder.)
#include <Gruntz/GruntzCmdMgr.h>

// The keyed-list the dtor clears (Clear @0x379a0): the canonical CKeyedList (real
// shared struct, <Net/KeyedList.h>). 0xc5280 IS CKeyedList::~CKeyedList (Clear() + the
// ~CObList base chain @0x1b48c6); the reinterpret here is the CNetThing<->CKeyedList
// identity (both share the CObList/GzObList base) - full unification (CNetThing is
// referenced as an opaque child by netcmdslot/multistartdlgroster) is a cross-unit
// rename, deferred.
#include <Net/KeyedList.h>

struct CNetThing : GzObList {
    ~CNetThing();
};
SIZE_UNKNOWN(CNetThing); // dtor-only view; retail size TBD

RVA(0x000c5280, 0x49)
CNetThing::~CNetThing() {
    ((CKeyedList*)this)->Clear();
}
