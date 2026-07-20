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
#include <Gruntz/GruntzMgr.h>    // canonical CGruntzMgr (ResetClockGlobals; the a1 arg)
#include <Gruntz/Play.h>         // canonical CPlay: 0xc7ec0 IS CPlay::LoadGameAssetNamespaces (slot 1)
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr (m_guts; LoadBattlezItemConfig/Teardown)
#include <Gruntz/ChatBoxOwner.h> // canonical CChatBoxOwner (m_hitTest; Attach/Deactivate/Configure)
#include <Gruntz/UserLogic.h>    // canonical CGameObject (m_scrollSink; m_stateFlags bit0)
#include <Gruntz/TileTriggerContainer.h> // canonical CTileTriggerContainer (m_beginMarker; dtor 0xc8640)
#include <Gruntz/TileTriggerSwitchLogic.h> // canonical CTileTriggerSwitchLogic (GetFlag74)

// Rec50::Init286f @0x286f IS CTimer::Init (canonical <Gruntz/Timer.h>).
#include <Gruntz/Timer.h>

namespace modeinit {

    // Compiler array-ctor helpers (reloc-masked) + their element ctor/dtor thunks.
    // (The five hand-rolled compiler-helper externs are GONE - ElemCtor403774 /
    // ElemDtor5b48c6 / ElemCtor403a3a / EhVecCtor / VecCtor. They were extern "C"
    // PHANTOMS - nothing anywhere defined `_EhVecCtor` or `_VecCtor` - invented only to
    // hand-drive the member-array construction of the 0x630 worker. Now that
    // CStatusBarMgr has its real inline ctor, the COMPILER emits those helpers itself
    // (??_L / ??_H / ??_M, the eh-vector-ctor / vector-ctor / eh-vector-dtor iterators,
    // all real NAFXCW symbols) with the real element ctor/dtor pointers.)

    // The 0x1c control block owned at this->m_2e0 IS the canonical CChatBoxOwner
    // (Attach @0x3e77->0x204e0 / Deactivate @0x285b->0x20510 / Configure @0x171c->
    // 0x20530); the inline nothrow ctor emulation below writes its real fields.

    // [The former ModeObj 37-slot placeholder view IS the canonical CPlay (this fn is
    // CPlay's slot-1 override): V74/V78/V90 are the RTTI-proven CPlay slots 29/30/36
    // = LoadImageBanks (+0x74, 0xcffe0) / LoadByMode (+0x78, 0xca200) / Vslot24
    // (+0x90, 0xd0030 - a bare `ret` empty body). Setup43a9 @0x43a9 IS
    // CState::LoadGameAssetNamespaces (0xf9ea0); IsModeReady @0x35da IS
    // CPlay::LoadCursorSprites (0xd0120). The Parent view was CGruntzMgr (m_chatLog/
    // m_saveInfoRec/m_114), Arg1 the CGruntzMgr singleton (m_options[0] gates m_020/
    // m_014 - the ex-GruntzPlayer m_active/m_14 view of the same +0x150 record),
    // Peer the CGameObject m_scrollSink (m_stateFlags bit0), Ctl1c the CChatBoxOwner,
    // Rec78 the CTileTriggerContainer, Rec50 the CTimer.
    //
    // `Worker630` (and its Sub530) was the
    // hand-inlined CStatusBarMgr ctor - ~100 raw `*reinterpret_cast<i32*>(p + 0xNN)` stores plus manual
    // EhVecCtor/VecCtor/CPtrArray-ctor calls. CStatusBarMgr now carries its REAL inline
    // ctor + dtor (<Gruntz/StatusBarMgr.h>), so this TU is view-free: `new
    // CStatusBarMgr` / `delete` emit the identical construction, and the compiler - not
    // this file - owns the member-array iterators.]

} // namespace modeinit

// The out-of-line element ctor 0xc86d0 (`mov eax,ecx; xor ecx,ecx; mov [eax+8],ecx;
// mov [eax+0x10],ecx; mov [eax+0xc],ecx; mov [eax+0x14],ecx; ret`) - the address-taken
// COMDAT that CStatusBarMgr's ctor hands to the vector-ctor iterator for BOTH
// m_groupSlots[3] (+0x2c0) and m_hlGrid[12] (+0x378). It is a compiler-generated
// default ctor, so its home is the TU that instantiates it; the retail COMDAT sits in
// this obj's band (right after CPlay::LoadGameAssetNamespaces / ~CPlay's pool).
RVA(0x000c86d0, 0x11)
CSbiHlRow::CSbiHlRow() {
    m_8 = 0;
    m_10 = 0;
    m_c = 0;
    m_14 = 0;
}

