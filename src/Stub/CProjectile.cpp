#include <rva.h>
// CProjectile.cpp - remaining engine-label stubs for CProjectile.
//
// The no-arg ctor (0x126e0) is reconstructed in src/Gruntz/Projectile.cpp (its
// own EH unit). These two members are still unmatched stubs.

class CProjectile {
public:
    void CProjectile_0dec60(i32);
    void LoadProjectileEffects();
};

// @confidence: med
// @source: rtti-vptr
// @stub
// The 1-arg CProjectile ctor (`CProjectile(CGameObject* owner)`, 597 B). DEFERRED
// to the final sweep: a big entangled ctor with 5 un-modeled engine collaborators
// + 3 external object layouts. Shape (mapped from the disasm, for the redo):
//   - chains CUserLogic(owner) OUT-OF-LINE (call 0x58cd0), NOT CMovingLogic() -
//     so it does NOT force the 0x13940 standalone;
//   - builds the +0x38 motion band via the field-init helper 0x136d0 (__thiscall,
//     no EH) + the 11-double setter 0x58bc0 (__thiscall, returns 1);
//   - conditionally seeds the four bounds at +0xa8/+0xb0/+0xc0/+0xc8 from the
//     config object owner->m_7c ints (0 -> default MIN/MAX, else (double)int);
//     int->double via fild/fstp;
//   - scales owner->m_168 by the double const 0x5eaa88, reads owner m_5c/m_60/
//     m_164/m_168 into SetCoords; stores the default-Z qword 0x5f04e8 into the
//     +0xd8/+0xe0/+0xe8 doubles; reads the spawn qword 0x645588;
//   - calls 0x16ea90 (__thiscall, no args), sets m_148/m_14c=0, m_10->+0xe4=7,
//     m_150/m_154=owner, m_158=owner->m_7c; builds the +0x204 CObList (block 10);
//   - stamps the CProjectile vptr; ORs owner flags m_8|=0x2000002, m_40|=1; a
//     conditional m_74/pose update; rep stos 7 dwords at +0x1e0; m_1fc/m_200=0.
// /GX EH frame (throwing CUserLogic base + CObList): EH states 1/2/3.
RVA(0x000dec60, 0x255)
void CProjectile::CProjectile_0dec60(i32) {}

// @confidence: med
// @source: string-xref
// @stub
// LoadProjectileEffects (1781 B). DEFERRED to the final sweep - a dense x87 wall
// (worse than the sprites loader): per-frame trajectory advance + effect-tier
// select. Non-/GX (`sub esp,0x10; push ebx/ebp/esi; mov esi,ecx; push edi`).
// Shape mapped for the redo:
//   - early-out unless m_1dc==0 (the "effects loaded" latch LoadProjectileSprites
//     leaves 0); on kind==0x16 (WINGZ, m_170) + the owner inside the level bounds
//     (g_mgrSettings +0x13c/+0x140/+0x144/+0x148) it loops the launch sound
//     (LaunchSound thunk 0x2b5d -> 0xe2190) else stops it (m_200->StopAndRewind
//     0x135380); sets m_1dc=1;
//   - integrates the parabola: fild g_645584 (draw-clock delta) * m_1b0 + m_198*
//     m_1c0 + m_1a0 -> m_1a0 (and the m_1b8/m_1c8/m_1a8 twin), __ftol via 0x11f570;
//   - clamps m_1d0/m_1d4 against m_17c/m_180 via the 0x5eaa90 sign fcomp ladders;
//   - THE WALL: when m_1d8 set, an fsqrt-normalised distance is compared against
//     m_188 scaled by SIX .rdata doubles (0x5eaa98..0x5eaad0) in a long
//     fld/fmul/fcompp ladder (0xe0000..0xe01f0) selecting the impact-effect tier -
//     a fxch/fcompp schedule not steerable from C source;
//   - the tail (0xe0242+) spawns the Particlez / LEVEL_DEATHSPLASH / GAME_WATER
//     effects via a jump-table switch (switchdataD_004e03f8) + registry lookups.
RVA(0x000dfd00, 0x6f5)
void CProjectile::LoadProjectileEffects() {}
