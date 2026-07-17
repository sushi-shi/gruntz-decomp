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
class CWarpStonePad : public CUserLogic, public CWapX {
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in UserLogic.cpp)
    RVA(0x00010f00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARPSTONEPAD;
    }

public:
public:
    CWarpStonePad(CGameObject* obj); // 0x10d650
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    static void InitActReg();                     // 0x10d840
    virtual void FireActivation(i32 id) OVERRIDE; // 0x10d8c0 (vtable slot 4)
    static void RegisterActs();                   // 0x10da20
    i32 AdvanceAnim();                            // 0x10dc20
                               //         the state pump's `new CWarpStonePad` = new(0x54))
};

// The activation-registry entry record (the .data CActReg row): its first dword
// receives the per-frame handler PMF (4-byte code pointer on this complete
// single-inheritance class); FireActivation dispatches it thiscall on the leaf.
typedef i32 (CUserLogic::*WarpStonePadHandler)();
struct CWarpStonePadActEntry {
    WarpStonePadHandler m_fn;
};

#endif // GRUNTZ_CWARPSTONEPAD_H
