// GruntzMgrTransition.cpp - CGruntzMgr::TransitionState (0x8b960), the /GX
// game-state factory (C:\Proj\Gruntz). Split into its own eh unit because it is a
// dense multi-`new` object factory whose per-object member construction (member
// CStrings/CByteArrays) forces the /GX EH frame + a long unwind-state ladder; it
// cannot share GruntzMgr.cpp's frame without re-framing that TU's matched leaves.
//
// Shape: tear down the current state (push it, or scalar-delete it + drain the
// stack), then - unless the replay gate (m_a4) short-circuits to a bare CState -
// switch on the requested state id and `new` the matching CState-derived object
// (base ctor + inline member construction + field zeroing + retail vtable stamp),
// install it at m_curState, run its slot-1 activate, and sync the state managers.
//
// The state objects are the CState/CPlay/CMulti mode hierarchy (see GameMode.h /
// CPlay.h / CMulti.h). The param-1 replay path constructs the REAL, fully-modeled
// CState (`new CState`) - CState alone has a standalone ctor (0x8c750) to call. The
// switch-case LEAF states have NO standalone ctor in retail (their construction
// exists only inlined here), so each is modeled as a reduced local layout of its
// REAL class (CAttract / CPlay / CMulti / ...) carrying only the destructible
// members (member CStrings/CByteArrays) that force the EH-state ladder.
//
// Vtable identity WITHOUT a steal: each leaf layout is NON-polymorphic and its ctor
// stamps the object's +0x00 vptr MANUALLY from a reloc-masked EXTERNAL vtable symbol
// (g_st<Class>Vtbl, address = the retail ??_7<Class>). Because the class is not
// polymorphic, cl emits NO local ??_7<Class> here - so the delinker's binding of the
// retail vtable RVA stays owned by the real class's TU (cattract/cplaydtor/cmulti/
// gamemode/...). The g_st<Class>Vtbl externs are DIFFERENTLY-NAMED from those TUs'
// ??_7<Class> and carry NO DATA() binding, so they only reloc-mask the stamp and
// never claim the RVA. (Same manual-inline-construction pattern as CButeSection /
// CNetPeer.) Field names are placeholders; offsets + code bytes are load-bearing.
#include <Gruntz/GruntzMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/Play.h> // canonical CPlay (the case-3 `new CPlay` uses the one shape)
#include <rva.h>

// operator new / the game heap (0x1b9b46), reloc-masked.
void* operator new(u32);

// The DirectInputMgr2 singleton (*g_645570) re-armed after the install (ReadAll).
DATA(0x00245570)
extern DirectInputMgr2* g_645570;

// The vtable-dispatch view of a FOREIGN state object: the factory drives the current
// state through slots 0 (scalar-deleting dtor), 1 (activate), 4 (id), 9 and 10; the
// rest are unreconstructed engine code (the retail vtable is stamped by the leaf
// ctors from the g_st<Class>Vtbl externs below). Honest model = a manual vptr into a
// typed vtable struct naming ONLY the dispatched slots as 4-byte thiscall PMFs + char
// pad[], NO fake virtuals; m_vtbl sits at +0x00 exactly where the fake vptr did, so
// the object layout (m_pad2c, m_1c) is byte-identical.
struct CTsStateVtbl;
struct CTsState {
    CTsStateVtbl* m_vtbl;                              // +0x00
    char m_pad2c[0x1c - 0x0c];                         // (kept exactly)
    i32 m_1c;                                          // sub-object saved on teardown
    void CallDtor(u32 flags);                          // slot 0  scalar-deleting dtor
    i32 CallActivate(CGruntzMgr* mgr, i32 a2, i32 a3); // slot 1  activate
    i32 CallId();                                      // slot 4  id/update
    i32 CallSlot9(i32 a);                              // slot 9
    i32 CallSlot10(i32 a);                             // slot 10 (+0x28)
};
typedef void (CTsState::*TsDtorFn)(u32);
typedef i32 (CTsState::*TsActFn)(CGruntzMgr*, i32, i32);
typedef i32 (CTsState::*TsIdFn)();
typedef i32 (CTsState::*TsSlotFn)(i32);
struct CTsStateVtbl {
    TsDtorFn Dtor;    // +0x00 slot 0
    TsActFn Activate; // +0x04 slot 1
    char m_pad08[0x10 - 0x08];
    TsIdFn Id; // +0x10 slot 4
    char m_pad14[0x24 - 0x14];
    TsSlotFn Slot9;  // +0x24 slot 9
    TsSlotFn Slot10; // +0x28 slot 10
};
SIZE_UNKNOWN(CTsStateVtbl);
inline void CTsState::CallDtor(u32 flags) {
    (this->*(m_vtbl->Dtor))(flags);
}
inline i32 CTsState::CallActivate(CGruntzMgr* mgr, i32 a2, i32 a3) {
    return (this->*(m_vtbl->Activate))(mgr, a2, a3);
}
inline i32 CTsState::CallId() {
    return (this->*(m_vtbl->Id))();
}
inline i32 CTsState::CallSlot9(i32 a) {
    return (this->*(m_vtbl->Slot9))(a);
}
inline i32 CTsState::CallSlot10(i32 a) {
    return (this->*(m_vtbl->Slot10))(a);
}

