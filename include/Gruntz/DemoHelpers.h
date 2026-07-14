// DemoHelpers.h - the small helper/orphan types that live in the demo feature TU
// (Demo.cpp). Modeled in a header (not as .cpp-local views) per rule 0; each carries
// its unrecovered-identity flag so the knowledge travels. Field names are
// placeholders; only offsets + emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_DEMOHELPERS_H
#define GRUNTZ_GRUNTZ_DEMOHELPERS_H

#include <Ints.h>
#include <rva.h>

struct CSpriteFactoryHolder; // CDemoSetup::m_c bound world holder (its +0x8 factory)

// CDemoSetup - the attract/demo-mode actor seeding. this->m_c is the world holder
// (the canonical CSpriteFactoryHolder; its +0x8 IS the typed m_8 factory).
// @identity-TODO: OWNER class genuinely unrecovered - FULL xref chase run + dead-ended:
//   (1) sema xref 0x3c070: no rel32 caller; only reached via the thunk @0x37fb.
//   (2) sema xref 0x37fb (the thunk): no rel32 caller AND no data-ref (not in any
//       vtable/command table) - a pure fn-ptr trampoline off unreconstructed data.
//   (3) no callee-mangled-type / new-site / RTTI COL to read (SetupDemoActors is XZ
//       and the owner is never allocated on a findable path).
//   (4) Ghidra fields: `this` touches only m_c@+0xc (a CSpriteFactoryHolder) - the
//       +0xc world-context slot fits the CGameObject/AnimWorkerObj/CUserLogic m_0c
//       family but nothing disambiguates. No further technique applies.
class CDemoSetup {
public:
    i32 SetupDemoActors(); // 0x3c070
    char m_pad0[0xc];
    CSpriteFactoryHolder* m_c; // +0xc  bound world holder
};

// Orient3 - a real 12-byte three-int orientation {facing, sub, dir}; StepA/StepB
// (__thiscall(count)) walk the CW/CCW rotation-transition tables (0x60d008 /
// 0x60d078). @orphan - class NAME genuinely unrecovered; FULL xref chase run:
//   (1) StepA disasm PROVES `this` is a bare 3-int struct (mov eax,[ecx]; mov
//       edx,[ecx+4]; ...; mov [ecx+8],esi - no vptr), so it is NOT itself a vtable
//       method; it operates on an embedded orientation FIELD of some caller.
//   (2) sema xref 0x3c850/0x3c8a0: reached only via jmp-thunks @0x27e8/0x3aad, which
//       have NO rel32 caller and NO data-ref -> dead-end fn-ptr trampolines.
//   (3) the "??_7CBoomerang@@6B@+0x44" data-ref the scan first surfaced is the
//       ADJACENT slot-17 thunk 0x27de (CProjectile::LoadProjectileSprites), a false
//       positive one thunk before StepA's 0x27e8 - NOT StepA.
//   The CW/CCW spin tables + the CBoomerang/CProjectile thunk-band proximity suggest
//   a projectile spin field, but no technique pins the owner class NAME.
struct Orient3 {
    i32 m_0, m_4, m_8;
    void StepA(i32 count); // 0x3c850
    void StepB(i32 count); // 0x3c8a0
};

// COwnerWithSubs - name-holder for the two compiler-generated member destructors
// (0x3cbc0 / 0x3cbf0) that run the two-phase virtual-base teardown of the bute
// editor's embedded fstream members (ifstream vbase adjust 0xc / ofstream 0x8).
// A synthetic thunk pair, not a real engine class; no data members.
struct COwnerWithSubs {
    void DtorSubC(); // 0x3cbc0  (`this` is an ifstream; vbase adjust 0xc)
    void DtorSub8(); // 0x3cbf0  (`this` is an ofstream; vbase adjust 0x8)
};

#endif // GRUNTZ_GRUNTZ_DEMOHELPERS_H
