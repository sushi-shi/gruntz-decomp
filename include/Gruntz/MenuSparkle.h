// MenuSparkle.h - the menu-sparkle eyecandy tile-logic leaf (C:\Proj\Gruntz), a
// CUserLogic game-object (vtable 0x5e82dc). The CANONICAL CMenuSparkle (ctor +
// full vtable); extracted from the former UserLogic.cpp-local view so the leaf dtor
// (0x101b0) homes onto the real class. NOTE: distinct from the Grunt.h-world serialize
// view in <Gruntz/MenuSparkleSerial.h> (documented dual-model, never coexist in a TU).
// Only offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CMenuSparkle : CUserLogic)

class CMenuSparkle : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CMenuSparkle(CGameObject* obj); // 0x0adbe0
    virtual ~CMenuSparkle() OVERRIDE;
    // The per-frame handler (@0x0ae2a0): tick the aux flicker countdown, advance the
    // +0x1a0 anim on expiry, then re-arm the random flicker delay.
    i32 AdvanceAnim();
    i32 m_40; // +0x40
};
VTBL(CMenuSparkle, 0x1e82dc);

#endif // GRUNTZ_CMENUSPARKLE_H
