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
// CPlay.h / CMulti.h); here they are modeled as sized shells carrying only the
// destructible members (so the EH-state ladder emits) + the manual retail-vtable
// stamp (the polymorphic vtables live in other TUs). Field names are placeholders;
// offsets + code bytes are load-bearing.
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

// operator new / the game heap (0x1b9b46), reloc-masked.
void* operator new(u32);

// The retail state vtables, referenced by address (reloc-masked DATA externs; the
// classes' real vtables live in other TUs, so a polymorphic model would emit a
// divergent ??_7 - the manual stamp is the transitional workaround).
DATA(0x001ea21c)
extern void* g_vtbl_CState[]; // 0x5ea21c
DATA(0x001ea194)
extern void* g_vtbl_5ea194[]; // 0x5ea194
DATA(0x001ea0bc)
extern void* g_vtbl_CPlay[]; // 0x5ea0bc
DATA(0x001e9fe4)
extern void* g_vtbl_CMulti[]; // 0x5e9fe4
DATA(0x001e9f0c)
extern void* g_vtbl_5e9f0c[]; // 0x5e9f0c
DATA(0x001e9e84)
extern void* g_vtbl_5e9e84[]; // 0x5e9e84
DATA(0x001e9dfc)
extern void* g_vtbl_5e9dfc[]; // 0x5e9dfc
DATA(0x001e9d74)
extern void* g_vtbl_5e9d74[]; // 0x5e9d74
DATA(0x001e9cec)
extern void* g_vtbl_5e9cec[]; // 0x5e9cec  (CBootyState)
DATA(0x001e9c64)
extern void* g_vtbl_5e9c64[]; // 0x5e9c64
DATA(0x001e9bdc)
extern void* g_vtbl_5e9bdc[]; // 0x5e9bdc  (CMultiBootyState)

// The DirectInputMgr2 singleton (*g_645570) re-armed after the install (ReadAll).
struct CDInputMgrZ {
    void ReadAll(); // 0x133110
};
DATA(0x00245570)
extern CDInputMgrZ* g_645570;

// The vtable-dispatch view of a state object: the factory drives the current
// state through slots 0 (scalar-deleting dtor), 1 (activate), 4 (id), 9 and 10.
struct CTsState {
    virtual void v00(u32 flags);                      // slot 0  scalar-deleting dtor
    virtual i32 v01(CGruntzMgr* mgr, i32 a2, i32 a3); // slot 1  activate
    virtual void v02();
    virtual void v03();
    virtual i32 v04(); // slot 4  id/update
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual i32 v09(i32 a);    // slot 9
    virtual i32 v0a(i32 a);    // slot 10 (+0x28)
    char m_pad2c[0x1c - 0x0c]; // to +0x1c
    i32 m_1c;                  // +0x1c  sub-object saved on teardown
};

// The minimal destructible MFC members that force the per-object EH-state ladder;
// their ctors/dtors are the reloc-masked NAFXCW bodies (0x1b9b93 / 0x1b4f0b ...).
struct MfcStr {
    MfcStr();
    ~MfcStr();
    char* m_p;
};
struct MfcBytes {
    MfcBytes();
    ~MfcBytes();
    void* m_vptr;
    void* m_data;
    i32 m_size, m_max, m_grow;
};

// The two out-of-line base ctors (0x8c750 = CState base; 0x8c9d0 = CPlay base for
// the multiplayer/param-7 states). Declared no-body -> reloc-masked base calls.
struct CTsBaseA {
    CTsBaseA(); // 0x8c750
    char m_cstate[0x1b4];
};
struct CTsBaseB {
    CTsBaseB(); // 0x8c9d0 (CPlay layout, 0x520)
    char m_cplay[0x520];
};

// Two extra member sub-objects the param-8 state builds (a small ctor @0x8c3b0 and
// the two 4-arg Set @0x8c380 initializers).
struct CTsSub45 {
    CTsSub45(); // 0x8c3b0
    char m_pad[8];
};
void Ts_Set(void* self, i32 a, i32 b, i32 c, i32 d); // 0x8c380 (member Set, 4 args)

