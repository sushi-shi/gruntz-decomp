#ifndef GRUNTZ_STUB_CPROJECTILE_H
#define GRUNTZ_STUB_CPROJECTILE_H
#include <rva.h>
#include <Stub/CMovingLogic.h>
// CProjectile : CMovingLogic (RTTI). sizeof 0x228.
class CProjectile : public CMovingLogic {
public:
    void CProjectile_0126e0();
    void CProjectile_0dec60(int);
    void LoadProjectileSprites(int, int, int, int, int, int, int);
    char m_size_pad[0x1e8]; // own region over CMovingLogic (0x40)
};
SIZE(CProjectile, 0x228);
#endif
