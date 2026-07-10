// OrphanLeaves.cpp - a handful of tiny orphan-COMDAT leaf functions (getters,
// global teardowns, member sub-object dtors, a range registrar, a 2-field setter)
// that survive in the low-RVA COMDAT pool with no recoverable owning class. Each
// is modeled from its disassembly with a PLACEHOLDER class/name; only the OFFSETS
// + emitted code bytes are load-bearing (campaign doctrine). All engine callees
// are external/no-body so their call rel32 / DIR32 reloc-mask.
#include <Ints.h>
#include <Wap32/ZDArrayDerived.h>
#include <rva.h>
#include <Mfc.h> // CString (0x1b9b93 default ctor)
#include <new>
#include <Globals.h>
#include <Wap32/ZVec.h>

// ---------------------------------------------------------------------------
// 0x183d0: return the address of a fixed global (a runtime-class / map pointer).

RVA(0x000183d0, 0x6)
void* GetGlobal5e8e98() {
    return &g_5e8e98;
}

// ---------------------------------------------------------------------------
// 0x183f0 (RVA-homed from src/Stub/ApiCallers.cpp) - a dialog list-item confirm:
// send LB_GETCURSEL (0x188) to item 0x516; if it returned a valid selection
// (!= LB_ERR), run OnPick(). __thiscall, no args.
// @orphan: only inbound edge is a fn-ptr-table slot (~g_5e8e98+0x1c, via thunk
// 0x3d5f) - no class vtable / new-site trace, so the owning dialog class is unrecovered.
struct DlgHostItem_183f0 {
    char m_pad0[0x1c];
    HWND m_hwnd; // +0x1c
};
struct DlgHost_183f0 {
    DlgHostItem_183f0* GetItem(i32 id); // thiscall, RVA 0x1be27d
    void OnPick();                      // thiscall, RVA 0x1bacc3
    void PickIfSelected();              // thiscall, RVA 0x183f0
};
RVA(0x000183f0, 0x2e)
void DlgHost_183f0::PickIfSelected() {
    HWND h = GetItem(0x516)->m_hwnd;
    if (SendMessageA(h, 0x188, 0, 0) != -1) {
        OnPick();
    }
}
SIZE_UNKNOWN(DlgHostItem_183f0);
SIZE_UNKNOWN(DlgHost_183f0);

// ---------------------------------------------------------------------------
// 0x3ac30: tear down a global CString-like object (tail-call its Free at 0x1b9b93).
DATA(0x0022c25c)
extern CString g_62c25c;

RVA(0x0003ac30, 0xa)
void FreeGlobal62c25c() {
    new (&g_62c25c) CString();
}

// ---------------------------------------------------------------------------
// 0x3cbc0 / 0x3cbf0: member sub-object destructors - run the embedded sub-object's
// two-phase teardown (a derived clear + the shared base dtor 0x169d70).
struct CSubObjC {
    void Clear();    // 0x16a240
    void BaseDtor(); // 0x169d70
};
struct CSubObj8 {
    void Clear();    // 0x16a8e0
    void BaseDtor(); // 0x169d70
};
struct COwnerWithSubs {
    char _00[0x08];
    CSubObj8 m_08; // +0x08  (empty reloc-masked view)
    char _09[0x0c - 0x09];
    CSubObjC m_0c;   // +0x0c
    void DtorSubC(); // 0x3cbc0
    void DtorSub8(); // 0x3cbf0  (acts on +0x08)
};

RVA(0x0003cbc0, 0x14)
void COwnerWithSubs::DtorSubC() {
    CSubObjC* s = &m_0c;
    s->Clear();
    s->BaseDtor();
}

RVA(0x0003cbf0, 0x14)
void COwnerWithSubs::DtorSub8() {
    CSubObj8* s = &m_08;
    s->Clear();
    s->BaseDtor();
}

// ---------------------------------------------------------------------------
// 0x3e120: register the default activation-id range [0x7d0, 0x7da] on a per-class
// registry (g_6446d8) via the shared SetActiveRange ILT thunk (0x3742).
DATA(0x002446d8)
extern CZDArrayDerived g_6446d8;

RVA(0x0003e120, 0x15)
void Register6446d8Range() {
    ((CZDArrayDerived*)&g_6446d8)->Construct(0x7d0, 0x7da);
}

// ---------------------------------------------------------------------------
// 0x75a10: a 2-field setter (CPoint/CSize-style) that fills m_0/m_4 and returns this.
struct CPairXY {
    i32 m_0;
    i32 m_4;
    RVA(0x00075a10, 0x12)
    CPairXY* Set(i32 a, i32 b) {
        m_0 = a;
        m_4 = b;
        return this;
    }
};

// CPairXY::Set (0x00075a10) is now an inline member in the header.


// ---------------------------------------------------------------------------
// 0xb4330: run a one-shot helper (ILT 0x2914) and return 0.
extern "C" void Helper2914(); // 0x2914 (ILT thunk)

RVA(0x000b4330, 0x8)
i32 RunHelper2914() {
    Helper2914();
    return 0;
}
SIZE_UNKNOWN(CZDArrayDerived);
SIZE_UNKNOWN(COwnerWithSubs);
SIZE_UNKNOWN(CPairXY);
SIZE_UNKNOWN(CSubObj8);
SIZE_UNKNOWN(CSubObjC);
