#include <rva.h>
// CProjectile.cpp - remaining engine-label stubs for CProjectile.
//
// The no-arg ctor (0x126e0) is reconstructed in src/Gruntz/Projectile.cpp (its
// own EH unit). These two members are still unmatched stubs.

class CProjectile {
public:
    void CProjectile_0dec60(int);
    void LoadProjectileSprites(int, int, int, int, int, int, int);
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
void CProjectile::CProjectile_0dec60(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000df050, 0x6ba)
void CProjectile::LoadProjectileSprites(int, int, int, int, int, int, int) {}
