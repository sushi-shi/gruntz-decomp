#ifndef GRUNTZ_CSINGLEFRAMEMESSAGE_H
#define GRUNTZ_CSINGLEFRAMEMESSAGE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSingleFrameMessage : CUserLogic)

class CSingleFrameMessage : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000f580, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SINGLEFRAMEMESSAGE;
    } // slot 2
public:
    CSingleFrameMessage(CGameObject* obj); // 0x0ab310 (ctor body in UserLogic.cpp)
    // Construct the class's activation-coordinate registry singleton
    // (g_singleFrameActReg @0x645ef0) over the fixed [2000, 2010] range. Static.
    static void InitActReg(); // 0x0ab530
    // Resolve the registry entry for id; run its bound handler as a PMF on this
    // (ResolveEntry inlined twice). 0x0ab5b0.
    virtual void FireActivation(i32 id) OVERRIDE;
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ab710
    // The per-frame handler (@0x0ab910); Ghidra did not carve it (recovery gap),
    // so it is declared only - RegisterActs takes its address as a reloc-masked
    // handler-store operand.
    i32 AdvanceAnim();
    // NO user-declared dtor: retail 0xf640 is COMPILER-GENERATED (implicit; pin in
    // SingleFrameMessage.cpp).
};
SIZE(0x54);

typedef i32 (CUserLogic::*SingleFrameHandler)();
struct CSingleFrameActEntry {
    SingleFrameHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CActReg (extern below)
extern CActReg g_singleFrameActReg; // 0x00245ef0

#endif // GRUNTZ_CSINGLEFRAMEMESSAGE_H
