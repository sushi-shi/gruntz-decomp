// SplashState.h - CSplashState, the splash-screen game state (CSplashState : CState,
// RTTI .?AVCSplashState@@, vtbl@0x1e9d74). Its methods are split across two TUs by
// retail RVA: the loader/frame methods (LoadSounds/Render/InputVirtual/... @0xf97xx)
// live in SplashState.cpp; the out-of-line /GX destructor (0x08d000, in the 0x08dxxx
// band) is emitted in HelpState.cpp - both include this one shared class definition.
#ifndef GRUNTZ_SPLASHSTATE_H
#define GRUNTZ_SPLASHSTATE_H

#include <Gruntz/State.h> // CState base (m_4/m_8/m_c/m_2c facets) + CGameModeBase + GameStateId

class CSplashState : public CState {
public:
    // The 11 overridden CState slots (vtbl@0x1e9d74; the other 15 inherited). The
    // vtable is emitted by whichever TU defines slot-0 (~CSplashState @0x08d000, in
    // HelpState.cpp); the loader TU never defines a virtual body, so cl emits no
    // ??_7CSplashState there (leaving its member-offset codegen unchanged).
    virtual ~CSplashState() OVERRIDE;            // slot 0  (0x08d000)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14

    i32 LoadSounds(i32 a, i32 b, i32 c);
    // The base asset-namespace loader (0x43a9 ILT thunk -> 0xf9ea0) is inherited from
    // CState now (LoadSounds calls it cast-free on `this`).

    char m_pad1a8[0x1b8 - 0x1a8];
    i32 m_1b8; // +0x1b8 splash-title countdown timer (frame-delta decremented, clamped 0)
};
SIZE_UNKNOWN(CSplashState);

#endif // GRUNTZ_SPLASHSTATE_H
