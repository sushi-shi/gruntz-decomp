// SingleFrameMessage.h - a single-frame message eyecandy game-object
// (C:\Proj\Gruntz), a CUserLogic leaf in the same animation family as
// CSimpleAnimation / CAniCycle: a per-class activation registry
// (g_singleFrameActReg @0x645ef0) bound by RegisterActs to a per-frame
// AdvanceAnim handler under the shared activation-name key "A".
//
// CSingleFrameMessage : CUserLogic. Only offsets / code bytes are load-bearing;
// names are placeholders for the recovered engine identities.
#ifndef GRUNTZ_CSINGLEFRAMEMESSAGE_H
#define GRUNTZ_CSINGLEFRAMEMESSAGE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSingleFrameMessage : CUserLogic)

class CSingleFrameMessage : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000f580, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SINGLEFRAMEMESSAGE;
    } // slot 2
    TILE_LOGIC_TAIL
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
    virtual ~CSingleFrameMessage()
        OVERRIDE; // empty vtable-anchor dtor (folds the CUserLogic teardown)
    char m_pad40
        [0x54 - 0x40]; // +0x40..0x53 (leaf tail; sizeof from `new CSingleFrameMessage` @0xa9b80)
};
VTBL(CSingleFrameMessage, 0x1e864c);
SIZE(CSingleFrameMessage, 0x54);

// The activation-registry handler-entry record (the .data CActReg row; 4-byte PMF).
typedef i32 (CSingleFrameMessage::*SingleFrameHandler)();
struct CSingleFrameActEntry {
    SingleFrameHandler m_fn;
};

#endif // GRUNTZ_CSINGLEFRAMEMESSAGE_H
