// CGuardPoint.h - the guard-point marker (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the
// ctor (0xae5f0) remains the @stub backlog in src/Stub/CGuardPoint.cpp. Offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_CGUARDPOINT_H
#define GRUNTZ_CGUARDPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CGuardPoint : public CUserLogic {
public:
    ~CGuardPoint(); // 0x10410 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
SIZE(CGuardPoint, 0x54);

#endif // GRUNTZ_CGUARDPOINT_H
