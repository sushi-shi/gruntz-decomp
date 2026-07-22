#ifndef GRUNTZ_SPLASHSTATE_H
#define GRUNTZ_SPLASHSTATE_H

#include <Gruntz/State.h> // CState base (m_4/m_8/m_c/m_2c facets) + GameStateId
#include <rva.h>

class CSplashState : public CState {
public:
    // Constructed by CGruntzMgr::TransitionState (`new CSplashState`, state id 14,
    // `push 0x1bc`); cl inlines this body at the new-site exactly as retail does.
    CSplashState() {
        m_1b4 = 0;
    }
    // The 11 overridden CState slots (vtbl@0x1e9d74; the other 15 inherited). The
    // vtable is emitted by whichever TU defines slot-0 (~CSplashState @0x08d000, in
    // HelpState.cpp); the loader TU never defines a virtual body, so cl emits no
    // ??_7CSplashState there (leaving its member-offset codegen unchanged).
    virtual ~CSplashState() OVERRIDE; // slot 0  (0x08d000)
    // slot 1  0x0f9780 (SplashState.cpp; retail ??_7CSplashState slot 1 = ILT
    // 0x1c0d -> 0xf9780, ex "LoadSounds") - the splash asset/sound loader.
    virtual i32 LoadGameAssetNamespaces(i32 a, i32 b, i32 c) OVERRIDE;
    virtual void ReleaseResources() OVERRIDE; // slot 2  0x0f9840 (SplashState.cpp; retail
                                              //   slot 2 = ILT 0x2919; ex "CGameModeBase::Reset")
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14

    // (The ex "LoadSounds" decl is GONE - it IS the slot-1 override above; its body
    // chains the base default via the qualified CState::LoadGameAssetNamespaces().)

    char m_pad1a8[0x1b4 - 0x1a8];
    i32 m_1b4; // +0x1b4 zeroed by the ctor (the state's own one-shot gate)
    i32 m_1b8; // +0x1b8 splash-title countdown timer (frame-delta decremented, clamped 0)
    // ENDS AT 0x1bc - the allocation-proven size (TransitionState: `push 0x1bc`).
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#endif // GRUNTZ_SPLASHSTATE_H
