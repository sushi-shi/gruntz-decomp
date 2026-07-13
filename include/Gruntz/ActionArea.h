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
    // ApplyColor (0x8580): re-name the bound object's sprite for the owning team
    // (owner 1 -> "GAME_ACTIONAREA_BLUE", owner 2 -> "GAME_ACTIONAREA_RED"), reset
    // its image set's pixel-format types (SetAllTypes 8), clear the object's
    // active bit. owner outside {1,2} is rejected (returns 0).
    i32 ApplyColor(i32 owner);
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    // 0x00007f80 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00007f80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ACTIONAREA;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    virtual ~CActionArea() OVERRIDE; // 0x7fd0 (folds the CUserLogic teardown)

    char m_pad40[0x54 - 0x40];
    i32 m_54; // +0x54
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
};
VTBL(CActionArea, 0x001e7004);

#endif // GRUNTZ_CACTIONAREA_H
