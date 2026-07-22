#ifndef GRUNTZ_HELPSTATE_H
#define GRUNTZ_HELPSTATE_H

#include <Ints.h>
#include <rva.h> // OVERRIDE
#include <Gruntz/State.h>

class CHelpState : public CState {
public:
    virtual ~CHelpState() OVERRIDE; // slot 0  (0x8cf30)
    // slot 1  0x095090 (HelpState.cpp; retail ??_7CHelpState slot 1 = ILT
    // 0x2f40 -> 0x95090, ex "LoadAssets") - the help asset loader.
    virtual i32 LoadGameAssetNamespaces(i32, i32, i32) OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;    // slot 2  (0x95120, unreconstructed)
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14

    // (The ex "LoadAssets" decl is GONE - it IS the slot-1 override above; its body
    // chains the base default via the qualified CState::LoadGameAssetNamespaces().)

    // The TRUE object is 0x1b8: CGruntzMgr::TransitionState (0x8b960) does `push 0x1b8;
    // call ??2` @0x8be4c, then the inline `mov [esi],??_7CHelpState@@6B@` (0x5e9dfc) stamp.
    //
    // This pad used to be written `[0x1b8 - 0x1b4]` on the claim that the CState spine is
    // 0x1b4. That is wrong, and it silently made sizeof(CHelpState) 0x1ac - so every
    // `new CHelpState` in GruntzMgr.cpp pushed 0x1ac where retail pushes 0x1b8. CState
    // actually ends at 0x1a8, corroborated independently: CAttract (a CState leaf) lands
    // EXACTLY on its retail 0x1c0 on top of this same 0x1a8 base, which it could not do if
    // the base were 0x1b4.
    //
    // ROLE UNRECOVERED for these 0x10 bytes: neither reconstructed CHelpState method
    // (LoadAssets 0x95090, ~CHelpState 0x8cf30) touches +0x1a8..+0x1b7. The SIZE is proven
    // from the allocation; the field roles are not. Do not invent them.
    char m_pad1a8[0x1b8 - 0x1a8];
};
SIZE(0x1b8);

extern u8 g_titleBuf;
#endif // GRUNTZ_HELPSTATE_H
