// ModeObjInit.cpp - a game-mode/object initializer re-homed out of
// src/Stub/ApiCallers.cpp (0x000c7ec0). __thiscall(a1, a2, a3): stamps a couple of
// owner flags, sets its own flag block, then allocates + wires four owned
// sub-objects (a 0x1c control block, a 0x630 worker with several sub-object arrays,
// a 0x78 four-CString record, and a 0x50 record); on any construction/attach
// failure it tears the partial state back down and returns 0. Finishes by resetting
// its own geometry/timer state, running two vtable init slots + a bind slot, and
// flagging the linked peer. /GX for the owned-object unwind. Placeholder names;
// only offsets + code bytes are load-bearing.
#include <Mfc.h> // MFC superset (CPtrList; timeGetTime/ShowCursor); was Win32.h
#include <new>

#include <Ints.h>
#include <rva.h>
#include <string.h>
#include <Gruntz/GruntzMgr.h>              // canonical CGruntzMgr (ResetClockGlobals; the a1 arg)
#include <Gruntz/Play.h>                   // canonical CPlay: 0xc7ec0 IS CPlay::Vfunc1 (slot 1)
#include <Gruntz/StatusBarMgr.h>           // canonical CStatusBarMgr (m_guts; LoadBattlezItemConfig/Teardown)
#include <Gruntz/ChatBoxOwner.h>           // canonical CChatBoxOwner (m_hitTest; Attach/Deactivate/Configure)
#include <Gruntz/UserLogic.h>              // canonical CGameObject (m_scrollSink; m_stateFlags bit0)
#include <Net/NetMgr.h>                    // CNetGameMgr field view of a1 (m_channels[0] gates)
#include <Gruntz/TileTriggerContainer.h>   // canonical CTileTriggerContainer (m_beginMarker; dtor 0xc8640)
#include <Gruntz/TileTriggerSwitchLogic.h> // canonical CTileTriggerSwitchLogic (GetFlag74)

// Rec50::Init286f @0x286f IS CTimer::Init (canonical <Gruntz/Timer.h>).
#include <Gruntz/Timer.h>

namespace modeinit {

    // Compiler array-ctor helpers (reloc-masked) + their element ctor/dtor thunks.
    extern "C" void ElemCtor403774();                                                 // 0x00403774
    extern "C" void ElemDtor5b48c6();                                                 // 0x005b48c6
    extern "C" void ElemCtor403a3a();                                                 // 0x00403a3a
    extern "C" void EhVecCtor(void* base, i32 sz, i32 count, void* ctor, void* dtor); // 0x0011f5a0
    extern "C" void VecCtor(void* base, i32 sz, i32 count, void* ctor);               // 0x00001aa5
    extern "C" void* RezAlloc(u32 sz);                                                // 0x001b9b46
    extern "C" void RezFree(void* p);                                                 // 0x001b9b82

    // The 0x1c control block owned at this->m_2e0 IS the canonical CChatBoxOwner
    // (Attach @0x3e77->0x204e0 / Deactivate @0x285b->0x20510 / Configure @0x171c->
    // 0x20530); the inline nothrow ctor emulation below writes its real fields.

    // A CString-like record element (out-of-line ctor/dtor -> reloc-masked).

    // The 0x630 worker owned at this->m_2dc.
    struct Worker630 {
        // Init10b4 IS CStatusBarMgr::LoadBattlezItemConfig; cast at the call.
        // PreDtor248c IS CStatusBarMgr::Teardown; cast at the call.
        // ModePostInit IS CGruntzMgr::ResetClockGlobals; cast at the call.
        // one owned sub-object at +0x530 (ctor 0x1b4f0b, dtor 0x1b4f3e)
        struct Sub530 {
            void Ctor1b4f0b(); // 0x001b4f0b
            void Dtor1b4f3e(); // 0x001b4f3e
        };
    };

    // [The former ModeObj 37-slot placeholder view IS the canonical CPlay (this fn is
    // CPlay's slot-1 override): V74/V78/V90 are the RTTI-proven CPlay slots 29/30/36
    // = LoadImageBanks (+0x74, 0xcffe0) / LoadByMode (+0x78, 0xca200) / Vslot24
    // (+0x90, 0xd0030 - a bare `ret` empty body). Setup43a9 @0x43a9 IS
    // CState::LoadGameAssetNamespaces (0xf9ea0); IsModeReady @0x35da IS
    // CPlay::LoadCursorSprites (0xd0120). The Parent view was CGruntzMgr (m_chatLog/
    // m_saveInfoRec/m_114), Arg1 the CNetGameMgr field view (m_channels[0] gates),
    // Peer the CGameObject m_scrollSink (m_stateFlags bit0), Ctl1c the CChatBoxOwner,
    // Rec78 the CTileTriggerContainer, Rec50 the CTimer - all dissolved onto the
    // canonicals; only Worker630 (the manual CStatusBarMgr-ctor emulation the /GX
    // rewrite below owns) remains a local shape.]

} // namespace modeinit