// ---- the CState-derived state shells (sized to the retail objects) ----------
struct StPlain2 : CTsBaseA { // param 2, 0x1c0, vtbl 0x5ea194
    char m_pad[0x1c0 - 0x1b4];
    StPlain2() {
        *(void**)this = g_vtbl_5ea194;
    }
};
struct StPlain5 : CTsBaseA { // param 5, 0x1c0, vtbl 0x5e9e84
    char m_pad[0x1c0 - 0x1b4];
    StPlain5() {
        *(void**)this = g_vtbl_5e9e84;
        *(i32*)((char*)this + 0x1b4) = 0;
    }
};
struct StPlain9 : CTsBaseA { // param 9, 0x1b8, vtbl 0x5e9dfc
    char m_pad[0x1b8 - 0x1b4];
    StPlain9() {
        *(void**)this = g_vtbl_5e9dfc;
    }
};
struct StPlain14 : CTsBaseA { // param 14, 0x1bc, vtbl 0x5e9d74
    char m_pad[0x1bc - 0x1b4];
    StPlain14() {
        *(void**)this = g_vtbl_5e9d74;
        *(i32*)((char*)this + 0x1b4) = 0;
    }
};
struct StPlain7 : CTsBaseB { // param 7, 0x528, vtbl 0x5e9f0c
    char m_pad[0x528 - 0x520];
    StPlain7() {
        *(void**)this = g_vtbl_5e9f0c;
    }
};
struct StMultiBooty : CTsBaseA { // param 18, 0x244, vtbl 0x5e9bdc
    char m_pad[0x244 - 0x1b4];
    StMultiBooty() {
        *(void**)this = g_vtbl_5e9bdc;
        *(i32*)((char*)this + 0x1b4) = 0;
        *(i32*)((char*)this + 0x1b8) = 0x64;
    }
};
struct StBooty : CTsBaseA { // param 10, 0x320, vtbl 0x5e9cec
    char m_pad[0x320 - 0x1b4];
    StBooty();
};
struct StParam8 : CTsBaseA { // param 8, 0x218, vtbl 0x5e9c64
    char m_pad1b4[0x1e8 - 0x1b4];
    CTsSub45 m_1e8; // +0x1e8
    MfcStr m_1f0;   // +0x1f0
    char m_pad1f4[0x218 - 0x1f4];
    StParam8();
};
struct StPlay : CTsBaseA { // param 3 (CPlay), 0x520, vtbl 0x5ea0bc
    MfcStr m_1b4;          // +0x1b4
    char m_pad1b8[0x370 - 0x1b8];
    MfcBytes m_370; // +0x370
    char m_pad384[0x3a4 - 0x384];
    MfcBytes m_3a4[4]; // +0x3a4
    char m_pad3f4[0x410 - 0x3f4];
    MfcStr m_410; // +0x410
    char m_pad414[0x488 - 0x414];
    MfcBytes m_488; // +0x488
    char m_pad49c[0x520 - 0x49c];
    StPlay();
};
struct StMulti : CTsBaseB { // param 17 (CMulti), 0x660, vtbl 0x5e9fe4
    char m_pad520[0x598 - 0x520];
    MfcStr m_598, m_59c, m_5a0; // +0x598/+0x59c/+0x5a0
    char m_pad5a4[0x5b4 - 0x5a4];
    MfcStr m_5b4, m_5b8; // +0x5b4/+0x5b8
    char m_pad5bc[0x604 - 0x5bc];
    MfcBytes m_604; // +0x604
    char m_pad618[0x660 - 0x618];
    StMulti();
};

