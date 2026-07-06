// ModeObjInit.cpp - a game-mode/object initializer re-homed out of
// src/Stub/ApiCallers.cpp (0x000c7ec0). __thiscall(a1, a2, a3): stamps a couple of
// owner flags, sets its own flag block, then allocates + wires four owned
// sub-objects (a 0x1c control block, a 0x630 worker with several sub-object arrays,
// a 0x78 four-CString record, and a 0x50 record); on any construction/attach
// failure it tears the partial state back down and returns 0. Finishes by resetting
// its own geometry/timer state, running two vtable init slots + a bind slot, and
// flagging the linked peer. /GX for the owned-object unwind. Placeholder names;
// only offsets + code bytes are load-bearing.
#include <Win32.h> // timeGetTime / ShowCursor via the real headers

#include <Ints.h>
#include <rva.h>
#include <string.h>

namespace modeinit {

    // Compiler array-ctor helpers (reloc-masked) + their element ctor/dtor thunks.
    extern "C" void ElemCtor403774();                                                 // 0x00403774
    extern "C" void ElemDtor5b48c6();                                                 // 0x005b48c6
    extern "C" void ElemCtor403a3a();                                                 // 0x00403a3a
    extern "C" void EhVecCtor(void* base, i32 sz, i32 count, void* ctor, void* dtor); // 0x0011f5a0
    extern "C" void VecCtor(void* base, i32 sz, i32 count, void* ctor);               // 0x00001aa5
    extern "C" void* RezAlloc(u32 sz);                                                // 0x001b9b46
    extern "C" void RezFree(void* p);                                                 // 0x001b9b82

    // The 0x1c control block owned at this->m_2e0.
    struct Ctl1c {
        i32 m_0, m_4, m_8, m_c, m_10, m_14, m_18; // +0x00..+0x18
        i32 Init3e77(i32 a, i32 b);               // 0x00003e77
        void Dtor285b();                          // 0x0000285b
        void SetModeFlag(i32 a);                  // 0x0000171c
    };

    // A CString-like record element (out-of-line ctor/dtor -> reloc-masked).
    struct StrRec {
        char m_pad0[4];
        void Ctor1b4867(i32 a); // 0x001b4867
    };

    // The 0x630 worker owned at this->m_2dc.
    struct Worker630 {
        i32 Init10b4(i32 a); // 0x000010b4
        void PreDtor248c();  // 0x0000248c
        void ModePostInit(); // 0x00001d98
        // one owned sub-object at +0x530 (ctor 0x1b4f0b, dtor 0x1b4f3e)
        struct Sub530 {
            void Ctor1b4f0b(); // 0x001b4f0b
            void Dtor1b4f3e(); // 0x001b4f3e
        };
    };

    // The 0x78 four-CString record owned at this->m_2e4.
    struct Rec78 {
        i32 Init403e();  // 0x0000403e
        void Dtor1cad(); // 0x00001cad
    };

    // The 0x50 record owned at this->m_3f4.
    struct Rec50 {
        void Init286f(); // 0x0000286f
    };

    // The owner's parent object (this->m_4).
    struct Parent {
        char m_pad0[0x5c];
        i32 m_5c; // +0x5c
        char m_pad60[0xbc - 0x60];
        i32 m_bc; // +0xbc
        char m_padc0[0x114 - 0xc0];
        i32 m_114; // +0x114
    };

    struct Arg1 {
        char m_pad0[0x164];
        i32 m_164; // +0x164 (a1->m_150.m_14)
        char m_pad168[0x170 - 0x168];
        i32 m_170; // +0x170 (a1->m_150.m_20)
    };

    // The peer linked at this->m_4e4.
    struct Peer {
        char m_pad0[0x40];
        i32 m_40; // +0x40
    };

    // The owner (this). This is a FOREIGN engine class: its ??_7 and slots 0..36 are
    // unreconstructed engine code, so the honest model is the THREE dispatched slots.
    // Its own vtable slots +0x74/+0x78 (init hooks) and +0x90 (bind hook) are
    // __thiscall (`this` in ecx, args pushed), modeled as 4-byte member-function
    // pointers loaded from the vtable (the `char m_pad` runs document the un-recovered
    // slots) so `this->CallVNN(...)` emits `mov eax,[this]; mov ecx,this;
    // call [eax+0xNN]`. Class COMPLETE before the T::* typedef so each PMF stays 4
    // bytes (docs/patterns/pmf-complete-class-4byte.md).
    struct ModeObjVtbl;
    struct ModeObj {
        ModeObjVtbl* m_vtbl; // +0x00