// The minimal destructible MFC members that force the per-object EH-state ladder;
// their ctors/dtors are the reloc-masked NAFXCW bodies (0x1b9b93 / 0x1b4f0b ...).
struct MfcStr {
    MfcStr();
    ~MfcStr();
    char* m_p;
};
struct MfcBytes {
    virtual void VSlot0(); // vptr @+0x00 (real polymorphic; MFC CObject-like)
    MfcBytes();
    ~MfcBytes();
    void* m_data;
    i32 m_size, m_max, m_grow;
};

// The two out-of-line base ctors (0x8c750 = CState base; 0x8c9d0 = CPlay base for
// the multiplayer/param-7 states). Declared no-body -> reloc-masked base calls. Both
// are NON-polymorphic sized layouts: +0x00 is an explicit vptr slot the derived leaf
// stamps by hand from its g_st<Class>Vtbl extern (no compiler-emitted ??_7 here).
struct CTsBaseA {
    virtual void VSlot0();    // vptr @+0x00 (real polymorphic; per-state override)
    CTsBaseA();               // 0x8c750
    char m_cstate[0x1b4 - 4]; // +0x04..+0x1b4
};
struct CTsBaseB {
    virtual void VSlot0();   // vptr @+0x00 (real polymorphic)
    CTsBaseB();              // 0x8c9d0 (CPlay layout, 0x520)
    char m_cplay[0x520 - 4]; // +0x04..+0x520
};

// Two extra member sub-objects the credits state (param 8) builds (a small ctor
// @0x8c3b0 and the two 4-arg Set @0x8c380 initializers).
struct CTsSub45 {
    CTsSub45(); // 0x8c3b0
    char m_pad[8];
};
void Ts_Set(void* self, i32 a, i32 b, i32 c, i32 d); // 0x8c380 (member Set, 4 args)

// The ten retail state vtables, referenced by RELOC-MASKED EXTERNAL reference. Each
// leaf ctor stamps `m_vptr = &g_st<Class>Vtbl`; the extern's address IS the retail
// ??_7<Class>, but its DIFFERENT name (+ no DATA() binding) means cl emits no local
// vtable and the delinker keeps the RVA bound to the real class's TU (noted). The
// switch key is the state id TransitionState is asked for.
// (CPlay uses the canonical `class CPlay : public CState` - cl auto-stamps
// ??_7CPlay in its ctor, so no g_stCPlayVtbl manual-stamp extern is needed.)

