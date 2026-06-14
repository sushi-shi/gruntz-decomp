// GameMode.h - the game-state ("mode") hierarchy that the per-frame tick drives.
//
// CGruntzMgr/RezMgr::PerFrameTick (@0x8b740, matched in the `rezmgr` unit) holds
// the active game-state object at RezMgr+0x2c (m_mode) and each frame calls:
//     int  m_mode->vtbl[+0x10]()   (slot 4) = Update()  -> a state-ID/status int
//     int  m_mode->vtbl[+0x14]()   (slot 5) = Render()  -> the per-frame step+draw
// (PerFrameTick gates timing on `Update() != 0x11` and gates Render on a render
// flag.) This file reconstructs that state hierarchy.
//
// THE HIERARCHY (recovered from RTTI + the vtables, ImageBase 0x400000):
//   CState           base game-state class. RTTI .?AVCState@@,  vftable @0x5ea21c.
//                    (CState itself derives from a WAP32 base @0xfa150 - the dtor
//                     chains to it; modeled here as an external no-body fn.)
//   CPlay            the in-game PLAY state.   RTTI .?AVCPlay@@,        vftable @0x5ea0bc.
//   CMenuState       the front-end menu state. RTTI .?AVCMenuState@@,   vftable @0x5e9e84.
//   CCreditsState    the credits state.        RTTI .?AVCCreditsState@@,vftable @0x5e9c64.
//   CBootyState      the bonus/"booty" state.  RTTI .?AVCBootyState@@,  vftable @0x5e9cec.
//
// THE KEY FINDING: each state's Update() (slot +0x10) is a 6-byte stub that just
// returns the state's own ID tag - it is NOT the per-frame logic:
//   CState::Update      @0x8c4b0  ->  return 1;
//   CPlay::Update       @0x8c910  ->  return 3;
//   CMenuState::Update  @0x8ce10  ->  return 5;
//   CCreditsState::Update @0x8d590 -> return 8;
//   CBootyState::Update @0x8d3f0  ->  return 0xa;
// The REAL per-frame step+draw lives in Render() (slot +0x14): CState::Render is a
// trivial `return 1;` default (@0x8c4d0), but the concrete states override it with
// the heavy per-frame work (input poll -> entity update loop -> network post ->
// draw): CCreditsState::Render @0x391d0 (380 B), CMenuState::Render @0xa0750
// (464 B), CBootyState::Render @0x1c210 (1205 B), CPlay::Render @0xc8cf0 (3092 B,
// the in-game per-frame heart).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the code bytes
// are load-bearing (campaign doctrine). The CState layout below is confirmed from
// the ctor (@0x8c750) and dtor (@0x8c710).
#ifndef SRC_GRUNTZ_GAMEMODE_H
#define SRC_GRUNTZ_GAMEMODE_H

// The WAP32 base cleanup CState's dtor chains to (@0xfa150, reached via the
// incremental-link thunk @0x3f53). It is a __thiscall (this in ecx, callee-
// cleaned, NO `add esp,4` at the call site), so it is modeled as a method on a
// tiny helper whose `this` is the CState object - that emits `mov ecx,this; call
// rel32` (reloc-masked) with no stack cleanup, matching the target.
struct CGameModeBase {
    void BaseCleanup();   // 0xfa150 (thiscall, no-body -> reloc-masked call)
};
// (The scalar-deleting dtor's `operator delete` @0x1b9b82 is reached via MSVC's
// auto-synthesized `??3@YAXPAX@Z` in the `??_G` thunk - no explicit decl needed.)

// ---------------------------------------------------------------------------
// CState - the base game-state class (vftable @0x5ea21c). Polymorphic so the
// vptr lands at +0x00 and the two-phase vtable store falls out.
//
// The ctor (@0x8c750) stores the vftable, then zeroes a flat list of scalar
// members and seeds four of them to 0x40:
//   +0x04 +0x08 +0x0c +0x14 +0x18 +0x24 +0x28 +0x2c +0x38 +0x3c = 0
//   +0x4c (byte) = 0
//   +0x150 +0x154 +0x160 +0x164 +0x168 +0x16c = 0
//   +0x170 = 0x40   +0x174 = 0x40
//   +0x178 = 0      +0x17c = 0
//   +0x180 = 0x40   +0x184 = 0x40
//   +0x188 +0x18c +0x190 +0x194 +0x198 +0x19c +0x1a0 +0x1a4 = 0
// (CState is at least 0x1a8 bytes; the concrete states extend it much further.)
// ---------------------------------------------------------------------------
class CState {
public:
    CState();                       // 0x8c750  (ctor)
    // The dtor body is defined inline so MSVC inlines the vtable-restore + base
    // cleanup directly into the synthesized scalar-deleting dtor `??_G` @0x8c710
    // (the target inlines them rather than emitting a `call ??1`).
    virtual ~CState() { ((CGameModeBase *)this)->BaseCleanup(); }  // slot 0 @0x8c710

