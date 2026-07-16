#include <rva.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
// DDrawSurfaceMgr.cpp - root object of the tomalla-named DDraw surface/page-manager
// family. CDDrawSurfaceMgr is the owner stored off CGruntzMgr
// +0x30; it holds one child manager pointer per slot and a pair of global draw
// clock mirrors reset by the ctor.
//
// Names are tomalla placeholders. Offsets, store order, vtable slots, and global
// addresses are load-bearing for matching.
//
// VIEW-FREE: every type this TU dereferences is the canonical header class -
// <Gruntz/Loadable.h> (the child base), <Dsndmgr/SoundStream.h>,
// <DDrawMgr/DDrawSubMgrLeafScan.h>, <Gruntz/GameLevel.h> (the +0x24 child),
// <DDrawMgr/DDrawPtrCollections.h>.

// <Mfc.h> FIRST (superset of Win32.h: same <windows.h> for HWND + the MFC classes,
// e.g. CMapStringToOb in the leaf-scan child). The old "pure-Win32, C1189 wall"
// note was wrong - afx.h pulls windows.h the afx-first way, so no C1189.
#include <Mfc.h>
#include <Wap32/Object.h>             // CObject - the shared engine grand-base
#include <DDrawMgr/DDrawSurfaceMgr.h> // THE canonical CDDrawSurfaceMgr class shape
#include <Gruntz/Loadable.h>          // CLoadable - the shared child base (slot-1 scalar-delete)
#include <DDrawMgr/DDrawWorkerRegistry.h> // real +0x10 child type (m_imageRegistry; virtual-dtor delete)
#include <DDrawMgr/DDrawWorkerCache.h> // real +0x14 child type (m_workerCache; virtual-dtor delete)
#include <DDrawMgr/DDrawWorkerMapSmall.h> // real +0x18 child type (m_workerMap; slot-1 scalar-delete)
#include <DDrawMgr/DDrawSubMgrPages.h> // real +0x04 child type (m_drawTarget: IsLoaded, m_frontPair)
#include <DDrawMgr/DDrawChildGroup.h>  // real +0x08 child type (m_childGroup)
#include <DDrawMgr/DDrawChildGroup.h>  // CDDrawChildGroup (Snapshot/RestoreChildren blit-op target)
#include <Gruntz/GameLevel.h>          // CGameLevel (m_level child; EditDispatch/MainPlaneQueryB)
#include <Globals.h>                   // g_wwdObjIdCounter (serialized header id)
#include <string.h>                    // strcpy/memset (inline header build)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // real +0x28 child type (m_2c held stream, ClearMap)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // real +0x2c child type (m_animRegistry; virtual-dtor delete)
#include <DDrawMgr/DDrawSurfacePair.h>    // m_drawTarget->m_frontPair geometry (m_width/m_height)
#include <DDrawMgr/DDrawPtrCollections.h> // real +0x1c pool type (non-virtual dtor 0x141d50)
#include <Dsndmgr/SoundStream.h>          // real +0x20 stream type (Stop 0x137a80 / Free 0x137740)

extern "C" u32 g_killCueClock;     // draw-clock mirror (== donor g_killCueClock, 0x2bf3c0)
extern "C" u32 g_engineFrameDelta; // draw-delta mirror

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::CDDrawSurfaceMgr()
// Stamps the vftable, clears every owned-child pointer except hwnd (+0x30), clears
// flags/bookkeeping at +0x34/+0x38/+0x3c, then resets the two draw-clock globals.
RVA(0x00155840, 0x41)
CDDrawSurfaceMgr::CDDrawSurfaceMgr() {
    m_drawTarget = 0;
    m_childGroup = 0;
    m_workerList = 0;
    m_imageRegistry = 0;
    m_workerCache = 0;
    m_workerMap = 0;
    m_ptrColl = 0;
    m_soundStream = 0;
    m_level = 0;
    m_soundRegistry = 0;
    m_animRegistry = 0;
    m_flags = 0;
    m_lastError = 0;
    m_callback = 0;
    g_killCueClock = 0;
    g_engineFrameDelta = 0;
}

