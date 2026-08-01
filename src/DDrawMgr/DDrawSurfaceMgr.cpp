#include <rva.h>
#include <Rez/FrameClock.h>
#include <Io/FileMem.h>

#include <Mfc.h>
#include <Wap32/Object.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/Loadable.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GameLevel.h>
#include <string.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <Dsndmgr/SoundStream.h>
#include <Wwd/WwdObjMgr.h>

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

RVA(0x00155900, 0x519)
i32 CDDrawSurfaceMgr::Init(HWND hWnd, i32 w, i32 h, i32 bpp, i32 flags) {
    m_hWnd = hWnd;
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
void CDDrawSurfaceMgr::SetRestoreHandler(SurfaceRestoreFn handler) {
    SetSurfaceRestoreHandler(handler);
}

RVA(0x00155f60, 0x56)
i32 CDDrawSurfaceMgr::SetDimensions(i32 x, i32 y, i32 flags) {
    CDDrawSurfaceChildA* child = m_drawTarget->m_frontPair;

    if (child->m_width != x || child->m_height != y) {
        if (m_drawTarget->ResizePages(x, y, flags) == 0) {
            return 0;
        }
        if (m_level != 0) {

            if (m_level->SetExtentsAndBuildAll(x, y) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_soundRegistry != 0) {

        SoundStream* inner = m_soundRegistry->m_soundStream;
        if (inner != 0) {
            inner->Stop();
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

// @early-stop
RVA(0x00156020, 0x505)

i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, char* path, char* name, i32 typeId) {
    if (path == 0) {
        return 0;
    }
    m_callback = cb;

    CFileMem S;
    S.Reset();

    if (S.SetName(path, 0, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

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

    if (m_callback && cb(this, &S, 1, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachProbe(&S, typeId) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 3, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 4, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachSerialize(&S, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, 4, 0, 0) == 0) {
        return 0;
    }
    if (m_callback && cb(this, &S, 5, 0, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 5, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, 5, 0, 0) == 0) {
        return 0;
    }

    S.Ready();
    return 1;
}

RVA(0x00156530, 0x557)
i32 CDDrawSurfaceMgr::RestoreChildren(HP_Callback cb, char* name, i32 typeId) {
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

    void* headerArg = &header;

    if (m_callback == 0 || m_callback(this, &S, 2, typeId, headerArg) == 0) {
        return 0;
    }
    g_wwdObjIdCounter = header.m_objIdCounter;
    m_childGroup->ClearChildren();
    if (m_childGroup->LoadObjects(&S, header.m_childCount, typeId) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 6, typeId, headerArg) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 6, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, 6, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 7, typeId, headerArg) == 0) {
        return 0;
    }
    if (m_childGroup->Deserialize(&S, header.m_childCount, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, 7, 0, 0) == 0) {
        return 0;
    }
    if (m_callback == 0 || m_callback(this, &S, 8, typeId, headerArg) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, 8, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, 8, 0, 0) == 0) {
        return 0;
    }

    S.Ready();
    m_level->DeactivateDistantObjectsOnMainPlane();
    return 1;
}

RVA(0x00156a90, 0x3a)

i32 CDDrawSurfaceMgr::InvokeCallback(void* ar, i32 mode, i32 typeId, void* payload) {
    if (!ar) {
        return 0;
    }
    if (!m_callback) {
        return 0;
    }
    return m_callback(this, ar, mode, typeId, payload) != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @early-stop
RVA(0x00156ad0, 0x1d2)
i32 __stdcall
LoadRecordFile(const char* name, CSnapshotHeader* hdrOut, void* buf, u32 len, i32 unused) {
    if (name == 0) {
        return 0;
    }
    CFileMem S;
    if (S.SetName(name, 1, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    S.Read(&hdrOut, sizeof(CSnapshotHeader));
    if (buf != 0 && len > 0) {
        S.Read(buf, len);
    }
    S.Ready();
    return 1;
}

RVA_COMPGEN(0x00157980, 0x74, ??1CFileMem@@UAE@XZ)
RVA_COMPGEN(0x00157a20, 0x1e, ??_GCFileMem@@UAEPAXI@Z)