        Parent* m_4; // +0x04
        char m_pad8[0xc - 8];
        i32 m_c; // +0x0c
        char m_pad10[0x40 - 0x10];
        u8 m_40; // +0x40
        char m_pad41[0x1c0 - 0x41];
        i32 m_1c0; // +0x1c0
        i32 m_1c4; // +0x1c4
        char m_pad1c8[0x1cc - 0x1c8];
        i32 m_1cc;       // +0x1cc
        i32 m_1d0[0x40]; // +0x1d0
        char m_pad2d0[0x2d8 - (0x1d0 + 0x40 * 4)];
        i32 m_2d8;        // +0x2d8
        Worker630* m_2dc; // +0x2dc
        Ctl1c* m_2e0;     // +0x2e0
        Rec78* m_2e4;     // +0x2e4
        char m_pad2e8[0x320 - 0x2e8];
        i32 m_320; // +0x320
        char m_pad324[0x3f4 - 0x324];
        Rec50* m_3f4; // +0x3f4
        char m_pad3f8[0x470 - 0x3f8];
        i32 m_470, m_474, m_478, m_47c, m_480, m_484; // +0x470..
        char m_pad488[0x49c - 0x488];
        i32 m_49c; // +0x49c
        char m_pad4a0[0x4b0 - 0x4a0];
        i32 m_4b0, m_4b4, m_4b8; // +0x4b0..
        char m_pad4bc[0x4e4 - 0x4bc];
        Peer* m_4e4; // +0x4e4

        i32 Init0c7ec0(Arg1* a1, i32 a2, i32 a3);
        i32 Setup43a9(Arg1* a1, i32 a2, i32 a3); // 0x000043a9
        i32 IsModeReady(i32 a, i32 b);           // 0x000035da
        i32 CallV74();                           // +0x74 slot 29
        i32 CallV78(i32 a, i32 b);               // +0x78 slot 30
        void CallV90();                          // +0x90 slot 36
    };
    typedef i32 (ModeObj::*ModeFn74)();
    typedef i32 (ModeObj::*ModeFn78)(i32 a, i32 b);
    typedef void (ModeObj::*ModeFn90)();
    struct ModeObjVtbl {
        char m_pad0[0x74];
        ModeFn74 m_74; // +0x74
        ModeFn78 m_78; // +0x78
        char m_pad7c[0x90 - 0x7c];
        ModeFn90 m_90; // +0x90
    };
    inline i32 ModeObj::CallV74() {
        return (this->*(m_vtbl->m_74))();
    }
    inline i32 ModeObj::CallV78(i32 a, i32 b) {
        return (this->*(m_vtbl->m_78))(a, b);
    }
    inline void ModeObj::CallV90() {
        (this->*(m_vtbl->m_90))();
    }

