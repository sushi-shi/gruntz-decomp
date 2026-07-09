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
    // 0x000124a0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x000124a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE { return LOGIC_OBJECTDROPPER; }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    CObjectDropper(CGameObject* obj);   // 0xc59f0 (folds CUserLogic(obj) + the drop setup)
    virtual ~CObjectDropper() OVERRIDE; // 0x124f0 (folds the CUserLogic teardown)
    i32 Update();                       // 0xc62e0 (per-frame drop tick + drift/wrap)
    void FireAct(i32 actId);            // 0xc5f80 (look up + fire the registered act handler)
    // The slot-1 serialize impl (modeled as a plain method so its ?Serialize name + RVA
    // pin; the vtable slot is reloc-masked, like CRollingBall::Serialize).
    i32 Serialize(struct CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0xc6680

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
VTBL(CObjectDropper, 0x001e7a9c);
SIZE(CObjectDropper, 0x98);

#endif // GRUNTZ_COBJECTDROPPER_H
