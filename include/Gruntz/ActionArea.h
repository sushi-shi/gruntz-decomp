// ActionArea.h - the action-area trigger tile-logic game-object (C:\Proj\Gruntz),
// a CUserLogic leaf (RTTI .?AVCActionArea@@). The 1-arg ctor (0x7da0) folds the
// shared CUserLogic(obj) prologue then a per-class tail; the leaf state begins at
// +0x54. Offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CACTIONAREA_H
#define GRUNTZ_CACTIONAREA_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

SIZE_UNKNOWN(CActionArea);
class CActionArea : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CActionArea(CGameObject* obj); // 0x7da0
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    LogicTypeId GetTypeTag();        // 0x7f80
    virtual ~CActionArea() OVERRIDE; // 0x7fd0 (folds the CUserLogic teardown)

    char m_pad40[0x54 - 0x40];
    i32 m_54; // +0x54
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
};

#endif // GRUNTZ_CACTIONAREA_H
