// CObjectDropper.h - the object-dropper tile-logic object (C:\Proj\Gruntz), a
// CUserLogic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed
// here; the ctor (0xc59f0) remains the @stub backlog in
// src/Stub/CObjectDropper.cpp. Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_COBJECTDROPPER_H
#define GRUNTZ_COBJECTDROPPER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CObjectDropper : public CUserLogic {
public:
    ~CObjectDropper(); // 0x124f0 (folds the CUserLogic teardown)
    char m_pad40[0x98 - 0x40];
};
SIZE(CObjectDropper, 0x98);

#endif // GRUNTZ_COBJECTDROPPER_H
