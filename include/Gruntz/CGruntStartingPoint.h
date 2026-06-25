// CGruntStartingPoint.h - the grunt starting-point marker (C:\Proj\Gruntz), a
// CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is
// reconstructed here; the ctor (0x3df30) remains the @stub backlog in
// src/Stub/CGruntStartingPoint.cpp. Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CGruntStartingPoint : public CUserLogic {
public:
    ~CGruntStartingPoint(); // 0x10670 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
SIZE(CGruntStartingPoint, 0x54);

#endif // GRUNTZ_CGRUNTSTARTINGPOINT_H