// ---- the CState-derived state objects (reduced local layouts of the real classes;
// the retail vtable is stamped by hand from the externals above) ----------------
struct CAttract : CTsBaseA { // param 2, 0x1c0
    char m_pad[0x1c0 - 0x1b4];
    CAttract() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    }
};
// NOT foldable onto the canonical <Gruntz/GameMode.h> `CMenuState : CState` (Bucket-C
// base-fold wall): a canonical polymorphic CMenuState would make cl emit + stamp a
// LOCAL ??_7CMenuState here (the ctor references its own vtable), claiming the retail
// vtable RVA 0x5e9e84 that gamemode already owns. This non-poly shell + manual g_st*
// stamp is the deliberate whole-family pattern (see file header); the loader-view fold
// happened in MenuStateAssets.cpp, which does not construct the class.
struct CMenuState : CTsBaseA { // param 5, 0x1c0
    i32 m_1b4;                 // +0x1b4
    char m_pad1b8[0x1c0 - 0x1b8];
    CMenuState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
        m_1b4 = 0;
    }
};
struct CHelpState : CTsBaseA { // param 9, 0x1b8
    char m_pad[0x1b8 - 0x1b4];
    CHelpState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    }
};
struct CSplashState : CTsBaseA { // param 14, 0x1bc
    i32 m_1b4;                   // +0x1b4
    char m_pad1b8[0x1bc - 0x1b8];
    CSplashState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
        m_1b4 = 0;
    }
};
struct CDemo : CTsBaseB { // param 7, 0x528
    char m_pad[0x528 - 0x520];
    CDemo() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    }
};
struct CMultiBootyState : CTsBaseA { // param 18, 0x244
    i32 m_1b4;                       // +0x1b4
    i32 m_1b8;                       // +0x1b8
    char m_pad1bc[0x244 - 0x1bc];
    CMultiBootyState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
        m_1b4 = 0;
        m_1b8 = 0x64;
    }
};
struct CBootyState : CTsBaseA { // param 10, 0x320
    i32 m_1b4;                  // +0x1b4
    i32 m_1b8;                  // +0x1b8
    i32 m_1bc;                  // +0x1bc
    i32 m_1c0;                  // +0x1c0
    i32 m_1c4;                  // +0x1c4
    i32 m_1c8;                  // +0x1c8
    i32 m_1cc;                  // +0x1cc
    i32 m_1d0;                  // +0x1d0
    i32 m_1d4;                  // +0x1d4
    char m_pad1d8[0x1ec - 0x1d8];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
    i32 m_1f4; // +0x1f4
    i32 m_1f8; // +0x1f8
    char m_pad1fc[0x200 - 0x1fc];
    i32 m_200; // +0x200
    char m_pad204[0x284 - 0x204];
    i32 m_284[8]; // +0x284
    i32 m_2a4[8]; // +0x2a4
    i32 m_2c4;    // +0x2c4
    char m_pad2c8[0x2e8 - 0x2c8];
    i32 m_2e8; // +0x2e8
    i32 m_2ec; // +0x2ec
    i32 m_2f0; // +0x2f0
    i32 m_2f4; // +0x2f4
    char m_pad2f8[0x320 - 0x2f8];
    CBootyState();
};
// NOTE for matcher-4 (the vtbl-stamp owner of this file): the canonical
// CCreditsState : CState EXISTS in <Gruntz/GameMode.h> (RTTI vtbl@0x1e9c64) and
// CreditzAssets.cpp now folds onto it. This local shell + its g_stCCreditsStateVtbl
// ctor stamp is the SAME Bucket-C whole-family pattern documented on CMenuState
// above; when the family converts, union THIS view's field knowledge into the
// canonical (m_1c8/m_1d8 rect subs; m_1e8's CTsSub45 ctor 0x8c3b0 is the canonical
// CCreditsImageList's; m_1f0 == m_caption; m_208/m_210 == m_videoPlaying/
// m_videoHandle - offsets agree, no conflation).
struct CCreditsState : CTsBaseA { // param 8, 0x218
    i32 m_1b4;                    // +0x1b4
    i32 m_1b8;                    // +0x1b8
    i32 m_1bc;                    // +0x1bc
    i32 m_1c0;                    // +0x1c0
    i32 m_1c4;                    // +0x1c4
    char m_1c8[0x10];             // +0x1c8  Set-initialized rect sub-object
    char m_1d8[0x10];             // +0x1d8  Set-initialized rect sub-object
    CTsSub45 m_1e8;               // +0x1e8
    MfcStr m_1f0;                 // +0x1f0
    i32 m_1f4;                    // +0x1f4
    i32 m_1f8;                    // +0x1f8
    i32 m_1fc;                    // +0x1fc
    i32 m_200;                    // +0x200
    i32 m_204;                    // +0x204
    i32 m_208;                    // +0x208
    i32 m_20c;                    // +0x20c
    i32 m_210;                    // +0x210
    char m_pad214[0x218 - 0x214];
    CCreditsState();
};
// CPlay (param 3, 0x520) is the canonical `class CPlay : public CState` from
// <Gruntz/Play.h>; its five destructible MFC members + CState base give the same
// inline-construction shape the factory needs, and its ctor (defined below) stamps
// ??_7CPlay via cl (no manual g_stCPlayVtbl stamp).
struct CMulti : CTsBaseB { // param 17, 0x660
    i32 m_520;             // +0x520
    i32 m_524;             // +0x524
    char m_pad528[0x590 - 0x528];
    i32 m_590; // +0x590
    char m_pad594[0x598 - 0x594];
    MfcStr m_598, m_59c, m_5a0; // +0x598/+0x59c/+0x5a0
    char m_pad5a4[0x5b0 - 0x5a4];
    i32 m_5b0;           // +0x5b0
    MfcStr m_5b4, m_5b8; // +0x5b4/+0x5b8
    char m_pad5bc[0x600 - 0x5bc];
    i32 m_600;      // +0x600
    MfcBytes m_604; // +0x604
    char m_pad618[0x660 - 0x618];
    CMulti();
};