// The class's own vtable datum: cl emits ??_7CDDrawSurfaceMgr@@6B@ (8 slots, real
// polymorphism) and the ctor/dtor stamp it; VTBL names it at the retail RVA. This
// REPLACES the CVtEmit_1efc58 tracking shim (FinalVtables.cpp) - proven CDDrawSurfaceMgr's
// own vtable by the per-slot audit (slots 5/6/7 = IsReady/Init/Cleanup_155e20).
VTBL(CDDrawSurfaceMgr, 0x001efc58);

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::~CDDrawSurfaceMgr() (0x1558b0, __thiscall, /GX)
// Real polymorphic with the CObject base subobject now: cl emits the implicit
// ??_7CDDrawSurfaceMgr own-vptr stamp in the ENTRY state (stamp-first), runs the
// owned-child teardown (Cleanup_155e20), then the empty ~CObject base
// folds the implicit grand-base re-stamp last. The destructible base subobject
// supplies the /GX EH frame. (eh-dtor-implicit-vptr-stamp-first.md /
// eh-dtor-needs-base-subobject.md.)
RVA(0x001558b0, 0x46)
CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {
    Cleanup_155e20();
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
//     m_drawTarget = new(0x1c)  vtbl 0x5efe08                                   (CDDrawSubMgrPages)
//     m_childGroup = new(0x6c)  ctor156cb0 + maps@0x10/0x2c/0x48 vtbl 0x5efdc0  (CDDrawChildGroup / CDDrawChildGroup view)
//     m_workerList = new(0x2c)  ctor156cb0 + map@0x10          vtbl 0x5efd88    (CDDrawWorkerList)
//     m_imageRegistry = new(0x2c)  CObject-base + map@0x10(0x1b7e17) vtbl 0x5efd28 (CDDrawSurfaceDesc submgr)
//     m_workerCache = new(0x2c)  CObject-base + map@0x10(0x1b7e17) vtbl 0x5efd00 (CDDrawWorkerCache)
//     m_workerMap = new(0x68)  ctor156cb0 + maps@0x10/2c/48(0x1b7e17) vtbl 0x5efcc8 (CDDrawWorkerMapSmall)
//     m_level = new(0x6d4) ctor 0x15ccd0                                   (CDDrawResolveSubMgr)
//     m_soundRegistry = new(0x38)  CObject-base + map@0x10(0x1b8247) vtbl 0x5efca0 (= CDDrawSubMgrLeafScan)
//     m_animRegistry = new(0x2c)  CObject-base + map@0x10(0x1b8247) vtbl 0x5efc78 (= CDDrawSubMgrLeaf)
//     m_ptrColl = new(0x948) ctor 0x141cc0                                   (CDDrawPtrCollections)
//     m_soundStream = new(0x9c)  ctor 0x1376d0                                   (SoundStream)
//   Validate phase: for m_childGroup,m_workerList,m_imageRegistry,m_workerCache,m_workerMap,m_animRegistry call child->vslot0x18(); on
//   0 (and m_initError==0) set m_initError = 0x3e9..0x3ee and return 0; m_level->vslot0x34(w,h) ->
//   0x3ef; m_drawTarget->vslot0x24(w,h,flags,arg5) -> 0x3f0.  Then flags&0x20 => m_level[+8]|=4;
//   SoundStream setup via 0x137720 with mode (bl&0x80?2:1), teardown-on-fail via
//   vslot0/[+0] scalar-delete + 0x3f1; finally m_soundRegistry->0x157a80(1) validate.  ret 0x14.
// @confidence: high
// @source: tomalla
// @stub
RVA(0x00155900, 0x519)
i32 CDDrawSurfaceMgr::Init(void* /*hWnd*/, i32 /*w*/, i32 /*h*/, i32 /*bpp*/, i32 /*flags*/) {
    return 0;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::Cleanup_155e20() (0x155e20, __thiscall)
// Tear down every owned child in the engine's fixed teardown order:
// `if (child) { delete child; child = 0; }` per child - the single test/je per
// block is the source `if` (cl elides delete's redundant null-guard on the
// flow-known-non-null pointer) and the zero-store is GUARDED (the je lands on
// the NEXT child's load). The slot-1 (slot-0 for the SoundStream) call is the
// virtual scalar-deleting dtor; the cached-in-edi +0x1c block is delete on the
// non-virtual-dtor CDDrawPtrCollections (call ??1 + push/call operator delete).
// Frameless (the dev's explicit teardown, not a compiler member-dtor chain).
// The operator-delete reloc (base ??3@YAXPAX@Z vs target `_RezFree`) is a
// naming artifact: 0x1b9b82 IS the CRT operator delete (every compiler-emitted
// ??_G calls it; see the view-burndown report).
RVA(0x00155e20, 0xd1)
void CDDrawSurfaceMgr::Cleanup_155e20() {
    if (m_level) {
        delete m_level;
        m_level = 0;
    }
    if (m_soundRegistry) {
        delete m_soundRegistry;
        m_soundRegistry = 0;
    }
    if (m_soundStream) {
        delete m_soundStream;
        m_soundStream = 0;
    }
    if (m_drawTarget) {
        delete m_drawTarget;
        m_drawTarget = 0;
    }
    if (m_childGroup) {
        delete m_childGroup;
        m_childGroup = 0;
    }
    if (m_workerList) {
        delete m_workerList;
        m_workerList = 0;
    }
    if (m_imageRegistry) {
        delete m_imageRegistry;
        m_imageRegistry = 0;
    }
    if (m_workerCache) {
        delete m_workerCache;
        m_workerCache = 0;
    }
    if (m_workerMap) {
        delete m_workerMap;
        m_workerMap = 0;
    }
    if (m_animRegistry) {
        delete m_animRegistry;
        m_animRegistry = 0;
    }
    if (m_ptrColl) {
        delete m_ptrColl;
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
    CDDrawSubMgrPages* first = m_drawTarget;

    if (first == 0) {
        goto fail;
    }
    if (m_childGroup == 0) {
        goto fail;
    }
    if (m_workerList == 0) {
        goto fail;
    }
    if (m_imageRegistry == 0) {
        goto fail;
    }
    if (m_workerCache == 0) {
        goto fail;
    }
    if (first->IsLoaded() == 0) {
        goto fail;
    }
    if (m_level != 0) {
        return 1;
    }

fail:
    return 0;
}

// 0x1437e0 (directdrawmgr): real signature takes a callback fn-ptr (?RelayHwnd@@YAXP6AHXZ@Z);
// SetHwnd forwards its opaque handle arg through it (reloc-masked, arg is a bare push).
extern void __cdecl RelayHwnd(i32(__cdecl* callback)());
extern i32 __stdcall CreateChildSurface(i32 x, i32 y, i32 flags);

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::SetHwnd()
// Relays the hWnd argument to an external manager function.
RVA(0x00155f50, 0x10)
void CDDrawSurfaceMgr::SetHwnd(void* hWnd) {
    RelayHwnd((i32(__cdecl*)())hWnd);
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::SetDimensions()
// Validates/sets surface dimensions.
RVA(0x00155f60, 0x56)
i32 CDDrawSurfaceMgr::SetDimensions(i32 x, i32 y, i32 flags) {
    CDDrawSurfacePair* child = m_drawTarget->m_frontPair;
    if (child->m_width != x || child->m_height != y) {
        if (CreateChildSurface(x, y, flags) == 0) {
            return 0;
        }
    }
    if (m_level != 0) {
        // FLAG(cross-cast): m_level is the CGameLevel child (new(0x6d4),
        // ctor 0x15ccd0), yet retail dispatches 0x155f60 - this class's own
        // SetDimensions body - on it. Either CGameLevel exposes a same-layout
        // SetCoords or the +0x24 head mirrors the owner's; unresolved, the cast
        // preserves retail's call target. @identity-TODO.
        if (((CDDrawSurfaceMgr*)m_level)->SetDimensions(x, y, 0) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::FreeContext()
// Frees context - Stop (0x137a80, pause/reset) the leaf-scan child's held stream +
// clear its keyed map, then Free (0x137740, full reap+shutdown) the owner's own
// m_soundStream. Both +0x20 and leafScan+0x2c hold SoundStream objects.
RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_soundRegistry != 0) {
        // FLAG(retype-deferred): the canonical types leafScan m_2c as its base
        // SoundDevice*; this site proves the held object is a SoundStream (the
        // non-virtual call 0x137a80 = SoundStream::Stop). The m_2c retype is
        // deferred (DDrawSubMgrLeafScan.h is additive-only this session).
        SoundStream* inner = (SoundStream*)m_soundRegistry->m_2c;
        if (inner != 0) {
            inner->Stop(); // 0x137a80 (leaf-scan +0x2c held stream: pause/reset)
        }
        m_soundRegistry->ClearMap();
    }
    if (m_soundStream != 0) {
        m_soundStream->Free();
    }
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::PlayDefaultSound()  (0x155ff0)
// Lazily (re)start the owner's sound stream: if it exists and its DirectSound device
// is not yet initialised (SoundDevice::m_initialized @+0x78 == 0), play the defaulted
// sound bound to the manager's window (PlaySoundDefaulted, 0x137720 __thiscall) and
// return its result; otherwise report OK (1). Sibling of Init/FreeContext (all three
// dispatch the +0x20 SoundStream). Re-homed from src/Stub/GapFunctions.cpp (matcher-2)
// - attributed via PlaySoundDefaulted's caller set (only Init + this).
RVA(0x00155ff0, 0x22)
i32 CDDrawSurfaceMgr::PlayDefaultSound() {
    if (m_soundStream != 0 && m_soundStream->m_initialized == 0) {
        return m_soundStream->PlaySoundDefaulted(m_hWnd, 1);
    }
    return 1;
}
// ===========================================================================
// CDDrawSurfaceMgr::SnapshotChildren (0x156020) / RestoreChildren (0x156530) - the /GX
// child blit-param serializer pair, the split-out /GX tail of THIS obj. The canonical
// class already declares both (DDrawSurfaceMgr.h); member access uses m_childGroup (cast
// CDDrawChildGroup* for the blit-op calls) / m_level (cast CGameLevel*) / m_callback (the
// canonical HP_Callback). Engine callees reloc-masked.
// ===========================================================================
// The stack serializer is the ONE canonical <Io/FileMem.h> CFileMem (0x28 B; a CFileMemBase
// stream + a CFileIO m_file @+0x10). DISASM PROOF (retail 0x156020 prologue): `CFileMem S;`
// lowers EXACTLY to `call 0x157850` (CFileMemBase ctor) + `call 0x1befd7` (CFile m_file ctor)
// + `mov [S],0x5efe30` (the derived-vtable stamp),
// then the methods are DIRECT devirtualized calls on the known-type local (Reset 0x157a50 /
// SetName 0x165e30 / Open 0x165e60 / Write 0x165f50 / Read 0x165f00 / Ready 0x165ef0), and the
// object auto-destructs (~CFileMem 0x157980, inlined at each /GX unwind) at every return - so
// NO cast, NO explicit m_file Init, NO explicit dtor call. ForEachSerialize takes it as a
// CSerialArchive* (== CFileMemBase, the implicit upcast). (Was a local Serializer/SnapStream
// view that hand-modelled this as an opaque blob and cast `this` to CFileMem at each call.)
typedef CFileMemBase CSerialArchive;

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::SnapshotChildren (0x156020, __thiscall, /GX)
RVA(0x00156020, 0x505)
i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, i32 arg1, char* name, i32 arg3) {
    if (cb == 0) {
        return 0;
    }
    m_callback = cb;

    CFileMem S;
    S.Reset();

    if (S.SetName((const char*)(void*)cb, 0, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    // Build the 0x120-byte header record (CTime stamp + the name strcpy).
    CTime now;
    char header[0x120];
    memset(header, 0, sizeof(header));
    *(i32*)(header + 0x04) = 1;
    *(i32*)(header + 0x08) = now.GetLocalTm(0)->tm_mon + 1;
    *(i32*)(header + 0x0c) = now.GetLocalTm(0)->tm_mday;
    *(i32*)(header + 0x0c) = now.GetLocalTm(0)->tm_year + 0x76c;
    strcpy(header + 0x10, name);
    i32 probe = m_childGroup->CountActive_15abc0();
    *(u32*)(header + 0x114) = g_wwdObjIdCounter;
    *(i32*)(header + 0x118) = probe;
    S.Write((const void*)header, 0x120);

    // ---- dispatch the five blit modes over the children ----
    if (m_callback && cb(this, &S, 1, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachProbe_15acb0((i32)&S, arg3) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch_15ac20((i32)&S, 3, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch((void*)&S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 4, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachSerialize_15b020(&S, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch((void*)&S, 4, 0, 0) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 5, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch_15ac20((i32)&S, 5, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch((void*)&S, 5, 0, 0) == 0) {
        return 0;
    }

    S.Ready();
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::RestoreChildren (0x156530, __thiscall, /GX) - the load
// counterpart of SnapshotChildren. Opens the same CFileMem-backed serializer over
// `name`, reads back the 0x120-byte header (publishing header[0x114] -> g_wwdObjIdCounter),
// then replays the run-callback (m_callback, REQUIRED here - a null m_callback rejects) and the
// child load-ops over the m_08 (CDDrawChildGroup) + m_24 (CGameLevel) children for
// modes 2/6/7/8. Success closes via End()/MainPlaneQueryB()/Close(). Field/method
// names are placeholders; OFFSETS, vtable slots, sizes, store order and the ordered
// call sequence are load-bearing. Engine callees are reloc-masked external.
//
// @early-stop
// big-SEH wall (same as SnapshotChildren above; docs/patterns/big-seh-fuzzy-desync.md
// + gx-state-machine-scalar-delete-cleanup.md + eh-state-numbering-base.md): a 1367-B
// /GX function with a multi-way fall-through reject ladder over the CFileMem serializer
// temp. The whole carcass (every offset, the embedded-stream Init, the 0x120 header
// Read, the g_wwdObjIdCounter publish, the ordered child load-op call sequence, the inline-vs-
// out-of-line ~Serializer split) is reproduced, but at each reject retail destroys the
// temp via the re-stamped scalar-deleting vtable (mov [esp+0xc],0x5efe30; call
// ds:0x5efe3c) under an even/odd __ehfuncinfo state pair before a shared ~T tail, while
// idiomatic scope-exit C++ emits the simple dtor per return -> the long fail ladder
// desyncs and the trylevel state numbers diverge. Not source-steerable; deferred to the
// final sweep once the serializer + child classes are fully modeled (leaf-first redo).
RVA(0x00156530, 0x557)
i32 CDDrawSurfaceMgr::RestoreChildren(HP_Callback cb, char* name, i32 arg3) {
    if (name == 0) {
        return 0;
    }
    m_callback = cb;

    CFileMem S;
    S.Reset();

    if (S.SetName((const char*)name, 1, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    char header[0x120];
    S.Read(header, 0x120);

    if (m_callback == 0 || m_callback(this, &S, 2, arg3, (i32)header) == 0) {
        return 0;
    }
    g_wwdObjIdCounter = *(u32*)(header + 0x114);
    m_childGroup->DestroyChildren_159ef0();
    if (m_childGroup->LoadObjects(&S, *(unsigned int*)(header + 0x110), arg3) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 6, arg3, (i32)header) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch_15ac20((i32)&S, 6, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch((void*)&S, 6, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 7, arg3, (i32)header) == 0) {
        return 0;
    }
    if (m_childGroup->Deserialize_15b0e0(&S, *(unsigned int*)(header + 0x110), arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch((void*)&S, 7, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 8, arg3, (i32)header) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch_15ac20((i32)&S, 8, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch((void*)&S, 8, 0, 0) == 0) {
        return 0;
    }

    S.Ready();
    m_level->MainPlaneQueryB();
    return 1;
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

// @identity-TODO (matcher-5): 0x156ad0 (466 B, free __stdcall 5 args, /GX) == a CFileMem
// "load file into buffer" helper (RVA-adjacent to CFileMemBase @0x157850; belongs to
// src/Io/FileMem.cpp once that TU carries an explicit inline CFileMem ctor). Homed here
// from GapFunctions.cpp by RVA neighbourhood (immediately after this TU's 0x156a90 block).
// DECODED: if(arg1==0) return 0; construct a local CFileMem (base+derived ctors inlined,
// Reset()); CFileMemBase::SetName; CFileMem::Open; CFileMem::Read(header 0x120); if(readLen
// && size) Read(buf2, size); Ready(); dtor; return 1. Byte-match BLOCKED on the inlined
// CFileMem ctor (retail re-inits all fields via a Reset()-body ctor).
RVA(0x00156ad0, 0x1d2)
i32 Gap_156ad0(void) {
    return 0;
}