    // The virtual interface (24+ slots). Only the slots the per-frame tick drives
    // (+0x10 Update, +0x14 Render) and the dtor (slot 0) carry meaning here; the
    // intervening slots are out-of-line stubs that anchor the vftable order so
    // Update lands at slot 4 (+0x10) and Render at slot 5 (+0x14).
    virtual void Vfunc1();          // slot 1  (+0x04) @0x8c4f0-ish stub
    virtual void Vfunc2();          // slot 2  (+0x08) base dtor thunk
    virtual void Vfunc3();          // slot 3  (+0x0c)
    virtual int  Update();          // slot 4  (+0x10) @0x8c4b0  return 1;
    virtual int  Render();          // slot 5  (+0x14) @0x8c4d0  return 1;

    int  m_4;       // +0x04
    int  m_8;       // +0x08
    int  m_c;       // +0x0c
    char m_pad10[0x14 - 0x10];      // +0x10 (not ctor-written)
    int  m_14;      // +0x14
    int  m_18;      // +0x18
    char m_pad1c[0x24 - 0x1c];      // +0x1c (not ctor-written)
    int  m_24;      // +0x24
    int  m_28;      // +0x28
    int  m_2c;      // +0x2c
    char m_pad30[0x38 - 0x30];      // +0x30 (not ctor-written)
    int  m_38;      // +0x38
    int  m_3c;      // +0x3c
    char m_pad40[0x4c - 0x40];      // +0x40 (not ctor-written)
    char m_4c;      // +0x4c (byte = 0)
    char m_pad4d[0x150 - 0x4d];     // +0x4d (not ctor-written)
    int  m_150;     // +0x150
    int  m_154;     // +0x154
    char m_pad158[0x160 - 0x158];   // +0x158 (not ctor-written)
    int  m_160;     // +0x160
    int  m_164;     // +0x164
    int  m_168;     // +0x168
    int  m_16c;     // +0x16c
    int  m_170;     // +0x170 (= 0x40)
    int  m_174;     // +0x174 (= 0x40)
    int  m_178;     // +0x178
    int  m_17c;     // +0x17c
    int  m_180;     // +0x180 (= 0x40)
    int  m_184;     // +0x184 (= 0x40)
    int  m_188;     // +0x188
    int  m_18c;     // +0x18c
    int  m_190;     // +0x190
    int  m_194;     // +0x194
    int  m_198;     // +0x198
    int  m_19c;     // +0x19c
    int  m_1a0;     // +0x1a0
    int  m_1a4;     // +0x1a4
};

// ---------------------------------------------------------------------------
// The concrete states. Each overrides Update() to return its own state-ID tag
// (the 6-byte stub) - that is the only override modeled here for the leaf match
// (their own vtables @0x5ea0bc / 0x5e9e84 / 0x5e9c64 / 0x5e9cec carry the heavy
// Render overrides, matched/carcassed separately).
// ---------------------------------------------------------------------------
class CPlay : public CState {
public:
    virtual int Update();           // @0x8c910  return 3;  (vtable @0x5ea0bc slot 4)
};

class CMenuState : public CState {
public:
    virtual int Update();           // @0x8ce10  return 5;  (vtable @0x5e9e84 slot 4)
};

class CCreditsState : public CState {
public:
    virtual int Update();           // @0x8d590  return 8;  (vtable @0x5e9c64 slot 4)
};

class CBootyState : public CState {
public:
    virtual int Update();           // @0x8d3f0  return 0xa; (vtable @0x5e9cec slot 4)
};

#endif // SRC_GRUNTZ_GAMEMODE_H
