// NetThingDtor.cpp - a /GX-framed C++ destructor (0xc5280): the body runs a
// this-cleanup that may throw (EH state 0), then the base-subobject destructor
// (EH state -1) unwinds the fs:0 frame. Built with the `eh` profile so the
// compiler emits the matching SEH scope table + state machine; the cleanup and
// base dtor are external (reloc-masked). Best-guess class; offsets/calls are what
// is load-bearing.
#include <rva.h>

// Base subobject whose destructor (0x1b48c6) the derived dtor chains to.
struct CNetThingBase {
    ~CNetThingBase(); // 0x1b48c6 (external, reloc-masked)
};
SIZE_UNKNOWN(CNetThingBase); // dtor-chain view; retail size TBD

// The keyed-list the dtor clears (Clear @0x379a0); TU-local method view of the real
// header-less CKeyedList (keyedlist unit).
class CKeyedList {
public:
    void Clear();
};

struct CNetThing : CNetThingBase {
    ~CNetThing();
};
SIZE_UNKNOWN(CNetThing); // dtor-only view; retail size TBD

RVA(0x000c5280, 0x49)
CNetThing::~CNetThing() {
    ((CKeyedList*)this)->Clear();
}
