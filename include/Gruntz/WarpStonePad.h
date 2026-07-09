// WarpStonePad.h - the warp-stone pad tile-logic leaf (C:\Proj\Gruntz), a CUserLogic
// game-object (RTTI game class, vtable 0x5e71ac). Extracted from the former
// UserLogic.cpp-local view so the leaf dtor (0x10fc0) homes onto the real class.
// Only offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CWARPSTONEPAD_H
#define GRUNTZ_CWARPSTONEPAD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CWarpStonePad : CUserLogic)

SIZE(CWarpStonePad, 0x54);
VTBL(CWarpStonePad, 0x001e71ac); // vtable_names -> code (RTTI game class)
class CWarpStonePad : public CUserLogic {
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
public:
    TILE_LOGIC_TAIL
public:
    CWarpStonePad(CGameObject* obj); // 0x10d650
    virtual ~CWarpStonePad() OVERRIDE;
    static void InitActReg();   // 0x10d840
    void FireWarp(i32 coord);   // 0x10d8c0 (vtable slot 4)
    static void RegisterActs(); // 0x10da20
    i32 AdvanceAnim();          // 0x10dc20
    char m_pad40[0x54 - 0x40];  // +0x40  (unmodeled leaf tail; size 0x54 proven from
                                //         the state pump's `new CWarpStonePad` = new(0x54))
};

#endif // GRUNTZ_CWARPSTONEPAD_H