// @early-stop
// 0xc7ec0 is CPlay's slot-1 override (RTTI),
// and this body now runs entirely on canonical classes: the ModeObj 37-slot placeholder
// view, the Parent/Arg1/Peer/Rec78/Ctl1c/Rec50 satellites AND the last one - Worker630,
// the hand-inlined CStatusBarMgr ctor - are all gone. The three dispatched slots are the
// real virtuals (LoadImageBanks 29 / LoadByMode 30 / Vslot24 36), the four sub-objects
// are real `new` expressions on real classes, and both previously-noted bugs are fixed
// (a1 - not m_guts - is the ResetClockGlobals receiver; m_40 is the i32 CState field).
//
// WHAT THE /GX REWRITE BOUGHT: the old wall here was "EH-frame ABSENCE" - retail emits
// the full C++-EH frame (mov eax,fs:0 / push handler / mov fs:0,esp) because the 0x630
// worker's member arrays are REAL C++ arrays whose element ctors register per-element
// unwind cleanup, while this file hand-drove them through extern-"C" calls that carry no
// EH semantics, so MSVC5 emitted NO frame at all. With CStatusBarMgr's real inline ctor
// (member CPtrList[8] via __ehvec_ctor, CSbiSlot[5], CSbiHlRow[3]/[12] via the
// vector-ctor iterator, ::CPtrArray) plus the real `new CChatBoxOwner` /
// `new CTileTriggerContainer`, the constructions ARE EH-tracked and the frame is emitted.
//
// WHAT REMAINS (the honest residual): retail's EH STATE NUMBERING still differs. Retail
// numbers the states 0/1 (the m_tabLists __ehvec_ctor), 3/2 (the `delete`-inlined
// ~CStatusBarMgr), 4..7 (the container's four CPtrList members) and 8 (the CTimer), which
// assumes a particular ctor-inlining order; and the CTimer alloc is `operator new` +
// Init() (0x286f binds EXACT as ?Init@CTimer@@QAEPAV1@XZ - a real method, NOT a ctor), so
// it contributes no state at all on our side. Logic, member identities, call targets and
// every fail-path shape are complete and binary-proven against the full 0x5f5 disasm.
RVA(0x000c7ec0, 0x5f5)
i32 CPlay::LoadGameAssetNamespaces(i32 a1_i, i32 a2, i32 a3) {
    using namespace modeinit;
    // a1 IS the CGruntzMgr singleton (the one cast is the mangling-locked i32 arg;
    // the CState slot-1 HHH signature cannot be retyped).
    CGruntzMgr* a1 = reinterpret_cast<CGruntzMgr*>(a1_i);
    {
        if (a1 == 0) {
            return 0;
        }
        GruntzPlayer* sub = a1->m_options; // &a1->m_150 (never null; the null-check is emitted)
        if (sub == 0) {
            return 0;
        }
        sub->m_liveGate = 1; // live gate  (== the ex-GruntzPlayer::m_active view)
        sub->m_014 = 1;      // armed gate (== the ex-GruntzPlayer::m_14 view)
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
        // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
        if (!CState::LoadGameAssetNamespaces(a1_i, a2, a3)) {
            return 0;
        }

        CChatBoxOwner* ctl = static_cast<CChatBoxOwner*>(RezAlloc(0x1c));
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
        if (m_hitTest->Attach(m_c, m_4->m_chatLog) == 0) {
            if (m_hitTest) {
                m_hitTest->Deactivate();
                RezFree(m_hitTest);
            }
            m_hitTest = 0;
            return 0;
        }
        m_hitTest->m_10 = 0;
        m_hitTest->Configure(1);

        // (2) the guts/UI host - the canonical CStatusBarMgr, now with its REAL inline
        // ctor (<Gruntz/StatusBarMgr.h>). This  expression IS the ~100-store block
        // the local  view used to open-code here: the member arrays
        // (m_tabLists[8] / m_slots[5] / m_groupSlots[3] / m_hlGrid[12] / m_ptrPool) are
        // constructed by the compiler, in retail's order, with retail's iterators.
        m_guts = new CStatusBarMgr;
        if (m_guts->LoadBattlezItemConfig(m_c) == 0) {
            CStatusBarMgr* w2 = m_guts;
            if (w2 == 0) {
                return 0;
            }
            //  inlines the real ~CStatusBarMgr: Teardown(), then the
            // compiler-generated member teardown (??1CPtrArray on m_ptrPool, then
            // __ehvec_dtor over m_tabLists), then operator delete - exactly retail's
            // 0xc82b6 sequence under /GX states 3/2.
            delete w2;
            m_guts = 0;
            return 0;
        }

        CTileTriggerContainer* r78 = static_cast<CTileTriggerContainer*>(RezAlloc(0x78));
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
        if (m_beginMarker->GetFlag74() == 0) {
            // RezFree IS ::operator delete (both 0x1b9b82), so this pair IS the delete.
            delete m_beginMarker; // ~CTileTriggerContainer non-virtual (0xc8640) + ??3
            m_beginMarker = 0;
            return 0;
        }

        CTimer* r50 = static_cast<CTimer*>(RezAlloc(0x50));
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
        m_40 = 0; // the retail DWORD store
        m_1c0 = 0;
        memset(&m_1d0, 0, 0x40 * 4); // clears +0x1d0..+0x2d0
        a1->ResetClockGlobals();     // retail ecx = the A1 arg slot (a1 IS the mgr)
        m_savedClock = 0;
        m_rngSeed = timeGetTime();
        m_lightFx = 0;
        if (m_4->m_114 == 0) {
            m_4->m_saveInfoRec = 0;
        }
        if (!LoadImageBanks()) { // slot 29 (+0x74) virtual dispatch
            return 0;
        }
        Vslot24();                // slot 36 (+0x90) virtual dispatch (retail body: bare ret)
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