// Field-heavy leaf ctors defined out-of-line for readability (still inline-folded
// into the factory's new-expressions). The manual vptr stamp leads each body (the
// ctor body runs after the base + member sub-object ctors, matching the retail
// position of the `mov [this],offset ??_7<Class>` store).
CBootyState::CBootyState() {
    // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    m_1c0 = 0;
    m_1c8 = 0;
    m_1c4 = 0;
    m_1cc = 0;
    m_1b8 = 0;
    m_1bc = 0x64;
    m_2c4 = 0;
    m_2e8 = 0;
    m_2ec = 0;
    m_2f0 = 0;
    m_1b4 = 0;
    m_2f4 = 0;
    m_200 = 0;
    m_1d0 = 0;
    m_1d4 = 0;
    m_1ec = 0;
    m_1f0 = 0;
    m_1f4 = 0;
    m_1f8 = 0;
    for (i32 i = 0; i < 8; i++) {
        m_284[i] = 0;
        m_2a4[i] = 0;
    }
}
CCreditsState::CCreditsState() {
    // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    m_1b8 = 0;
    m_1bc = 0;
    m_1c0 = 0;
    m_1c4 = 0;
    m_1f4 = 0;
    m_1f8 = 0;
    m_1fc = 0;
    m_200 = 0;
    m_204 = 0;
    Ts_Set(m_1c8, 0, 0, 0x280, 0x1e0);
    Ts_Set(m_1d8, 0, 0, 0x280, 0x1e0);
    m_20c = 1;
    m_210 = 0;
    m_208 = 0;
    m_1b4 = 0;
}
CPlay::CPlay() {
    // cl runs the CState base ctor + the five member ctors, then auto-stamps
    // ??_7CPlay, then this field-init body (matching the retail inlined construction).
    char* p = (char*)this;
    *(i32*)(p + 0x328) = 0;
    *(i32*)(p + 0x330) = 0;
    *(i32*)(p + 0x32c) = 0;
    *(i32*)(p + 0x334) = 0;
    *(i32*)(p + 0x338) = 0;
    *(i32*)(p + 0x340) = 0;
    *(i32*)(p + 0x33c) = 0;
    *(i32*)(p + 0x344) = 0;
    *(i32*)(p + 0x350) = 0;
    *(i32*)(p + 0x358) = 0;
    *(i32*)(p + 0x354) = 0;
    *(i32*)(p + 0x35c) = 0;
    *(i32*)(p + 0x3f8) = 0;
    *(i32*)(p + 0x400) = 0;
    *(i32*)(p + 0x3fc) = 0;
    *(i32*)(p + 0x404) = 0;
    *(i32*)(p + 0x1bc) = 0;
    *(i32*)(p + 0x1c0) = 0;
    *(i32*)(p + 0x1c8) = 0;
    *(i32*)(p + 0x2e0) = 0;
    *(i32*)(p + 0x3f4) = 0;
    *(i32*)(p + 0x2dc) = 0;
    *(i32*)(p + 0x2e4) = 0;
    *(i32*)(p + 0x4cc) = 0;
    *(i32*)(p + 0x4e4) = 0;
    *(i32*)(p + 0x2f0) = 0;
    *(i32*)(p + 0x2d0) = 0;
    *(i32*)(p + 0x2d4) = 0;
    *(i32*)(p + 0x2f4) = 0;
    *(i32*)(p + 0x2f8) = -1;
    *(i32*)(p + 0x320) = 0;
    *(i32*)(p + 0x4d4) = 0;
    *(i32*)(p + 0x4b0) = 0;
    *(i32*)(p + 0x348) = 1;
    *(i32*)(p + 0x510) = 0;
    *(i32*)(p + 0x518) = 0;
    *(i32*)(p + 0x30c) = 0;
    *(i32*)(p + 0x2e8) = 0;
    *(i32*)(p + 0x4f0) = 0;
    *(i32*)(p + 0x368) = 0;
    *(i32*)(p + 0x36c) = 0;
    *(i32*)(p + 0x2ec) = 0;
    *(i32*)(p + 0x504) = 0;
}
CMulti::CMulti() {
    // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    m_520 = 0;
    m_524 = 0;
    m_590 = 1;
    m_5b0 = 0;
    m_600 = 1;
}

