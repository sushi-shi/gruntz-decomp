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
    CObjectDropper(CGameObject* obj); // 0xc59f0 (folds CUserLogic(obj) + the drop setup)
    ~CObjectDropper();                // 0x124f0 (folds the CUserLogic teardown)

    i32 m_40; // +0x40  geometry id (m_38->m_1b4 snapshot)
    char m_pad44[0x58 - 0x44];
    double m_58; // +0x58  per-frame speed (divisor / time)
    double m_60; // +0x60  accumulated x (double)
    double m_68; // +0x68  accumulated y (double)
    i32 m_70;    // +0x70  travel dx (-1/0/1)
    i32 m_74;    // +0x74  travel dy (-1/0/1)
    i32 m_78;    // +0x78  (-1)
    i32 m_7c;    // +0x7c  (-1)
    i32 m_80;    // +0x80  scroll mode (0/1)
    char m_pad84[0x88 - 0x84];
    double m_88; // +0x88  (cleared)
    double m_90; // +0x90  (cleared)
};
SIZE(CObjectDropper, 0x98);

#endif // GRUNTZ_COBJECTDROPPER_H