    // @early-stop
    // EH-frame-absence wall (~55%, RE-PROVEN 2026-07-05). Root cause pinned via --diff:
    // retail emits the full /GX C++-EH frame (mov eax,fs:0; push handler; push scope;
    // mov fs:0,esp) because the 0x630 worker's field arrays are REAL C++ array members
    // whose element ctors (__ehvec_ctor 0x11f5a0) register per-element unwind cleanup;
    // this reconstruction builds them via manual RezAlloc + extern-"C" EhVecCtor/VecCtor
    // CALLS, which carry no EH semantics, so MSVC5 emits NO fs:0 frame at all (the unit
    // is already `eh`/​/GX - the flag can't help without a destructible C++ construct).
    // That absent frame shifts every arg/[esp+N] and flips the this/zero regalloc
    // (retail this=ebx/zero=ebp vs base this=ebp/zero=ebx).
    //
    // FULL RETAIL RECIPE MAPPED (matcher-5 2026-07-05, from the complete 0x5f5-byte
    // disasm; the rewrite was scoped out at session end - execute it verbatim):
    //  * IDENTITY: 0xc7ec0 is CPlay's vtable slot 1 override `Init0c7ec0` (origin
    //    CState; `gruntz sema class CPlay`) -> ModeObj IS CPlay, a1 IS the game
    //    manager (a1+0x164/+0x170 == CNetGameMgr::m_channels[0].m_14/m_20, and the
    //    0x1d98 callee == CNetGameMgr::ResetClockGlobals). Fold to <Gruntz/Play.h>
    //    as the follow-up.
    //  * FIELD MAP CONFIRMED vs canonical CPlay (Play.h, 2026-07-05 - matcher-6):
    //    ModeObj m_2dc==CPlay m_guts (guts/UI subsystem = Worker630 0x630),
    //    m_2e0==m_hitTest (Ctl1c 0x1c control/hit-test sink), m_320==m_overlayActive,
    //    m_3f4==m_frameMarker (CPlaySerialChild = Rec50 0x50), m_470==m_region0Gate,
    //    m_4e4==m_scrollSink (Peer). The FOLD BLOCKER is a leaf-first cascade: CPlay
    //    holds m_guts/m_hitTest/m_frameMarker as ANONYMOUS struct-ptr members, so the
    //    typed sub-object construction here (Worker630::Init10b4/Ctl1c::Init3e77/
    //    Rec78 4xStrRec/Rec50::Init286f + the EhVecCtor arrays) needs those 5 sub-
    //    object classes MODELED as real canonicals first (else the fold re-introduces
    //    casts). Do it WITH the scoped-out /GX-frame rewrite above (both leaf-first).
    //  * All four allocations are genuine `new T` with class operator new/delete ==
    //    RezAlloc/RezFree: Ctl1c (inline nothrow ctor: m_18,m_14,m_c,m_10,m_0,m_4,
    //    m_8=1 -> NO EH state), Worker630 (inline ctor -> states 0/1; inline dtor
    //    { PreDtor248c(); } used by a real `delete` in the fail path -> states 3/2),
    //    Rec78 (inline ctor : 4x StrRec(0xa) members each WITH out-of-line dtor ->
    //    states 4..7; body m_74=0; fail path = EXPLICIT ~Rec78() + RezFree, one null
    //    check), Rec50 (out-of-line ctor 0x286f -> state 8; `if (!(m_3f4=new Rec50))
    //    return 0`).
    //  * Worker630 member kinds by construction order: Elem1c m_2c[8] (out-of-line
    //    ctor 0x403774 + dtor 0x5b48c6 -> __ehvec_ctor); Elem18z m_228[5] (INLINE
    //    ctor zeroing m_0,m_8,m_4,m_c of 0x18 -> the count-5 store loop; body also
    //    pokes m_228[4].m_14/m_10 = 0); Elem10z (0x10, inline ctor zeroing
    //    m_0,m_8,m_4,m_c) singles at 0x2a0,0x2b0,0x320,0x338,0x4d0,0x4f0,0x560;
    //    Elem18c m_2c0[3]/m_378[12] (out-of-line ctor 0x403a3a, no dtor -> 4-arg
    //    vector-ctor iterator); Sub530 @0x530 is 0x14 BYTES (0x544..0x554 are
    //    Worker fields the ctor body writes: m_544=1). Ctor body store order == the
    //    current w[] sequence with 0x1c8..0x200 as FIFTEEN INDIVIDUAL stores (not a
    //    loop; m_618=0 scheduled between m_1f0/m_1f4) and memsets only at
    //    0x114/0x150/0x18c (0xf dwords, rep stosd), 0x204 (5, unrolled), 0x498
    //    (0xc, rep stosd); 0x308 x3 and 0x61c x4 are individual stores.
    //  * Two BUGS in this body vs retail: the 0x1d98 call receiver is
    //    ecx=[esp+0x20] == the A1 ARG SLOT (a1->ResetClockGlobals()), NOT
    //    m_2dc->ModePostInit(); and m_40 is a DWORD store (i32 field), not u8.
    //  * The a1 gate stores are (a1+0x150)-relative disp8: model an Arg1Sub at
    //    +0x150 (m_14/m_20) and write `sub->m_20=1; sub->m_14=1` off &a1->m_150.
    //  * ShowCursor/timeGetTime go through the CACHED import pointers
    //    ?g_ShowCursor@@3P6GHH@ZA (0x6c44c4) / _g_pTimeGetTime (0x6c4650), not the
    //    import thunks.
    //  * Fail paths: `if (m_X == 0) return 0;` FIRST (je to the shared xor-eax
    //    tail), then dtor+free; only Worker630's uses `delete` (keeps its own
    //    second null check, je to the m_2dc=0 store).
    // The logic (owner-flag stamps, 4 owned sub-objects with per-step
    // teardown-and-return-0, ShowCursor drain, geometry/timer reset, the two
    // vtable inits + bind + peer flag) is complete + correct by shape.
    RVA(0x000c7ec0, 0x5f5)
    i32 ModeObj::Init0c7ec0(Arg1* a1, i32 a2, i32 a3) {
        if (a1 == 0) {
            return 0;
        }
        Arg1* sub = a1; // &a1->m_150 sub-object (never null; the null-check is emitted)
        if (sub == 0) {
            return 0;
        }
        sub->m_170 = 1;
        sub->m_164 = 1;
        m_470 = 0;
        m_474 = 0;
        m_478 = 0;
        m_47c = 0;
        m_480 = 0;
        m_484 = 1;
        m_49c = -1;
        m_4b0 = 0;
        m_4b4 = 0;
        m_4b8 = 0;
        m_3f4 = 0;
        if (!Setup43a9(a1, a2, a3)) {
            return 0;
        }

        Ctl1c* ctl = (Ctl1c*)RezAlloc(0x1c);
        if (ctl) {
            ctl->m_18 = 0;
            ctl->m_14 = 0;
            ctl->m_c = 0;
            ctl->m_10 = 0;
            ctl->m_0 = 0;
            ctl->m_4 = 0;
            ctl->m_8 = 1;
        } else {
            ctl = 0;
        }
        m_2e0 = ctl;
        if (m_2e0->Init3e77(m_c, m_4->m_5c) == 0) {
            if (m_2e0) {
                m_2e0->Dtor285b();
                RezFree(m_2e0);
            }
            m_2e0 = 0;
            return 0;
        }
        m_2e0->m_10 = 0;
        m_2e0->SetModeFlag(1);

        Worker630* wk = (Worker630*)RezAlloc(0x630);
        if (wk) {
            char* p = (char*)wk;
            i32 i;
            EhVecCtor(p + 0x2c, 0x1c, 8, (void*)ElemCtor403774, (void*)ElemDtor5b48c6);
            for (i = 0; i < 5; i++) {
                i32* e = (i32*)(p + 0x228 + i * 0x18);
                e[0] = 0;
                e[2] = 0;
                e[1] = 0;
                e[3] = 0;
            }
            VecCtor(p + 0x2c0, 0x18, 3, (void*)ElemCtor403a3a);
            VecCtor(p + 0x378, 0x18, 12, (void*)ElemCtor403a3a);
            ((Worker630::Sub530*)(p + 0x530))->Ctor1b4f0b();
            i32* w = (i32*)p;
            w[0x228 / 4] = 0;
            w[0x22c / 4] = 0;
            w[0x230 / 4] = 0;
            w[0x234 / 4] = 0;
            w[0x2a0 / 4] = 0;
            w[0x2a4 / 4] = 0;
            w[0x2a8 / 4] = 0;
            w[0x2ac / 4] = 0;
            w[0x2b0 / 4] = 0;
            w[0x2b4 / 4] = 0;
            w[0x2b8 / 4] = 0;
            w[0x2bc / 4] = 0;
            w[0x320 / 4] = 0;
            w[0x324 / 4] = 0;
            w[0x328 / 4] = 0;
            w[0x32c / 4] = 0;
            w[0x338 / 4] = 0;
            w[0x33c / 4] = 0;
            w[0x340 / 4] = 0;
            w[0x344 / 4] = 0;
            w[0x4d0 / 4] = 0;
            w[0x4d4 / 4] = 0;
            w[0x4d8 / 4] = 0;
            w[0x4dc / 4] = 0;
            w[0x4f0 / 4] = 0;
            w[0x4f4 / 4] = 0;
            w[0x4f8 / 4] = 0;
            w[0x4fc / 4] = 0;
            w[0x560 / 4] = 0;
            w[0x564 / 4] = 0;
            w[0x568 / 4] = 0;
            w[0x56c / 4] = 0;
            for (i = 0x1c8; i < 0x204; i += 4) {
                w[i / 4] = 0;
            }
            w[0x8 / 4] = 0;
            w[0xc / 4] = 0;
            w[0x20 / 4] = 0;
            w[0x10c / 4] = 0;
            w[0x354 / 4] = 0;
            w[0x358 / 4] = 0;
            w[0x550 / 4] = 0;
            w[0x554 / 4] = 0;
            w[0x614 / 4] = 0x1e0;
            w[0x62c / 4] = 0;
            memset(p + 0x114, 0, 0xf * 4);
            memset(p + 0x150, 0, 0xf * 4);
            memset(p + 0x18c, 0, 0xf * 4);
            memset(p + 0x204, 0, 5 * 4);
            memset(p + 0x498, 0, 0xc * 4);
            w[0x308 / 4] = 0;
            w[0x30c / 4] = 0;
            w[0x310 / 4] = 0;
            w[0x61c / 4] = 0;
            w[0x620 / 4] = 0;
            w[0x624 / 4] = 0;
            w[0x628 / 4] = 0;
            w[0x364 / 4] = 0;
            w[0x36c / 4] = 0;
            w[0x370 / 4] = 0;
            w[0x368 / 4] = 0;
            w[0x4e0 / 4] = 0;
            w[0x500 / 4] = 0;
            w[0x348 / 4] = 0;
            w[0x570 / 4] = 0;
            w[0x218 / 4] = 0;
            w[0x21c / 4] = 0;
            w[0x29c / 4] = 0;
            w[0x298 / 4] = 0;
            w[0x544 / 4] = 1;
            w[0x548 / 4] = 0;
            w[0x54c / 4] = 0;
            w[0x574 / 4] = 0;
        } else {
            wk = 0;
        }
        m_2dc = wk;
        if (m_2dc->Init10b4(m_c) == 0) {
            if (m_2dc) {
                m_2dc->PreDtor248c();
                ((Worker630::Sub530*)((char*)m_2dc + 0x530))->Dtor1b4f3e();
                EhVecCtor((char*)m_2dc + 0x2c, 0, 0, 0, 0); // __ehvec_dtor 0x11f640 (reloc-masked)
                RezFree(m_2dc);
            }
            m_2dc = 0;
            return 0;
        }

        Rec78* r78 = (Rec78*)RezAlloc(0x78);
        if (r78) {
            char* p = (char*)r78;
            ((StrRec*)(p + 0x00))->Ctor1b4867(0xa);
            ((StrRec*)(p + 0x1c))->Ctor1b4867(0xa);
            ((StrRec*)(p + 0x38))->Ctor1b4867(0xa);
            ((StrRec*)(p + 0x54))->Ctor1b4867(0xa);
            *(i32*)(p + 0x74) = 0;
        } else {
            r78 = 0;
        }
        m_2e4 = r78;
        if (m_2e4->Init403e() == 0) {
            if (m_2e4) {
                m_2e4->Dtor1cad();
                RezFree(m_2e4);
            }
            m_2e4 = 0;
            return 0;
        }

        Rec50* r50 = (Rec50*)RezAlloc(0x50);
        if (r50) {
            r50->Init286f();
        } else {
            r50 = 0;
        }
        m_3f4 = r50;
        if (r50 == 0) {
            return 0;
        }

        if (ShowCursor(0) >= 0) {
            while (ShowCursor(0) >= 0) {
            }
        }
        m_1c4 = 1;
        m_40 = 0;
        m_1c0 = 0;
        memset(m_1d0, 0, 0x40 * 4);
        m_2dc->ModePostInit();
        m_1cc = 0;
        m_2d8 = timeGetTime();
        m_320 = 0;
        if (m_4->m_114 == 0) {
            m_4->m_bc = 0;
        }
        if (!CallV74()) {
            return 0;
        }
        CallV90();
        if (!CallV78(a2, 1)) {
            return 0;
        }
        if (!IsModeReady(0, 0)) {
            return 0;
        }
        Peer* peer = m_4e4;
        if (peer) {
            peer->m_40 |= 1;
        }
        return 1;
    }

} // namespace modeinit
