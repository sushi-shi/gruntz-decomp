// CSingleFrameMessage.h - a single-frame message eyecandy game-object
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
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ab710
    // The per-frame handler (@0x0ab910); Ghidra did not carve it (recovery gap),
    // so it is declared only - RegisterActs takes its address as a reloc-masked
    // handler-store operand.
    i32 AdvanceAnim();
};

#endif // GRUNTZ_CSINGLEFRAMEMESSAGE_H
