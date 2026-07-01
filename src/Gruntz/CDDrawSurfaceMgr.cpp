#include <rva.h>
// HarryPotter.cpp - root object of the tomalla-named DDraw surface/page-manager
// family. CDDrawSurfaceMgr is the owner stored off CGruntzMgr
// +0x30; it holds one child manager pointer per slot and a pair of global draw
// clock mirrors reset by the ctor.
//
// Names are tomalla placeholders. Offsets, store order, vtable slots, and global
// addresses are load-bearing for matching.

// HWND comes from the real <windows.h> (via Win32.h; pure-Win32 TU, no MFC).
#include <Win32.h>
typedef i32 intptr_t; // VC5 predates <stdint.h>; the one HP_Callback cast below needs it.

class CDDrawSubMgrItem {
public:
    i32 m_10;
    i32 m_14;
};

class CDDrawSubMgr {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual i32 Vfunc14();

    void* m_04;
    CDDrawSubMgrItem* m_10;
};

struct MinervaMgr; // defined below; m_28 points at one

// The CObject-like grand base (vtable @0x5e8cb4): the implicit vptr @+0x00 + the
// 5-slot CObject interface, with the scalar-deleting dtor at slot 1. Real
// polymorphic base subobject: the empty inline virtual dtor makes cl emit the
// implicit grand-base re-stamp (reloc-masks 0x5e8cb4) folded LAST into
// ~CDDrawSurfaceMgr, and the destructible base subobject supplies the dtor's /GX
// EH frame. Placing it here also fixes the derived vtable layout: the dtor lands
// at slot 1 and UnknownVirtualMethod14 (byte 0x14) at slot 5, etc.
struct CDDrawSurfaceMgrBase {
    virtual void V0();               // slot 0 (sub_1bef01)
    virtual ~CDDrawSurfaceMgrBase(); // slot 1 (scalar-deleting dtor)
    virtual void V2();               // slot 2 (sub_0028ec)
    virtual void V3();               // slot 3 (sub_00106e)
    virtual void V4();               // slot 4 (sub_004034)
};
inline CDDrawSurfaceMgrBase::~CDDrawSurfaceMgrBase() {}

class CDDrawSurfaceMgr : public CDDrawSurfaceMgrBase {
public:
    CDDrawSurfaceMgr();
    virtual ~CDDrawSurfaceMgr();
    virtual i32 UnknownVirtualMethod14();
    virtual i32 UnknownVirtualMethod18(HWND hWnd, i32 width, i32 height, i32 bpp, i32 flagsUnknown);
    virtual void UnknownVirtualMethod1C();
    virtual void UnknownVirtualMethod20();
    virtual i32 UnknownVirtualMethod24(i32 x, i32 y, i32 flags);
    virtual void UnknownVirtualMethod28(void* hWnd);
    virtual i32 UnknownVirtualMethod2C(i32 unknown);
    virtual i32
    UnknownVirtualMethod30(i32 width, i32 height, i32 bpp, i32 flagsUnknown, void* callback);
    virtual i32
    UnknownVirtualMethod34(i32 width, i32 height, i32 bpp, i32 flagsUnknown, void* callback);
    virtual i32 UnknownVirtualMethod38(void* arg1, i32 arg2, i32 arg3, i32 arg4);

    // Engine-label backlog stubs.
    void UnknownVirtualMethod18();

    // Owned-child teardown helper, called by ~CDDrawSurfaceMgr (0x1558b0).
    void Cleanup_155e20();

    CDDrawSubMgr* m_04; // +0x04  Draco
    CDDrawSubMgr* m_08; // +0x08  Hermiona
    CDDrawSubMgr* m_0c; // +0x0c  Hagrid
    CDDrawSubMgr* m_10; // +0x10  Severus
    CDDrawSubMgr* m_14; // +0x14  Sirius
    CDDrawSubMgr* m_18; // +0x18  Albus
    void* m_1c;         // +0x1c  Filch
    void* m_20;         // +0x20  Voldemort
    CDDrawSubMgr* m_24; // +0x24  Remus
    MinervaMgr* m_28;   // +0x28  Minerva
    void* m_2c;         // +0x2c  Pettigrew
    HWND m_hWnd;        // +0x30
    i32 m_flags;        // +0x34
    i32 m_38;           // +0x38
    i32 m_3c;           // +0x3c
};

DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0; // draw-clock mirror
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc; // draw-delta mirror

