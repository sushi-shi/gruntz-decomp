// CState.h - the WAP32 base game-state class (C:\Proj\Gruntz). One canonical
// definition, shared by GameMode.h (the leaf states CMenuState/CCreditsState/
// CBootyState + the gamemode CPlay::Update match) and CPlay.h (the in-game PLAY
// state whose Render drives the high vtable slots).
//
// Layout is the ctor ground truth (CState::CState at 0x08c750 zeroes a flat
// scalar list and seeds four fields to 0x40). The two reconstructions modeled
// the same owner/view sub-objects under different names: the +0x04 owner back-
// ptr is `void *` here (gamemode casts it to CGMOwner, cplay to CWorld); the
// +0x0c view holder is `CView *` (cplay dereferences it directly, gamemode casts
// to CGMView). Both are 4-byte pointer slots, so the casts are codegen-neutral.
//
// The virtual interface is ~41 slots. Most are out-of-line stubs that only
// anchor the vftable order so the meaningful slots land at the right offset:
// Update (slot 4 / +0x10), Render (slot 5 / +0x14), InputVirtual (slot 8 / +0x20,
// CCreditsState's per-frame poll), BeginFrameClear (slot 31 / +0x7c) and
// RenderSlow/RenderFast (slots 39/40, the CPlay frame-rate split). The vftables
// are not diffed, so the high slots' presence is free; the concrete states
// override the few they implement.
#ifndef GRUNTZ_GRUNTZ_CSTATE_H
#define GRUNTZ_GRUNTZ_CSTATE_H
#include <Gruntz/GameModeBase.h>

struct CView; // +0x0c view holder; defined fully in CPlay.h, opaque elsewhere

class CState {
public:
    CState();
    // dtor body INLINE so MSVC folds the vtable-restore + base cleanup into the
    // synthesized scalar-deleting dtor ??_G (matched) rather than emitting a ??1.
    virtual ~CState() {
        ((CGameModeBase*)this)->BaseCleanup();
    } // slot 0
    virtual void Vfunc1();           // slot 1
    virtual void ReleaseResources(); // slot 2  (+0x8)  resource teardown (leaf override)
    virtual void Vfunc3();           // slot 3
    virtual int Update();            // slot 4  (+0x10)  base default = return 1;
    virtual int Render();            // slot 5  (+0x14)  base default = return 1;
    virtual void Vslot06();          // slot 6
    virtual void Vslot07();          // slot 7
    virtual int InputVirtual();      // slot 8  (+0x20)  per-frame input poll
    virtual void Vslot09();
    virtual int FrameSlot28(int); // slot 10 (+0x28)  per-frame poll (leaf override)
    virtual void Vslot0b();
    virtual void Vslot0c();
    virtual void Vslot0d();
    virtual void Vslot0e();
    virtual void Vslot0f();
    virtual void Vslot10();
    virtual void Vslot11();
    virtual void Vslot12();
    virtual void Vslot13();
    virtual void Vslot14();
    virtual void Vslot15();
    virtual void Vslot16();
    virtual void Vslot17();
    virtual void Vslot18();
    virtual void Vslot19();
    virtual void Vslot1a();
    virtual void Vslot1b();
    virtual void Vslot1c();
    virtual void Vslot1d();
    virtual void Vslot1e();
    virtual void BeginFrameClear(int, int, int); // slot 31 (+0x7c)
    virtual void Vslot20();
    virtual void Vslot21();
    virtual void Vslot22();
    virtual void Vslot23();
    virtual void Vslot24();
    virtual void Vslot25();
    virtual void Vslot26();
    virtual void RenderSlow(); // slot 39 (+0x9c)
    virtual void RenderFast(); // slot 40 (+0xa0)

    // Non-virtual leaf (matched): seeds the begin-clear params.
    int SetBeginClearParams(int unused, int arg2, int arg3);

    // --- scalar members, at the offsets CState::CState pins ---
    void* m_4;  // +0x04  owner back-ptr (CGMOwner / CWorld view)
    int m_8;    // +0x08
    CView* m_c; // +0x0c  view/anim holder (CGMView / CView view)
    char m_pad10[0x14 - 0x10];
    int m_14; // +0x14
    int m_18; // +0x18
    char m_pad1c[0x24 - 0x1c];
    int m_24; // +0x24
    int m_28; // +0x28
    int m_2c; // +0x2c
    char m_pad30[0x38 - 0x30];
    int m_38; // +0x38
    int m_3c; // +0x3c
    char m_pad40[0x4c - 0x40];
    char m_4c; // +0x4c (byte)
    char m_pad4d[0x150 - 0x4d];
    int m_150; // +0x150 BeginFrameClear arg
    int m_154; // +0x154 BeginFrameClear arg
    char m_pad158[0x160 - 0x158];
    // +0x160..+0x1a4: the per-axis scroll/input state block StepInputA walks
    // (two mirrored halves; four extents seeded to 0x40 by the ctor).
    int m_160; // +0x160 first-half axis value
    int m_164; // +0x164 second-half axis value
    int m_168; // +0x168 first-half block (addr taken)
    int m_16c;
    int m_170; // +0x170 (= 0x40)
    int m_174; // +0x174 (= 0x40)
    int m_178; // +0x178 second-half block (addr taken)
    int m_17c;
    int m_180; // +0x180 (= 0x40)
    int m_184; // +0x184 (= 0x40)
    int m_188; // +0x188 first-half {x,y} edge feed
    int m_18c; // +0x18c
    int m_190;
    int m_194;
    int m_198; // +0x198 second-half {x,y} edge feed
    int m_19c;
    int m_1a0;
    int m_1a4;

    void BuildWarpStoneGlitterAnimation();
    void LoadGruntEffectSprites();
    void BuildBootyWalkingGruntz();
};

#endif // GRUNTZ_GRUNTZ_CSTATE_H
