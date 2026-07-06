// ObjectDropper.h - the object-dropper tile-logic object (C:\Proj\Gruntz), a
// CUserLogic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed
// here; the ctor (0xc59f0) remains the @stub backlog in
// src/Stub/ObjectDropper.cpp. Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_COBJECTDROPPER_H
#define GRUNTZ_COBJECTDROPPER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

class CObjectDropper : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    LogicTypeId GetTypeTag();           // 0x124a0
    CObjectDropper(CGameObject* obj);   // 0xc59f0 (folds CUserLogic(obj) + the drop setup)
    virtual ~CObjectDropper() OVERRIDE; // 0x124f0 (folds the CUserLogic teardown)
    i32 Update();                       // 0xc62e0 (per-frame drop tick + drift/wrap)

    i32 m_geomId; // +0x40  geometry id (m_38->m_1b4 snapshot)
    char m_pad44[0x58 - 0x44];
    double m_speed;      // +0x58  per-frame speed (32.0 / time-per-tile)
    double m_posX;       // +0x60  accumulated x (double)
    double m_posY;       // +0x68  accumulated y (double)
    i32 m_travelDx;      // +0x70  travel dx (-1/0/1)
    i32 m_travelDy;      // +0x74  travel dy (-1/0/1)
    i32 m_lastDropTileX; // +0x78  last-drop tile x (-1)
    i32 m_lastDropTileY; // +0x7c  last-drop tile y (-1)
    i32 m_scrollMode;    // +0x80  scroll mode (0/1)
    char m_pad84[0x88 - 0x84];
    i64 m_lastDropTime; // +0x88  last-drop timestamp (64-bit)
    i64 m_dropInterval; // +0x90  drop interval (64-bit)
};
SIZE(CObjectDropper, 0x98);

#endif // GRUNTZ_COBJECTDROPPER_H
