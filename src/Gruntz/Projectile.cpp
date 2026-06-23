// Projectile.cpp - the CProjectile game-object (C:\Proj\Gruntz). Continues the
// CUserBase/CUserLogic/CMovingLogic hierarchy (see include/Gruntz/Projectile.h).
//
// CProjectile::CProjectile() (0x126e0) is the no-arg ctor: it folds the inline
// CMovingLogic init (the +0x38..+0x10c motion ints + the twelve default-bound
// doubles), constructs the +0x204 tracked-hit CObList, and stamps its vftable.
// Like the rest of the family it constructs a throwing CUserBaseLink (in the
// CUserLogic base) + a CObList, so MSVC emits the /GX EH frame -> built eh.
#include <Gruntz/Projectile.h>
#include <rva.h>

// The shared default-bound doubles the CMovingLogic ctor copies into the twelve
// coordinate-bound members (retail .rdata 0x5f04b0 / 0x5f04b8). Defined here with
// DATA() pins so the ctor's dword loads reloc-mask against them.
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. Give CMovingLogic / CProjectile real vftables in
// this TU so the inline ctors emit their vptr stores. Bodies are not matched.
// ---------------------------------------------------------------------------
CMovingLogic::~CMovingLogic() {}
int CMovingLogic::MovingLogicVfunc() {
    return 0;
}

CProjectile::~CProjectile() {}
int CProjectile::ProjectileVfunc() {
    return 0;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): body, all
// field offsets, the double init and the CObList construction are byte-identical;
// the only residue is the CObList __ehfuncinfo state id (recompile 1 vs retail 2,
// the `mov [esp+0x18],N`). Resolves only when the full CMovingLogic/CProjectile
// family lives in this TU (its out-of-line ctor 0x13940 is still a Stub) so the
// funcinfo state-numbering base aligns. ~99% on a documented wall.
RVA(0x000126e0, 0x1fc)
CProjectile::CProjectile() {}
