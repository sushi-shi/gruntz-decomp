// ProjActRegistry.cpp - the projectile-action type registry (C:\Proj\Gruntz).
//
// A global registry object (g_projReg @VA 0x629388) maps projectile-action keys to
// cache slots (g_projActCache / g_projActAllocResult) via a grid + bute-tree lookup.
// This TU sits between Utils/ApplyRange and Wap32/ZVec in retail-RVA order. Field
// names are placeholders; only OFFSETS + code bytes are load-bearing.
#include <Gruntz/UserLogic.h>

// The leaf game-object whose dtor opens this TU. A CUserLogic leaf: its only
// destructible member is the inherited +0x18 EngStr link, so the dtor folds the
// bare CUserLogic teardown (the established /GX leaf-dtor archetype).
class CProjActOwner : public CUserLogic {
public:
    ~CProjActOwner();
};

// The global registry object at VA 0x629388. SetActiveRange reaches it through an
// ILT thunk (0x3742) -> modeled NO-body so the call reloc-masks.
struct CProjReg {
    void SetActiveRange(i32 lo, i32 hi); // 0x3742 (ILT thunk; reloc-masked)
};
DATA(0x00229388)
extern CProjReg g_projReg;

// 0x7fd0: ~CProjActOwner - the bare CUserLogic leaf teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (~EngStr @0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame.
RVA(0x00007fd0, 0x44)
CProjActOwner::~CProjActOwner() {}

// 0x8060: register the default projectile-action id range [0x7d0, 0x7da] on the
// global registry.
RVA(0x00008060, 0x15)
void ProjActRegisterDefaults() {
    g_projReg.SetActiveRange(0x7d0, 0x7da);
}