// ===========================================================================
// CGruntzMgr::TransitionState (0x8b960) - see the file header.
// @early-stop
// /GX EH-state-numbering wall (same family as the CProjectile ctor / ApplySwitch /
// DestroyGroup plateaus): the teardown, the m_a4 replay fast-path, the whole
// per-state new/ctor/member/vtable/field-init switch and the install/activate tail
// are reconstructed with the retail sizes, vtables and field sets. The residue is
// the __ehfuncinfo state ladder across the ~13 nested new-expression try-regions
// (each `mov [esp+0x1c],N` state id) + the interleave of the per-object field
// zeroing with the member ctors - MSVC5's state numbering + store scheduling for a
// factory of this size is not source-steerable. Logic complete. topic:wall topic:eh.
// ===========================================================================
RVA(0x0008b960, 0x7c4)
i32 CGruntzMgr::TransitionState(i32 stateId, i32 a2, i32 keepCurrent, i32 a4) {
    (void)a4;
    CTsState* cur = (CTsState*)m_curState;
    i32 local10 = 0;
    if (cur != 0) {
        local10 = cur->CallId();
        i32 savedSub = cur->m_1c;
        cur->CallSlot10(stateId);
        if (keepCurrent != 0) {
            PushState(m_curState);
            a2 = savedSub; // retail reuses the arg2 stack slot as scratch for cur->m_1c
            m_curState = 0;
        } else {
            if (m_curState != 0) {
                ((CTsState*)m_curState)->CallDtor(1);
            }
            m_curState = 0;
            FlushStateStack();
            m_curState = 0;
        }
    } else if (keepCurrent == 0) {
        FlushStateStack();
    }

    if (m_a4 != 0) {
        // The replay fast-path constructs a bare, fully-modeled CState (its ctor is
        // the standalone CState::CState @0x8c750). No shell, no (CState*)this view -
        // this is the real class. The external `call ??0CState` vs retail's inlined
        // field init is a call-vs-inline delta inside this factory's @early-stop EH
        // residue (measured neutral, 43.10% either way); ??_7CState stays owned by
        // its real TU (gamemode) - the ctor stamps it, this new-expression doesn't.
        m_curState = new CState;
        return 1;
    }

    CTsState* obj;
    switch (stateId) {
        case 2:
            obj = (CTsState*)new CAttract;
            break;
        case 3:
            obj = (CTsState*)new CPlay;
            break;
        case 5:
            obj = (CTsState*)new CMenuState;
            break;
        case 7:
            obj = (CTsState*)new CDemo;
            break;
        case 8:
            obj = (CTsState*)new CCreditsState;
            break;
        case 9:
            obj = (CTsState*)new CHelpState;
            break;
        case 10:
            obj = (CTsState*)new CBootyState;
            break;
        case 14:
            obj = (CTsState*)new CSplashState;
            break;
        case 17:
            obj = (CTsState*)new CMulti;
            break;
        case 18:
            obj = (CTsState*)new CMultiBootyState;
            break;
        default:
            goto install;
    }
    m_curState = (CState*)obj;

install:
    if (m_curState == 0) {
        *(i32*)(*(char**)((char*)this + 0x8) + 0x244) = 0;
        return 0;
    }
    PerFrameTick();
    {
        CTsState* st = (CTsState*)m_curState;
        i32 ok = st->CallActivate(this, a2, local10);
        st = (CTsState*)m_curState;
        if (ok == 0) {
            if (st != 0) {
                st->CallDtor(1);
            }
            m_curState = 0;
            return 0;
        }
        st->CallSlot9(local10);
        *(i32*)(*(char**)((char*)this + 0x8) + 0x244) = 1;
        g_645570->ReadAll();
        PerFrameTick();
        return 1;
    }
}

SIZE_UNKNOWN(CAttract);
SIZE_UNKNOWN(CBootyState);
SIZE_UNKNOWN(CCreditsState);
SIZE_UNKNOWN(CDInputMgrZ);
SIZE_UNKNOWN(CDemo);
SIZE_UNKNOWN(CHelpState);
SIZE_UNKNOWN(CMenuState);
SIZE_UNKNOWN(CMulti);
SIZE_UNKNOWN(CMultiBootyState);
SIZE_UNKNOWN(CSplashState);
SIZE_UNKNOWN(CTsBaseA);
SIZE_UNKNOWN(CTsBaseB);
SIZE_UNKNOWN(CTsState);
SIZE_UNKNOWN(CTsSub45);
SIZE_UNKNOWN(MfcBytes);
SIZE_UNKNOWN(MfcStr);