// Polymorphic-delete views for the owned children torn down by Cleanup_155e20.
// Most children carry the engine scalar-deleting destructor at vtable slot 1
// (`mov eax,[child]; push 1; call [eax+4]`); the Voldemort surface at +0x20 has
// it at slot 0 (`call [eax]`). The Filch object at +0x1c is a heap object with a
// non-virtual __thiscall dtor + an explicit RezFree.
struct DDChildSlot1 {
    virtual void Slot00();
    virtual void Destroy(u32 flags); // slot 1: scalar-deleting destructor
};
struct DDChildSlot0 {
    virtual void Destroy(u32 flags); // slot 0: scalar-deleting destructor
};
struct FilchObj {
    void Dtor(); // 0x141d50, __thiscall /GX destructor
};
extern "C" void RezFree(void* p); // 0x1b9b82, __cdecl engine operator delete

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::CDDrawSurfaceMgr()
// Stamps the vftable, clears every owned-child pointer except hwnd (+0x30), clears
// flags/bookkeeping at +0x34/+0x38/+0x3c, then resets the two draw-clock globals.
RVA(0x00155840, 0x41)
CDDrawSurfaceMgr::CDDrawSurfaceMgr() {
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 0;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_2c = 0;
    m_flags = 0;
    m_38 = 0;
    m_3c = 0;
    g_6bf3c0 = 0;
    g_6bf3bc = 0;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::~CDDrawSurfaceMgr() (0x1558b0, __thiscall, /GX)
// Real polymorphic with the CObject base subobject now: cl emits the implicit
// ??_7CDDrawSurfaceMgr own-vptr stamp in the ENTRY state (stamp-first), runs the
// owned-child teardown (Cleanup_155e20), then the empty ~CDDrawSurfaceMgrBase
// folds the implicit grand-base re-stamp last. The destructible base subobject
// supplies the /GX EH frame. (eh-dtor-implicit-vptr-stamp-first.md /
// eh-dtor-needs-base-subobject.md.)
RVA(0x001558b0, 0x46)
CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {
    Cleanup_155e20();
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::Cleanup_155e20() (0x155e20, __thiscall)
// Tear down every owned child in the engine's fixed teardown order. Each child
// carries the scalar-deleting destructor at vtable slot 1 (slot 0 for the
// Voldemort surface at +0x20); the Filch object at +0x1c is a heap object freed
// by its non-virtual dtor + RezFree. Frameless (the dev's explicit teardown, not
// a compiler member-dtor chain), so no /GX frame here.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): body, the
// child order, the slot-1/slot-0 deletes, the m_1c cache+RezFree, and every store
// are byte-exact; only the callee-saved register holding the constant 0 differs
// (retail pins ebx for the zero / edi for the cached m_1c; cl pins edi=0 / ebx=m_1c),
// a 1-instr phase shift through the =0 stores. No source lever flips the allocation.
// ~96%; defer to the final sweep.
RVA(0x00155e20, 0xd1)
void CDDrawSurfaceMgr::Cleanup_155e20() {
    if (m_24) {
        ((DDChildSlot1*)m_24)->Destroy(1);
        m_24 = 0;
    }
    if (m_28) {
        ((DDChildSlot1*)m_28)->Destroy(1);
        m_28 = 0;
    }
    if (m_20) {
        ((DDChildSlot0*)m_20)->Destroy(1);
        m_20 = 0;
    }
    if (m_04) {
        ((DDChildSlot1*)m_04)->Destroy(1);
        m_04 = 0;
    }
    if (m_08) {
        ((DDChildSlot1*)m_08)->Destroy(1);
        m_08 = 0;
    }
    if (m_0c) {
        ((DDChildSlot1*)m_0c)->Destroy(1);
        m_0c = 0;
    }
    if (m_10) {
        ((DDChildSlot1*)m_10)->Destroy(1);
        m_10 = 0;
    }
    if (m_14) {
        ((DDChildSlot1*)m_14)->Destroy(1);
        m_14 = 0;
    }
    if (m_18) {
        ((DDChildSlot1*)m_18)->Destroy(1);
        m_18 = 0;
    }
    if (m_2c) {
        ((DDChildSlot1*)m_2c)->Destroy(1);
        m_2c = 0;
    }
    FilchObj* filch = (FilchObj*)m_1c;
    if (filch) {
        filch->Dtor();
        RezFree(filch);
        m_1c = 0;
    }
    m_3c = 0;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod14()
// Returns whether the core child managers are present and the first child accepts
// its +0x14 virtual readiness check.
RVA(0x00155f00, 0x41)
i32 CDDrawSurfaceMgr::UnknownVirtualMethod14() {
    CDDrawSubMgr* first = m_04;

    if (first == 0) {
        goto fail;
    }
    if (m_08 == 0) {
        goto fail;
    }
    if (m_0c == 0) {
        goto fail;
    }
    if (m_10 == 0) {
        goto fail;
    }
    if (m_14 == 0) {
        goto fail;
    }
    if (first->Vfunc14() == 0) {
        goto fail;
    }
    if (m_24 != 0) {
        return 1;
    }

fail:
    return 0;
}

// Helper structs for __thiscall external functions.
struct MinervaInner {
    void Free();
};
struct VoldemortObj {
    void Free();
};
struct MinervaMgr {
    void ClearMap();
    char m_pad00[0x2c];
    MinervaInner* m_2c; // +0x2c  inner (Free'd on context teardown)
};
extern void __cdecl RelayHwnd(void* hWnd);
extern i32 __stdcall CreateChildSurface(i32 x, i32 y, i32 flags);
struct RemusCoordsHelper {
    i32 SetCoords(i32 x, i32 y);
};
typedef i32(__cdecl* HP_Callback)(void*, void*, i32, i32, i32);

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod20()
// Frees context — cleans up the Voldemort surface and the Minerva map.
RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::UnknownVirtualMethod20() {
    if (m_28 != 0) {
        MinervaInner* inner = m_28->m_2c;
        if (inner != 0) {
            inner->Free();
        }
        m_28->ClearMap();
    }
    if (m_20 != 0) {
        ((VoldemortObj*)m_20)->Free();
    }
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod24()
// Validates/sets surface dimensions.
RVA(0x00155f60, 0x56)
i32 CDDrawSurfaceMgr::UnknownVirtualMethod24(i32 x, i32 y, i32 flags) {
    CDDrawSubMgrItem* child = m_04->m_10;
    if (child->m_10 != x || child->m_14 != y) {
        if (CreateChildSurface(x, y, flags) == 0) {
            return 0;
        }
    }
    if (m_24 != 0) {
        if (((RemusCoordsHelper*)m_24)->SetCoords(x, y) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod28()
// Relays the hWnd argument to an external manager function.
RVA(0x00155f50, 0x10)
void CDDrawSurfaceMgr::UnknownVirtualMethod28(void* hWnd) {
    RelayHwnd(hWnd);
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod38()
// Dispatches arguments through the m_3c callback function pointer,
// returning 1 on success / 0 on failure.
RVA(0x00156a90, 0x3a)
i32 CDDrawSurfaceMgr::UnknownVirtualMethod38(void* arg1, i32 arg2, i32 arg3, i32 arg4) {
    if (!arg1) {
        return 0;
    }
    if (!m_3c) {
        return 0;
    }
    return ((HP_Callback)(intptr_t)m_3c)(this, arg1, arg2, arg3, arg4) != 0;
}

// Out-of-line stubs so the vftable is emitted in this TU. They are not claimed
// as matched in symbol_names.csv.
i32 CDDrawSurfaceMgr::UnknownVirtualMethod18(HWND, i32, i32, i32, i32) {
    return 0;
}
void CDDrawSurfaceMgr::UnknownVirtualMethod1C() {}
i32 CDDrawSurfaceMgr::UnknownVirtualMethod2C(i32) {
    return 0;
}
i32 CDDrawSurfaceMgr::UnknownVirtualMethod30(i32, i32, i32, i32, void*) {
    return 0;
}
i32 CDDrawSurfaceMgr::UnknownVirtualMethod34(i32, i32, i32, i32, void*) {
    return 0;
}

// Engine-label backlog stubs (moved from src/Stub/CDDrawSurfaceMgr.cpp).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00155900, 0x519)
void CDDrawSurfaceMgr::UnknownVirtualMethod18() {}

// class-metadata sweep: size annotations (SIZE_UNKNOWN = retail size TBD).
SIZE_UNKNOWN(CDDrawSubMgrItem);
SIZE_UNKNOWN(CDDrawSurfaceMgrBase);
SIZE_UNKNOWN(DDChildSlot0);
SIZE_UNKNOWN(DDChildSlot1);
SIZE_UNKNOWN(FilchObj);
SIZE_UNKNOWN(MinervaMgr);
SIZE_UNKNOWN(RemusCoordsHelper);
SIZE_UNKNOWN(VoldemortObj);
