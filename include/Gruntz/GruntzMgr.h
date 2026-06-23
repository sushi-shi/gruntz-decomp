// GruntzMgr.h - CGruntzMgr, the Gruntz game manager (C:\Proj\Gruntz). It is the
// REAL derived game manager: `CGruntzMgr : public WAP32::CGameMgr`, 0xa30 bytes,
// with its own vftable (??_7CGruntzMgr@@6B@ @0x5e9b64). CGruntzApp::Initialize-
// GameManager (@0x080a20) does `new CGruntzMgr` => operator new(0xa30) + the
// CGruntzMgr ctor.  The base WAP32::CGameMgr is the genuine 0x2c engine class;
// all the 0xa30 of per-game state lives HERE.
//
// Only the offsets the matched methods touch are load-bearing:
//   +0xc8 CString, +0xd0 CD drive-letter cache (char) / +0xd4 probed flag,
//   +0xd8 CByteArray, +0xec/+0xf0 CString, +0x150 a 0x238-byte options object
//   (ctor/dtor are out-of-line NAFXCW-style FUN_0051f5a0/FUN_0051f640 calls,
//   reloc-masked). The member subobjects' destructible nature is what gives the
//   ctor/dtor their /GX C++ EH frame.
#ifndef GRUNTZ_GRUNTZ_GRUNTZMGR_H
#define GRUNTZ_GRUNTZ_GRUNTZMGR_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
#include <Wap32/Wap32.h>
#include <Gruntz/CString.h>
#include <Gruntz/GameLevel.h> // CByteArray
#include <Gruntz/CState.h>    // CState (m_2c game-state; Update() at slot 4)

// The 0x238-byte options/registry-backed sub-object embedded at +0x150. Its
// ctor (FUN_0051f5a0) takes (this, 0x238, 4, &thunk_FUN_00431250, &LoadOptions)
// and its dtor (FUN_0051f640) takes (this, 0x238, 4, &LoadOptions); both are
// out-of-line so the calls reloc-mask. Only the size (0x238) + that it is a
// destructible member (EH state 4) are load-bearing.
struct CGruntzMgrOptions {
    CGruntzMgrOptions();
    ~CGruntzMgrOptions();
    char m_pad[0x238];
};

// Minimal sound/DirectSound-ish object held at CGruntzMgr +0x48. Its two
// reloc-masked __thiscall helpers (this in ecx) only need the right call shape;
// the inner object at +0x1c carries the per-frame busy poll.
struct CGruntzSoundInnerZ {
    int IsBusy(); // FUN_00538f60 (this) -> ret (busy state-id 4/0x10)
};
struct CGruntzSoundZ {
    void StopBank(int flag);  // FUN_00538900 (this, 1)  -> ret 4
    void StopAll();           // FUN_005388f0 (this)
    char m_pad0[0x1c];        // +0x00..+0x1c
    CGruntzSoundInnerZ* m_1c; // +0x1c  inner object IsBusy/StopAll deref
};

// Minimal IDirectPlayLobby-shaped COM surface used by
// InitializeLobbyConnectionSettings: only the two vtable slots it calls are
// needed - Release (slot 2, +0x8) and GetConnectionSettings (slot 8, +0x20,
// called twice in the size-probe / fill-buffer idiom). The call rel32/IAT
// displacements reloc-mask; only the slot offsets + arg shapes are load-bearing.
struct IDirectPlayLobbyZ {
    struct Vtbl {
        void *slot0, *slot1;
        long(__stdcall* Release)(IDirectPlayLobbyZ*); // +0x08
        void *slot3, *slot4, *slot5, *slot6, *slot7;
        long(__stdcall* GetConnectionSettings)(
            IDirectPlayLobbyZ*,
            unsigned long appId,
            void* lpData,
            unsigned long* lpdwSize
        ); // +0x20
    }* vtbl;
};

class CGruntzMgr : public WAP32::CGameMgr {
public:
    CGruntzMgr();
    virtual ~CGruntzMgr() OVERRIDE; // vtbl slot 0 (own vftable 0x5e9b64)

