#ifndef GRUNTZ_CWARPSTONEPAD_H
#define GRUNTZ_CWARPSTONEPAD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CWarpStonePad : CUserLogic)

class CWarpStonePad : public CUserLogic, public CWapX {
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in UserLogic.cpp)
    RVA(0x00010f00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARPSTONEPAD;
    }

public:
public:
    CWarpStonePad(CGameObject* obj); // 0x10d650
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    static void InitActReg();                     // 0x10d840
    virtual void FireActivation(i32 id) OVERRIDE; // 0x10d8c0 (vtable slot 4)
    static void RegisterActs();                   // 0x10da20
    i32 AdvanceAnim();                            // 0x10dc20
    //         the state pump's `new CWarpStonePad` = new(0x54))
};
SIZE(0x54);

typedef i32 (CUserLogic::*WarpStonePadHandler)();
struct CWarpStonePadActEntry {
    WarpStonePadHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CWARPSTONEPAD_H
