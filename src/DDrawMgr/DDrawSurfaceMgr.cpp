#include <rva.h>
// DDrawSurfaceMgr.cpp - root object of the tomalla-named DDraw surface/page-manager
// family. CDDrawSurfaceMgr is the owner stored off CGruntzMgr
// +0x30; it holds one child manager pointer per slot and a pair of global draw
// clock mirrors reset by the ctor.
//
// Names are tomalla placeholders. Offsets, store order, vtable slots, and global
// addresses are load-bearing for matching.

// <Mfc.h> FIRST (superset of Win32.h: same <windows.h> for HWND + the MFC classes,
// e.g. CMapStringToOb in the leaf-scan child). The old "pure-Win32, C1189 wall"
// note was wrong - afx.h pulls windows.h the afx-first way, so no C1189.
#include <Mfc.h>
#include <Wap32/Object.h>             // Wap::CObject - the shared engine grand-base
#include <DDrawMgr/DDrawSurfaceMgr.h> // THE canonical CDDrawSurfaceMgr class shape

class CDDrawSubMgrItem {
public:
    i32 m_width;  // +0x10  current surface width
    i32 m_height; // +0x14  current surface height
};

// The owned child managers: polymorphic engine objects with the scalar-deleting
// destructor at vtable slot 1 (`mov eax,[child]; push 1; call [eax+4]`). Modeling
// that as a real virtual folds the former per-slot delete-view casts.
class CDDrawSubMgr {
public:
    virtual void FUN_005bef01();     // [0] 0x1bef01 (dispatch view, declared-only)
    virtual void Destroy(u32 flags); // [1] scalar-deleting destructor
    virtual void FUN_004028ec();     // [2] 0x0028ec
    virtual void FUN_0040106e();     // [3] 0x00106e
    virtual void FUN_00404034();     // [4] 0x004034
    virtual i32 Vfunc14();           // [5] readiness predicate (dispatched)

    i32 m_04;
    CDDrawSubMgrItem* m_item; // +0x10  surface-dimensions item
};

// The +0x20 sound stream is the foreign Dsndmgr SoundStream (<Dsndmgr/SoundStream.h>,
// SoundStream : SoundDevice; scalar-deleting dtor at vtable slot 0, `call [eax]`).
// Two distinct non-virtual __thiscall teardowns, proven by FreeContext's callees:
//   Stop (0x137a80) - pause/reset streaming (the leaf-scan +0x2c inner teardown),
//   Free (0x137740) - full reap+shutdown (the owner's own m_soundStream @+0x20).
// Kept as a minimal local view (not the real header) to protect this TU's 6 exact
// functions from the SoundStream/SoundDevice dependency chain's codegen perturbation.
struct SoundStream {
    virtual void Destroy(u32 flags); // [0] scalar-deleting destructor
    void Stop();                     // 0x137a80  (SoundStream::Stop)
    void Free();                     // 0x137740  (SoundStream::Free)
};

// The +0x28 leaf-scan child (CDDrawSubMgrLeafScan, own vtable 0x5efca0): a polymorphic
// submgr (vptr@+0, scalar-delete at slot 1) with a non-virtual ClearMap, an MFC
// CMapStringToOb keyed cache @+0x10 (0x1c bytes -> ends +0x2c), and a held sound object
// @+0x2c. That held object's concrete type is SoundStream (this TU calls Stop on it);
// the canonical (DDrawSubMgrLeafScan.cpp) views the same field as its base SoundDevice*
// (SoundStream : SoundDevice) - consistent via inheritance, not a real conflict.
struct CDDrawSubMgrLeafScan {
    virtual void Slot00();
    virtual void Destroy(u32 flags); // [1] scalar-deleting destructor
    void ClearMap();
    char m_pad04[0x10 - 0x04]; // +0x04
    CMapStringToOb m_cache;    // +0x10  keyed asset cache (MFC; ends +0x2c)
    SoundStream* m_inner;      // +0x2c  held SoundStream (Stop'd on context teardown)
};

// The +0x24 resolution submgr is a CDDrawSubMgr subtype (CDDrawResolveSubMgr /
// CGameLevel); SetCoords is reached by the base->derived view downcast at its site.
struct CDDrawResolveSubMgr : public CDDrawSubMgr {
    i32 SetCoords(i32 x, i32 y);
};

