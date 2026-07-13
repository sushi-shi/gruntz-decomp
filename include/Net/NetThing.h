// NetThing.h - CNetThing, the per-session net object whose destructor lives at 0xc5280.
//
// THE shared definition. It used to be declared TWICE, in two .cpp files, with two
// DIFFERENT shapes - and that divergence was a guaranteed link break:
//
//   * NetThingDtor.cpp    `struct CNetThing : public CPtrList { ~CNetThing(); }`
//     CPtrList (MFC) has a virtual destructor, so CNetThing's is implicitly virtual too
//     and the DEFINITION cl emits at 0xc5280 is ??1CNetThing@@UAE@XZ.
//   * MultiStartDlgRoster.cpp  `struct CNetThing { ~CNetThing(); }`  (no base)
//     A non-polymorphic view, so its teardown call referenced ??1CNetThing@@QAE@XZ -
//     a symbol nothing defines. objdiff masked the reloc; the linker would not have.
//
// One shape, one mangled dtor. (Identity note: 0xc5280 is the CKeyedList teardown - it
// runs CKeyedList::Clear then chains the ~CPtrList base - so CNetThing and CKeyedList are
// very likely the same engine class. That unification is a cross-unit rename tracked
// separately; what is fixed here is the two-shapes-one-symbol defect.)
#ifndef GRUNTZ_NET_NETTHING_H
#define GRUNTZ_NET_NETTHING_H

#include <Mfc.h> // real MFC CPtrList (the base whose virtual dtor 0x1b48c6 the chain hits)
#include <rva.h>

SIZE_UNKNOWN(CNetThing); // dtor-only shape; retail size TBD
struct CNetThing : public CPtrList {
    ~CNetThing(); // 0xc5280 - implicitly VIRTUAL (CPtrList's dtor is); ??1CNetThing@@UAE@XZ
};

#endif // GRUNTZ_NET_NETTHING_H