    // Manager-owned methods reconstructed in GruntzMgr.cpp.
    void UnknownClose() OVERRIDE;                   // @0x0855e0 (member teardown; stubbed)
    void ReportError(WPARAM wParam, LPARAM lParam); // @0x08dc60  -> m_8->vtbl[0x1c]
    char GetGruntzDriveLetter();                    // @0x08fa70  (memoised CD letter)
    int InitializeLobbyConnectionSettings();        // @0x08eca0 (DirectPlay lobby connect)
    CString BuildMoviePath(int movie);              // @0x08ff30 (per-movie path on the CD)
    void PerFrameTick();                            // @0x08f620 (per-frame draw-clock tick)
    void AdvanceFrame(int doDraw, int unused);      // @0x08f6a0 (the per-frame advance gate)
    int CheckPlayState();                           // @0x08ec50 (m_2c->Update()==3||==0x11)
    int RestoreVideoMode(int save);                 // @0x08ddd0 (re-assert 640x480; save on hit)
    int SetVideoMode(int w, int h, int flag);       // @0x08df00 (mode-switch the display; stubbed)

    // --- members (offsets relative to `this`; base CGameMgr occupies 0x00..0x2c) ---
    CState* m_2c;                           // +0x2c  current game-state (Update() -> state id)
    int m_30, m_34, m_38, m_3c, m_40, m_44; // +0x30..+0x44
    CGruntzSoundZ* m_48;                    // +0x48  sound/bank object (StopBank/StopAll)
    int m_4c, m_50;                         // +0x4c, +0x50
    int m_54, m_58, m_5c, m_60, m_64, m_68, m_6c, m_70, m_74, m_78; // +0x54..+0x78
    int m_7c;                                                       // +0x7c
    int m_80, m_84;                                                 // +0x80, +0x84
    int m_88;                                                       // +0x88  (=0x10 in ctor)
    int m_8c, m_90;                                                 // +0x8c, +0x90  live video mode (w, h)
    int m_94, m_98;                                                 // +0x94, +0x98  saved/last-good mode (w, h)
    int m_9c, m_a0, m_a4, m_a8, m_ac, m_b0, m_b4;                   // +0x9c..+0xb4
    int m_b8;                                                       // +0xb8  (=1 in ctor)
    int m_bc;                                                       // +0xbc
    IDirectPlayLobbyZ* m_c0;        // +0xc0  the lobby interface (Released/recreated)
    void* m_c4;                     // +0xc4  the IDirectPlay interface from Connect
    CString m_strC8;                // +0xc8  (EH state 0)
    int m_cc;                       // +0xcc  (=0x1e in ctor)
    char m_d0;                      // +0xd0  cached CD drive letter
    char m_padD1[3];                // +0xd1
    int m_d4;                       // +0xd4  drive-letter probed flag
    CByteArray m_arrD8;             // +0xd8  (0x14 bytes; EH state 1)
    CString m_strEC;                // +0xec  (EH state 2)
    CString m_strF0;                // +0xf0  (EH state 3)
    int m_f4;                       // +0xf4  (=1 in ctor)
    int m_f8, m_fc;                 // +0xf8, +0xfc
    int m_100, m_104, m_108;        // +0x100..+0x108  (m_100/m_104 =1 in ctor)
    int m_10c, m_110;               // +0x10c, +0x110  (=1 in ctor)
    int m_114;                      // +0x114
    int m_118;                      // +0x118
    char m_pad11c[0x128 - 0x11c];   // +0x11c..+0x128 gap
    int m_128, m_12c, m_130, m_134; // +0x128..+0x134
    int m_138;                      // +0x138  (=3 in ctor)
    char m_pad13c[0x150 - 0x13c];   // +0x13c..+0x150 gap
    CGruntzMgrOptions m_options150; // +0x150 (0x238 bytes; EH state 4)
    char m_pad388[0xa30 - 0x388];   // +0x388..0xa30  remaining game state
};

#endif // GRUNTZ_GRUNTZ_GRUNTZMGR_H