// The +0x3c callback slot (HP_Callback), the CObject grand base (Wap::CObject,
// vtable @0x5e8cb4), and the full CDDrawSurfaceMgr class now live in the canonical
// <DDrawMgr/DDrawSurfaceMgr.h> (included above). The empty inline ~Wap::CObject
// gives cl the implicit grand-base re-stamp (reloc-masks 0x5e8cb4) folded LAST into
// ~CDDrawSurfaceMgr, and the destructible base subobject supplies the dtor's /GX EH
// frame; the base also fixes the derived vtable layout (dtor at slot 1, IsReady at
// slot 5). The children (CDDrawSubMgr etc.) are forward-declared in the header and
// fully defined above so these method bodies can dereference them.

DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0; // draw-clock mirror
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc; // draw-delta mirror

// The CDDrawPtrCollections at +0x1c is a heap object with a non-virtual __thiscall
// dtor + an explicit RezFree (the other owned children delete through their slot-1
// (slot-0 for the sound stream) scalar-deleting destructor, now real virtuals).
struct CDDrawPtrCollections {
    void Dtor(); // 0x141d50, __thiscall /GX destructor
};
extern "C" void RezFree(void* p); // 0x1b9b82, __cdecl engine operator delete

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::CDDrawSurfaceMgr()
// Stamps the vftable, clears every owned-child pointer except hwnd (+0x30), clears
// flags/bookkeeping at +0x34/+0x38/+0x3c, then resets the two draw-clock globals.
RVA(0x00155840, 0x41)
CDDrawSurfaceMgr::CDDrawSurfaceMgr() {
    m_pages = 0;
    m_childGroup = 0;
    m_workerList = 0;
    m_surfaceDesc = 0;
    m_workerCache = 0;
    m_workerMap = 0;
    m_ptrColl = 0;
    m_soundStream = 0;
    m_resolveSubMgr = 0;
    m_leafScan = 0;
    m_leaf = 0;
    m_flags = 0;
    m_lastError = 0;
    m_callback = 0;
    g_6bf3c0 = 0;
    g_6bf3bc = 0;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::~CDDrawSurfaceMgr() (0x1558b0, __thiscall, /GX)
// Real polymorphic with the CObject base subobject now: cl emits the implicit
// ??_7CDDrawSurfaceMgr own-vptr stamp in the ENTRY state (stamp-first), runs the
// owned-child teardown (Cleanup_155e20), then the empty ~Wap::CObject base
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
// SoundStream at +0x20); the CDDrawPtrCollections at +0x1c is a heap object freed
// by its non-virtual dtor + RezFree. Frameless (the dev's explicit teardown, not
// a compiler member-dtor chain), so no /GX frame here.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): body, the
// child order, the slot-1/slot-0 deletes, the m_ptrColl cache+RezFree, and every store
// are byte-exact; only the callee-saved register holding the constant 0 differs
// (retail pins ebx for the zero / edi for the cached m_ptrColl; cl pins edi=0 / ebx=m_ptrColl),
// a 1-instr phase shift through the =0 stores. No source lever flips the allocation.
// ~96%; defer to the final sweep.
RVA(0x00155e20, 0xd1)
void CDDrawSurfaceMgr::Cleanup_155e20() {
    if (m_resolveSubMgr) {
        m_resolveSubMgr->Destroy(1);
        m_resolveSubMgr = 0;
    }
    if (m_leafScan) {
        m_leafScan->Destroy(1);
        m_leafScan = 0;
    }
    if (m_soundStream) {
        m_soundStream->Destroy(1);
        m_soundStream = 0;
    }
    if (m_pages) {
        m_pages->Destroy(1);
        m_pages = 0;
    }
    if (m_childGroup) {
        m_childGroup->Destroy(1);
        m_childGroup = 0;
    }
    if (m_workerList) {
        m_workerList->Destroy(1);
        m_workerList = 0;
    }
    if (m_surfaceDesc) {
        m_surfaceDesc->Destroy(1);
        m_surfaceDesc = 0;
    }
    if (m_workerCache) {
        m_workerCache->Destroy(1);
        m_workerCache = 0;
    }
    if (m_workerMap) {
        m_workerMap->Destroy(1);
        m_workerMap = 0;
    }
    if (m_leaf) {
        m_leaf->Destroy(1);
        m_leaf = 0;
    }
    CDDrawPtrCollections* ptrColl = m_ptrColl;
    if (ptrColl) {
        ptrColl->Dtor();
        RezFree(ptrColl);
        m_ptrColl = 0;
    }
    m_callback = 0;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::IsReady()
// Returns whether the core child managers are present and the first child accepts
// its +0x14 virtual readiness check.
RVA(0x00155f00, 0x41)
i32 CDDrawSurfaceMgr::IsReady() {
    CDDrawSubMgr* first = m_pages;

    if (first == 0) {
        goto fail;
    }
    if (m_childGroup == 0) {
        goto fail;
    }
    if (m_workerList == 0) {
        goto fail;
    }
    if (m_surfaceDesc == 0) {
        goto fail;
    }
    if (m_workerCache == 0) {
        goto fail;
    }
    if (first->Vfunc14() == 0) {
        goto fail;
    }
    if (m_resolveSubMgr != 0) {
        return 1;
    }

fail:
    return 0;
}

// External context helpers (SoundStream / CDDrawSubMgrLeafScan / CDDrawResolveSubMgr /
// HP_Callback are defined above the class). Both the m_soundStream stream (+0x20) and
// the CDDrawSubMgrLeafScan +0x2c inner are SoundStream objects, but teardown differs:
// the inner is Stop'd (0x137a80), the owner's own stream is Free'd (0x137740).
extern void __cdecl RelayHwnd(void* hWnd);
extern i32 __stdcall CreateChildSurface(i32 x, i32 y, i32 flags);

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::FreeContext()
// Frees context — cleans up the SoundStream (m_soundStream) and the CDDrawSubMgrLeafScan map.
RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_leafScan != 0) {
        SoundStream* inner = m_leafScan->m_inner;
        if (inner != 0) {
            inner->Stop(); // 0x137a80 (leaf-scan +0x2c held stream: pause/reset)
        }
        m_leafScan->ClearMap();
    }
    if (m_soundStream != 0) {
        m_soundStream->Free();
    }
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::SetDimensions()
// Validates/sets surface dimensions.
RVA(0x00155f60, 0x56)
i32 CDDrawSurfaceMgr::SetDimensions(i32 x, i32 y, i32 flags) {
    CDDrawSubMgrItem* child = m_pages->m_item;
    if (child->m_width != x || child->m_height != y) {
        if (CreateChildSurface(x, y, flags) == 0) {
            return 0;
        }
    }
    if (m_resolveSubMgr != 0) {
        if (((CDDrawResolveSubMgr*)m_resolveSubMgr)->SetCoords(x, y) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::SetHwnd()
// Relays the hWnd argument to an external manager function.
RVA(0x00155f50, 0x10)
void CDDrawSurfaceMgr::SetHwnd(void* hWnd) {
    RelayHwnd(hWnd);
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::InvokeCallback()
// Dispatches arguments through the m_callback callback function pointer,
// returning 1 on success / 0 on failure.
RVA(0x00156a90, 0x3a)
i32 CDDrawSurfaceMgr::InvokeCallback(void* arg1, i32 arg2, i32 arg3, i32 arg4) {
    if (!arg1) {
        return 0;
    }
    if (!m_callback) {
        return 0;
    }
    return m_callback(this, arg1, arg2, arg3, arg4) != 0;
}

// Out-of-line stubs so the vftable is emitted in this TU. They are not claimed
// as matched in symbol_names.csv.
i32 CDDrawSurfaceMgr::Init(HWND, i32, i32, i32, i32) {
    return 0;
}
void CDDrawSurfaceMgr::Slot1C() {}
i32 CDDrawSurfaceMgr::Slot2C(i32) {
    return 0;
}
i32 CDDrawSurfaceMgr::Slot30(i32, i32, i32, i32, void*) {
    return 0;
}
i32 CDDrawSurfaceMgr::Slot34(i32, i32, i32, i32, void*) {
    return 0;
}

// Engine-label backlog stubs (moved from src/Stub/DDrawSurfaceMgr.cpp).

// 0x155900 IS the real 5-arg virtual Init(hWnd,w,h,bpp,flags) —
// the SurfaceMgr Init that heap-allocates all 11 owned sub-managers, validates
// each, and configures the display.  Deferred to the final sweep: it is a 1305-B
// /GX method whose FULL nested construction-EH funclet can only be reproduced once
// every child is modeled as a real MFC-derived class (each child's ctor inlines
// CMap*/CList member ctors, and the parent funclet unwinds the in-flight child +
// its half-built map members).  The constituent leaf ctors themselves are still
// @early-stop walls (CDDrawWorkerMapSmall/CDDrawSubMgrLeaf/CDDrawSubMgrLeafScan at
// 94–96%), so this parent cannot exceed them until they land — a leaf-first job.
//
// DECODED STRUCTURE (for the final sweep — retail-verified from 0x155900):
//   m_hWnd = hWnd (arg1);  m_flags = flags (arg5).
//   Then a run of `child = new T(...)` blocks (EH state in [esp+0x1c]/[esp+0x20]),
//   each: op-new(size) -> if non-null: base-ctor 0x156cb0(0,0,this) [surface-desc
//   children instead stamp base vtbl 0x5efc30 + [+4]=[+8]=0 + [+c]=this], inline
//   CMap member ctors(0xa), then stamp the derived vtbl; store into this->m_XX:
//     m_pages = new(0x1c)  vtbl 0x5efe08                                   (CDDrawSubMgrPages)
//     m_childGroup = new(0x6c)  ctor156cb0 + maps@0x10/0x2c/0x48 vtbl 0x5efdc0  (CDDrawChildGroup / CWwdObjMgr view)
//     m_workerList = new(0x2c)  ctor156cb0 + map@0x10          vtbl 0x5efd88    (CDDrawWorkerList)
//     m_surfaceDesc = new(0x2c)  Wap::CObject-base + map@0x10(0x1b7e17) vtbl 0x5efd28 (CDDrawSurfaceDesc submgr)
//     m_workerCache = new(0x2c)  Wap::CObject-base + map@0x10(0x1b7e17) vtbl 0x5efd00 (CDDrawWorkerCache)
//     m_workerMap = new(0x68)  ctor156cb0 + maps@0x10/2c/48(0x1b7e17) vtbl 0x5efcc8 (CDDrawWorkerMapSmall)
//     m_resolveSubMgr = new(0x6d4) ctor 0x15ccd0                                   (CDDrawResolveSubMgr)
//     m_leafScan = new(0x38)  Wap::CObject-base + map@0x10(0x1b8247) vtbl 0x5efca0 (= CDDrawSubMgrLeafScan)
//     m_leaf = new(0x2c)  Wap::CObject-base + map@0x10(0x1b8247) vtbl 0x5efc78 (= CDDrawSubMgrLeaf)
//     m_ptrColl = new(0x948) ctor 0x141cc0                                   (CDDrawPtrCollections)
//     m_soundStream = new(0x9c)  ctor 0x1376d0                                   (SoundStream)
//   Validate phase: for m_childGroup,m_workerList,m_surfaceDesc,m_workerCache,m_workerMap,m_leaf call child->vslot0x18(); on
//   0 (and m_initError==0) set m_initError = 0x3e9..0x3ee and return 0; m_resolveSubMgr->vslot0x34(w,h) ->
//   0x3ef; m_pages->vslot0x24(w,h,flags,arg5) -> 0x3f0.  Then flags&0x20 => m_resolveSubMgr[+8]|=4;
//   SoundStream setup via 0x137720 with mode (bl&0x80?2:1), teardown-on-fail via
//   vslot0/[+0] scalar-delete + 0x3f1; finally m_leafScan->0x157a80(1) validate.  ret 0x14.
// @confidence: high
// @source: tomalla
// @stub
RVA(0x00155900, 0x519)
void CDDrawSurfaceMgr::Init() {}

SIZE_UNKNOWN(CDDrawSubMgrItem);
SIZE_UNKNOWN(Wap::CObject);
SIZE_UNKNOWN(CDDrawPtrCollections);
SIZE_UNKNOWN(CDDrawSubMgrLeafScan);
SIZE_UNKNOWN(CDDrawResolveSubMgr);
SIZE_UNKNOWN(SoundStream);
