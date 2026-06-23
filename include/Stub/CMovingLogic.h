#ifndef GRUNTZ_STUB_CMOVINGLOGIC_H
#define GRUNTZ_STUB_CMOVINGLOGIC_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CMovingLogic : CUserLogic (RTTI). Size AMBIGUOUS, so NO SIZE assert: the
// vtable-clean new-site gave 0x8d8, but that's a CGrunt allocation leaked onto
// CMovingLogic's transient base-vtable. CProjectile : CMovingLogic is 0x228, so
// CMovingLogic <= 0x228; real size TBD (size from RTTI base-offset, not `new`).
// Modeled as CUserLogic (0x40) lower bound; derived classes pad to their own size.
class CMovingLogic : public CUserLogic {
public:
    CMovingLogic();
};
#endif // GRUNTZ_STUB_CMOVINGLOGIC_H
