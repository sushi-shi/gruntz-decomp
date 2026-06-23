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
RVA(0x000dec60, 0x255)
void CProjectile::CProjectile_0dec60(int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000df050, 0x6ba)
void CProjectile::LoadProjectileSprites(int, int, int, int, int, int, int) {}