// Field-heavy ctors defined out-of-line for readability (still inline-folded into
// the factory's new-expressions).
StBooty::StBooty() {
    char* p = (char*)this;
    *(i32*)(p + 0x1c0) = 0;
    *(i32*)(p + 0x1c8) = 0;
    *(i32*)(p + 0x1c4) = 0;
    *(i32*)(p + 0x1cc) = 0;
    *(void**)this = g_vtbl_5e9cec;
    *(i32*)(p + 0x1b8) = 0;
    *(i32*)(p + 0x1bc) = 0x64;
    *(i32*)(p + 0x2c4) = 0;
    *(i32*)(p + 0x2e8) = 0;
    *(i32*)(p + 0x2ec) = 0;
    *(i32*)(p + 0x2f0) = 0;
    *(i32*)(p + 0x1b4) = 0;
    *(i32*)(p + 0x2f4) = 0;
    *(i32*)(p + 0x200) = 0;
    *(i32*)(p + 0x1d0) = 0;
    *(i32*)(p + 0x1d4) = 0;
    *(i32*)(p + 0x1ec) = 0;
    *(i32*)(p + 0x1f0) = 0;
    *(i32*)(p + 0x1f4) = 0;
    *(i32*)(p + 0x1f8) = 0;
    for (i32 i = 0; i < 8; i++) {
        *(i32*)(p + 0x284 + i * 4) = 0;
        *(i32*)(p + 0x2a4 + i * 4) = 0;
    }
}
StParam8::StParam8() {
    char* p = (char*)this;
    *(void**)this = g_vtbl_5e9c64;
    *(i32*)(p + 0x1b8) = 0;
    *(i32*)(p + 0x1bc) = 0;
    *(i32*)(p + 0x1c0) = 0;
    *(i32*)(p + 0x1c4) = 0;
    *(i32*)(p + 0x1f4) = 0;
    *(i32*)(p + 0x1f8) = 0;
    *(i32*)(p + 0x1fc) = 0;
    *(i32*)(p + 0x200) = 0;
    *(i32*)(p + 0x204) = 0;
    Ts_Set(p + 0x1c8, 0, 0, 0x280, 0x1e0);
    Ts_Set(p + 0x1d8, 0, 0, 0x280, 0x1e0);
    *(i32*)(p + 0x20c) = 1;
    *(i32*)(p + 0x210) = 0;
    *(i32*)(p + 0x208) = 0;
    *(i32*)(p + 0x1b4) = 0;
}
StPlay::StPlay() {
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
    *(void**)this = g_vtbl_CPlay;
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
StMulti::StMulti() {
    char* p = (char*)this;
    *(void**)this = g_vtbl_CMulti;
    *(i32*)(p + 0x520) = 0;
    *(i32*)(p + 0x524) = 0;
    *(i32*)(p + 0x590) = 1;
    *(i32*)(p + 0x5b0) = 0;
    *(i32*)(p + 0x600) = 1;
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
        local10 = cur->v04();
        i32 savedSub = cur->m_1c;
        cur->v0a(stateId);
        if (keepCurrent != 0) {
            PushState(m_curState);
            a2 = savedSub; // retail reuses the arg2 stack slot as scratch for cur->m_1c
            m_curState = 0;
        } else {
            if (m_curState != 0) {
                ((CTsState*)m_curState)->v00(1);
            }
            m_curState = 0;
            FlushStateStack();
            m_curState = 0;
        }
    } else if (keepCurrent == 0) {
        FlushStateStack();
    }

    if (m_a4 != 0) {
        char* p = (char*)operator new(0x1b4);
        if (p != 0) {
            *(void**)p = g_vtbl_CState;
            *(i32*)(p + 0x4) = 0;
            *(i32*)(p + 0x8) = 0;
            *(i32*)(p + 0xc) = 0;
            *(i32*)(p + 0x28) = 0;
            *(i32*)(p + 0x2c) = 0;
            *(i32*)(p + 0x14) = 0;
            *(i32*)(p + 0x18) = 0;
            *(i32*)(p + 0x38) = 0;
            *(i32*)(p + 0x3c) = 0;
            *(char*)(p + 0x4c) = 0;
            *(i32*)(p + 0x24) = 0;
            *(i32*)(p + 0x160) = 0;
            *(i32*)(p + 0x164) = 0;
            *(i32*)(p + 0x168) = 0;
            *(i32*)(p + 0x170) = 0x40;
            *(i32*)(p + 0x16c) = 0;
            *(i32*)(p + 0x174) = 0x40;
            *(i32*)(p + 0x178) = 0;
            *(i32*)(p + 0x180) = 0x40;
            *(i32*)(p + 0x17c) = 0;
            *(i32*)(p + 0x184) = 0x40;
            *(i32*)(p + 0x188) = 0;
            *(i32*)(p + 0x190) = 0;
            *(i32*)(p + 0x18c) = 0;
            *(i32*)(p + 0x194) = 0;
            *(i32*)(p + 0x198) = 0;
            *(i32*)(p + 0x1a0) = 0;
            *(i32*)(p + 0x19c) = 0;
            *(i32*)(p + 0x1a4) = 0;
            *(i32*)(p + 0x150) = 0;
            *(i32*)(p + 0x154) = 0;
        }
        m_curState = (CState*)p;
        return 1;
    }

    CTsState* obj;
    switch (stateId) {
        case 2:
            obj = (CTsState*)new StPlain2;
            break;
        case 3:
            obj = (CTsState*)new StPlay;
            break;
        case 5:
            obj = (CTsState*)new StPlain5;
            break;
        case 7:
            obj = (CTsState*)new StPlain7;
            break;
        case 8:
            obj = (CTsState*)new StParam8;
            break;
        case 9:
            obj = (CTsState*)new StPlain9;
            break;
        case 10:
            obj = (CTsState*)new StBooty;
            break;
        case 14:
            obj = (CTsState*)new StPlain14;
            break;
        case 17:
            obj = (CTsState*)new StMulti;
            break;
        case 18:
            obj = (CTsState*)new StMultiBooty;
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
        i32 ok = st->v01(this, a2, local10);
        st = (CTsState*)m_curState;
        if (ok == 0) {
            if (st != 0) {
                st->v00(1);
            }
            m_curState = 0;
            return 0;
        }
        st->v09(local10);
        *(i32*)(*(char**)((char*)this + 0x8) + 0x244) = 1;
        g_645570->ReadAll();
        PerFrameTick();
        return 1;
    }
}

SIZE_UNKNOWN(CDInputMgrZ);
SIZE_UNKNOWN(CTsBaseA);
SIZE_UNKNOWN(CTsBaseB);
SIZE_UNKNOWN(CTsState);
SIZE_UNKNOWN(CTsSub45);
SIZE_UNKNOWN(MfcBytes);
SIZE_UNKNOWN(MfcStr);
SIZE_UNKNOWN(StBooty);
SIZE_UNKNOWN(StMulti);
SIZE_UNKNOWN(StMultiBooty);
SIZE_UNKNOWN(StParam8);
SIZE_UNKNOWN(StPlain14);
SIZE_UNKNOWN(StPlain2);
SIZE_UNKNOWN(StPlain5);
SIZE_UNKNOWN(StPlain7);
SIZE_UNKNOWN(StPlain9);
SIZE_UNKNOWN(StPlay);
