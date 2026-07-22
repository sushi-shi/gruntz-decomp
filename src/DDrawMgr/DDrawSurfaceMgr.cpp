#include <rva.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)

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
#include <string.h>                    // strcpy/memset (inline header build)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // real +0x28 child type (m_2c held stream, ClearMap)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // real +0x2c child type (m_animRegistry; virtual-dtor delete)
#include <DDrawMgr/DDrawSurfacePair.h>    // m_drawTarget->m_frontPair geometry (m_width/m_height)
#include <DDrawMgr/DDrawPtrCollections.h> // real +0x1c pool type (non-virtual dtor 0x141d50)
#include <Dsndmgr/SoundStream.h>          // real +0x20 stream type (Stop 0x137a80 / Free 0x137740)
#include <Wwd/WwdObjMgr.h> // ex Globals.h


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

VTBL(CDDrawSurfaceMgr, 0x001efc58);

RVA(0x001558b0, 0x46)
CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {
    Cleanup();
}

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

RVA(0x00155e20, 0xd1)
void CDDrawSurfaceMgr::Cleanup() {
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


RVA(0x00155f50, 0x10)
void CDDrawSurfaceMgr::SetHwnd(void* hWnd) {
    RelayHwnd(static_cast<i32(__cdecl*)()>(hWnd));
}

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
        if ((reinterpret_cast<CDDrawSurfaceMgr*>(m_level))->SetDimensions(x, y, 0) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_soundRegistry != 0) {
        // FLAG(retype-deferred): the canonical types leafScan m_2c as its base
        // SoundDevice*; this site proves the held object is a SoundStream (the
        // non-virtual call 0x137a80 = SoundStream::Stop). The m_2c retype is
        // deferred (DDrawSubMgrLeafScan.h is additive-only this session).
        SoundStream* inner = static_cast<SoundStream*>(m_soundRegistry->m_2c);
        if (inner != 0) {
            inner->Stop(); // 0x137a80 (leaf-scan +0x2c held stream: pause/reset)
        }
        m_soundRegistry->ClearMap();
    }
    if (m_soundStream != 0) {
        m_soundStream->Free();
    }
}

RVA(0x00155ff0, 0x22)
i32 CDDrawSurfaceMgr::PlayDefaultSound() {
    if (m_soundStream != 0 && m_soundStream->m_initialized == 0) {
        return m_soundStream->PlaySoundDefaulted(m_hWnd, 1);
    }
    return 1;
}
typedef CFileMemBase CSerialArchive;

RVA(0x00156020, 0x505)
i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, i32 arg1, char* name, i32 arg3) {
    if (cb == 0) {
        return 0;
    }
    m_callback = cb;

    CFileMem S;
    S.Reset();

    if (S.SetName(static_cast<const char*>(static_cast<void*>(cb)), 0, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    // Build the 0x120-byte header record (CTime stamp + the name strcpy).
    CTime now;
    char header[0x120];
    memset(header, 0, sizeof(header));
    *reinterpret_cast<i32*>((header + 0x04)) = 1;
    *reinterpret_cast<i32*>((header + 0x08)) = now.GetLocalTm(0)->tm_mon + 1;
    *reinterpret_cast<i32*>((header + 0x0c)) = now.GetLocalTm(0)->tm_mday;
    *reinterpret_cast<i32*>((header + 0x0c)) = now.GetLocalTm(0)->tm_year + 0x76c;
    strcpy(header + 0x10, name);
    i32 probe = m_childGroup->CountActive();
    *reinterpret_cast<u32*>((header + 0x114)) = g_wwdObjIdCounter;
    *reinterpret_cast<i32*>((header + 0x118)) = probe;
    S.Write(static_cast<const void*>(header), 0x120);

    // ---- dispatch the five blit modes over the children ----
    if (m_callback && cb(this, &S, 1, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachProbe(reinterpret_cast<i32>(&S), arg3) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(reinterpret_cast<i32>(&S), 3, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 3, 0, 0) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 4, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachSerialize(&S, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 4, 0, 0) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 5, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(reinterpret_cast<i32>(&S), 5, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 5, 0, 0) == 0) {
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

    if (S.SetName(static_cast<const char*>(name), 1, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    char header[0x120];
    S.Read(header, 0x120);

    if (m_callback == 0 || m_callback(this, &S, 2, arg3, reinterpret_cast<i32>(header)) == 0) {
        return 0;
    }
    g_wwdObjIdCounter = *reinterpret_cast<u32*>((header + 0x114));
    m_childGroup->DestroyChildren_159ef0();
    if (m_childGroup->LoadObjects(&S, *reinterpret_cast<unsigned int*>((header + 0x110)), arg3) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 6, arg3, reinterpret_cast<i32>(header)) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(reinterpret_cast<i32>(&S), 6, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 6, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 7, arg3, reinterpret_cast<i32>(header)) == 0) {
        return 0;
    }
    if (m_childGroup->Deserialize(&S, *reinterpret_cast<unsigned int*>((header + 0x110)), arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 7, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 8, arg3, reinterpret_cast<i32>(header)) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(reinterpret_cast<i32>(&S), 8, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 8, 0, 0) == 0) {
        return 0;
    }

    S.Ready();
    m_level->MainPlaneQueryB();
    return 1;
}

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
i32 LoadRecordFile(void) {
    return 0;
}
