#include <rva.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)

#include <Mfc.h>
#include <Wap32/Object.h>             // CObject - the shared engine grand-base
#include <DDrawMgr/DDrawSurfaceMgr.h> // THE canonical CDDrawSurfaceMgr class shape
#include <Gruntz/Loadable.h>          // CLoadable - the shared child base (slot-1 scalar-delete)
#include <DDrawMgr/DDrawWorkerRegistry.h> // real +0x10 child type (m_imageRegistry; virtual-dtor delete)
#include <DDrawMgr/DDrawWorkerCache.h> // real +0x14 child type (m_workerCache; virtual-dtor delete)
#include <DDrawMgr/DDrawWorkerList.h>  // real +0x0c child type (m_workerList; Init's `new`)
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
#include <Wwd/WwdObjMgr.h>                // ex Globals.h

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

RVA_COMPGEN(0x00155890, 0x1e, ??_GCDDrawSurfaceMgr@@UAEPAXI@Z)
RVA(0x001558b0, 0x46)
CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {
    Cleanup();
}

// 0x155900 IS the real 5-arg virtual Init(hWnd,w,h,bpp,flags) - the SurfaceMgr
// display bring-up: heap-allocate all eleven owned sub-managers, validate each
// (m_lastError 0x3e9..0x3f2) and configure the display + sound stream.
//
// The eleven `new T(this)` blocks are what proves the child classes' constructors:
// eight of them are expanded INLINE here (base ctor, the default-block-size CMap/
// CObList member ctors, the derived ??_7 stamp, the trailing field zeroes), so those
// eight ctors are header-inline - they are now declared as such on each class. Three
// are out-of-line calls (CGameLevel 0x15ccd0, CDDrawPtrCollections 0x141cc0,
// SoundStream 0x1376d0). Under /GX each `new` runs under its own __ehfuncinfo state
// ([esp+0x1c]) with the in-flight pointer homed at [esp+0x10], so the whole
// allocate-then-construct ladder is one EH state machine; that falls out of the
// `new` expressions, it is not hand-written.
RVA(0x00155900, 0x519)
i32 CDDrawSurfaceMgr::Init(void* hWnd, i32 w, i32 h, i32 bpp, i32 flags) {
    m_hWnd = static_cast<HWND>(hWnd);
    m_flags = flags;

    m_drawTarget = new CDDrawSubMgrPages(this);
    m_childGroup = new CDDrawChildGroup(this);
    m_workerList = new CDDrawWorkerList(this);
    m_imageRegistry = new CDDrawWorkerRegistry(this);
    m_workerCache = new CDDrawWorkerCache(this);
    m_workerMap = new CDDrawWorkerMapSmall(this);
    m_level = new CGameLevel(this, 0, 0);
    m_soundRegistry = new CDDrawSubMgrLeafScan(this);
    m_animRegistry = new CDDrawSubMgrLeaf(this);
    m_ptrColl = new CDDrawPtrCollections();
    m_soundStream = new SoundStream();

    if (!m_childGroup->IsReady()) {
        if (m_lastError == 0) {
            m_lastError = 0x3e9;
        }
        return 0;
    }
    if (!m_workerList->IsReady()) {
        if (m_lastError == 0) {
            m_lastError = 0x3ea;
        }
        return 0;
    }
    if (!m_imageRegistry->IsReady()) {
        if (m_lastError == 0) {
            m_lastError = 0x3eb;
        }
        return 0;
    }
    if (!m_workerCache->IsReady()) {
        if (m_lastError == 0) {
            m_lastError = 0x3ec;
        }
        return 0;
    }
    if (!m_workerMap->IsReady()) {
        if (m_lastError == 0) {
            m_lastError = 0x3ed;
        }
        return 0;
    }
    if (!m_animRegistry->IsReady()) {
        if (m_lastError == 0) {
            m_lastError = 0x3ee;
        }
        return 0;
    }
    if (!m_level->SetCoordExtents(w, h)) {
        if (m_lastError == 0) {
            m_lastError = 0x3ef;
        }
        return 0;
    }
    if (flags & 0x20) {
        m_level->m_flags |= 4;
    }
    if (!m_drawTarget->CreateChildren(w, h, bpp, flags)) {
        if (m_lastError == 0) {
            m_lastError = 0x3f0;
        }
        return 0;
    }

    i32 mode = 1;
    if (flags & 0x80) {
        mode = 2;
    }
    if (!m_soundStream->PlaySoundDefaulted(hWnd, mode)) {
        delete m_soundStream;
        m_soundStream = 0;
        if (flags & 8) {
            if (m_lastError == 0) {
                m_lastError = 0x3f1;
            }
            return 0;
        }
    }
    if (m_soundStream != 0 && (flags & 4)) {
        delete m_soundStream;
        m_soundStream = 0;
    }
    if (!m_soundRegistry->BindSoundStream(1)) {
        if (m_lastError == 0) {
            m_lastError = 0x3f2;
        }
        return 0;
    }
    return 1;
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
    CDDrawSurfaceChildA* child = m_drawTarget->m_frontPair;
    if (child->m_width != x || child->m_height != y) {
        if (m_drawTarget->ResizePages(x, y, flags) == 0) {
            return 0;
        }
    }
    if (m_level != 0) {
        // Retail rel32 is 0x15d700 = CGameLevel::SetExtentsAndBuildAll(x, y) - the
        // ex cross-cast "recursive SetDimensions" placeholder mis-read the target.
        if (m_level->SetExtentsAndBuildAll(x, y) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_soundRegistry != 0) {
        // m_2c IS a SoundStream* on the canonical class (the cast that used to sit
        // here is retired): CDDrawSubMgrLeafScan::BindSoundStream @0x157a80 assigns
        // it straight from this manager's m_soundStream, and this site's
        // non-virtual call 0x137a80 is SoundStream::Stop.
        SoundStream* inner = m_soundRegistry->m_2c;
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

RVA(0x00156020, 0x505)
i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, char* arg1, char* name, i32 arg3) {
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
    CSnapshotHeader header;
    memset(&header, 0, sizeof(header));
    header.m_version = 1;
    header.m_month = now.GetLocalTm(0)->tm_mon + 1;
    header.m_dayThenYear = now.GetLocalTm(0)->tm_mday;
    header.m_dayThenYear = now.GetLocalTm(0)->tm_year + 0x76c;
    strcpy(header.m_name, name);
    i32 probe = m_childGroup->CountActive();
    header.m_objIdCounter = g_wwdObjIdCounter;
    header.m_activeCount = probe;
    S.Write(&header, sizeof(header));

    // ---- dispatch the five blit modes over the children ----
    if (m_callback && cb(this, &S, 1, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachProbe(&S, arg3) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 3, arg3) == 0) {
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
    if (m_childGroup->ForEachDispatch(&S, 5, arg3) == 0) {
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

    CSnapshotHeader header;
    S.Read(&header, sizeof(header));

    // API-forced, at one seam: m_callback is a client-registered hook whose last
    // parameter is an opaque payload word, so the out-pointer has to be widened into
    // it - widened once here instead of at each of the four dispatches.
    i32 headerArg = reinterpret_cast<i32>(&header);

    if (m_callback == 0 || m_callback(this, &S, 2, arg3, headerArg) == 0) {
        return 0;
    }
    g_wwdObjIdCounter = header.m_objIdCounter;
    m_childGroup->DestroyChildren_159ef0();
    if (m_childGroup->LoadObjects(&S, header.m_childCount, arg3) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 6, arg3, headerArg) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 6, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 6, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 7, arg3, headerArg) == 0) {
        return 0;
    }
    if (m_childGroup->Deserialize(&S, header.m_childCount, arg3) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(static_cast<void*>(&S), 7, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 8, arg3, headerArg) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 8, arg3) == 0) {
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