// @early-stop
// REPARENTED 2026-07-10: 0xc7ec0 is now the real CPlay::Vfunc1 (slot 1) so
// CDemo::Vfunc1 (0x3bfa0) reloc-pairs its base call (Demo.cpp, now byte-exact).
// FOLDED 2026-07-13 (Fable lane): the ModeObj foreign view is DISSOLVED - the body
// now runs on the canonical CPlay members (m_guts/m_hitTest/m_beginMarker/
// m_frameMarker/m_region*Gate/...), the three slot dispatches are the real virtuals
// (LoadImageBanks/LoadByMode/Vslot24), and the two noted bugs are fixed (a1 is the
// ResetClockGlobals receiver; m_40 is the i32 CState field). Only the Worker630
// construction block (the manual CStatusBarMgr-ctor emulation) remains a local
// shape - it is what the /GX rewrite below replaces. Still the EH wall:
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
//    ((CGruntzMgr*)m_2dc)->ResetClockGlobals(); and m_40 is a DWORD store (i32 field), not u8.
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
// 0xc7ec0 IS CPlay's vtable slot-1 override (origin CState; the mode/object
// initializer CDemo::Vfunc1 delegates to). The whole body is still modeled through
// the `modeinit::ModeObj` foreign view (the 5-sub-object leaf-first cascade to fold
// it onto CPlay's real m_guts/m_hitTest/m_frameMarker members is the documented
// deferred work); `this` is reinterpret-cast to that view once - byte-neutral (both
// are single-inheritance with the vptr at +0), reparenting only the emitted symbol
// from ?Init0c7ec0@ModeObj to ?Vfunc1@CPlay so CDemo::Vfunc1's base call reloc-pairs.
RVA(0x000c7ec0, 0x5f5)
i32 CPlay::Vfunc1(i32 a1_i, i32 a2, i32 a3) {
    using namespace modeinit;
    CNetGameMgr* a1 = (CNetGameMgr*)a1_i; // a1 IS the game-manager singleton (field view)
    {
        if (a1 == 0) {
            return 0;
        }
        CNetChannel* sub = a1->m_channels; // &a1->m_150 (never null; the null-check is emitted)
        if (sub == 0) {
            return 0;
        }
        sub->m_active = 1;
        sub->m_14 = 1;
        m_region0Gate = 0;
        m_region1Gate = 0;
        m_region2Gate = 0;
        m_region3Gate = 0;
        m_viewMode = 0;
        m_hudSuppressed = 1;
        m_49c = -1;
        m_snapshotActive = 0;
        m_scrollEdgeActive = 0;
        m_scrollEdgeLock = 0;
        m_frameMarker = 0;
        if (!LoadGameAssetNamespaces(a1_i, a2, a3)) {
            return 0;
        }

        CChatBoxOwner* ctl = (CChatBoxOwner*)modeinit::RezAlloc(0x1c);
        if (ctl) {
            // the inline nothrow ctor (no EH state)
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
        m_hitTest = ctl;
        if ((m_hitTest->Attach(m_c, (CChatBoxTextHost*)m_4->m_chatLog), 0)) {
            if (m_hitTest) {
                m_hitTest->Deactivate();
                modeinit::RezFree(m_hitTest);
            }
            m_hitTest = 0;
            return 0;
        }
        m_hitTest->m_10 = 0;
        m_hitTest->Configure(1);

        Worker630* wk = (Worker630*)modeinit::RezAlloc(0x630);
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
        m_guts = (CStatusBarMgr*)wk;
        if (m_guts->LoadBattlezItemConfig((i32)m_c) == 0) {
            if (m_guts) {
                m_guts->Teardown();
                ((Worker630::Sub530*)((char*)m_guts + 0x530))->Dtor1b4f3e();
                EhVecCtor(
                    (char*)m_guts + 0x2c,
                    0,
                    0,
                    0,
                    0
                ); // __ehvec_dtor 0x11f640 (reloc-masked)
                modeinit::RezFree(m_guts);
            }
            m_guts = 0;
            return 0;
        }

        CTileTriggerContainer* r78 = (CTileTriggerContainer*)modeinit::RezAlloc(0x78);
        if (r78) {
            // the inline ctor: the four CPtrList(0xa) members + the m_74 gate
            new (&r78->m_base) CPtrList(0xa);
            new (&r78->m_list1) CPtrList(0xa);
            new (&r78->m_list2) CPtrList(0xa);
            new (&r78->m_list3) CPtrList(0xa);
            r78->m_74 = 0;
        } else {
            r78 = 0;
        }
        m_beginMarker = r78;
        if (((CTileTriggerSwitchLogic*)m_beginMarker)->GetFlag74() == 0) {
            if (m_beginMarker) {
                m_beginMarker->~CTileTriggerContainer();
                modeinit::RezFree(m_beginMarker);
            }
            m_beginMarker = 0;
            return 0;
        }

        CTimer* r50 = (CTimer*)modeinit::RezAlloc(0x50);
        if (r50) {
            r50->Init();
        } else {
            r50 = 0;
        }
        m_frameMarker = r50;
        if (r50 == 0) {
            return 0;
        }

        if (ShowCursor(0) >= 0) {
            while (ShowCursor(0) >= 0) {
            }
        }
        m_1c4 = 1;
        m_40 = 0; // the retail DWORD store (the ex-view's u8 was the noted bug)
        m_1c0 = 0;
        memset(&m_1d0, 0, 0x40 * 4); // clears +0x1d0..+0x2d0
        ((CGruntzMgr*)a1)->ResetClockGlobals(); // retail ecx = the A1 arg slot (a1 IS the mgr)
        m_savedClock = 0;
        m_rngSeed = timeGetTime();
        m_lightFx = 0;
        if (m_4->m_114 == 0) {
            m_4->m_saveInfoRec = 0;
        }
        if (!LoadImageBanks()) { // slot 29 (+0x74) virtual dispatch
            return 0;
        }
        Vslot24(); // slot 36 (+0x90) virtual dispatch (retail body: bare ret)
        if (!LoadByMode(a2, 1)) { // slot 30 (+0x78) virtual dispatch
            return 0;
        }
        if (!LoadCursorSprites(0, 0)) {
            return 0;
        }
        CGameObject* peer = m_scrollSink;
        if (peer) {
            peer->m_stateFlags |= 1;
        }
        return 1;
    }
}
