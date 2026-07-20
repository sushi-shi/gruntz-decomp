#ifndef GRUNTZ_GRUNTZ_DEMOHELPERS_H
#define GRUNTZ_GRUNTZ_DEMOHELPERS_H

#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr; // CDemoSetup::m_c bound world holder (its +0x8 factory)

// CDemoSetup - the attract/demo-mode actor seeding. this->m_c is the world holder
// (the canonical CDDrawSurfaceMgr; its +0x8 IS the typed m_8 factory).
// @identity-TODO: OWNER class genuinely unrecovered - FULL xref chase run + dead-ended:
//   (1) sema xref 0x3c070: no rel32 caller; only reached via the thunk @0x37fb.
//   (2) sema xref 0x37fb (the thunk): no rel32 caller AND no data-ref (not in any
//       vtable/command table) - a pure fn-ptr trampoline off unreconstructed data.
//   (3) no callee-mangled-type / new-site / RTTI COL to read (SetupDemoActors is XZ
//       and the owner is never allocated on a findable path).
//   (4) Ghidra fields: `this` touches only m_c@+0xc (a CDDrawSurfaceMgr) - the
//       +0xc world-context slot fits the CGameObject/AnimWorkerObj/CUserLogic m_0c
//       family but nothing disambiguates. No further technique applies.
class CDemoSetup {
public:
    i32 SetupDemoActors(); // 0x3c070
    char m_pad0[0xc];
    CDDrawSurfaceMgr* m_c; // +0xc  bound world holder
};

struct Orient3 {
    i32 m_0, m_4, m_8;
    void StepA(i32 count); // 0x3c850
    void StepB(i32 count); // 0x3c8a0
};

struct COwnerWithSubs {
    void DtorSubC(); // 0x3cbc0  (`this` is an ifstream; vbase adjust 0xc)
    void DtorSub8(); // 0x3cbf0  (`this` is an ofstream; vbase adjust 0x8)
};

#endif // GRUNTZ_GRUNTZ_DEMOHELPERS_H
