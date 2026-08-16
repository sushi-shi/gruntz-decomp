#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdObjMgr.h>

#include <string.h>

RVA(0x00155840, 0x41)
CDDrawSurfaceMgr::CDDrawSurfaceMgr() {
    m_drawTarget = NULL;
    m_childGroup = NULL;
    m_workerList = NULL;
    m_imageRegistry = NULL;
    m_workerCache = NULL;
    m_workerMap = NULL;
    m_ptrColl = NULL;
    m_soundStream = NULL;
    m_level = NULL;
    m_soundRegistry = NULL;
    m_animRegistry = NULL;
    m_flags = 0;
    m_lastError = WORLDERR_NONE;
    m_callback = NULL;
    g_killCueClock = 0;
    g_engineFrameDelta = 0;
}

RVA_COMPGEN(0x00155890, 0x1e, ??_GCDDrawSurfaceMgr@@UAEPAXI@Z)
RVA(0x001558b0, 0x46)
CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {
    Cleanup();
}

RVA(0x00155900, 0x519)
i32 CDDrawSurfaceMgr::Init(HWND hWnd, i32 w, i32 h, ColorDepth bpp, i32 flags) {
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
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_CHILD_GROUP;
        }
        return 0;
    }
    if (!m_workerList->IsReady()) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_WORKER_LIST;
        }
        return 0;
    }
    if (!m_imageRegistry->IsReady()) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_IMAGE_REGISTRY;
        }
        return 0;
    }
    if (!m_workerCache->IsReady()) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_WORKER_CACHE;
        }
        return 0;
    }
    if (!m_workerMap->IsReady()) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_WORKER_MAP;
        }
        return 0;
    }
    if (!m_animRegistry->IsReady()) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_ANIM_REGISTRY;
        }
        return 0;
    }
    if (!m_level->SetCoordExtents(w, h)) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_LEVEL_EXTENTS;
        }
        return 0;
    }
    if (flags & 0x20) {
        m_level->m_flags |= 4;
    }
    if (!m_drawTarget->CreateChildren(w, h, bpp, flags)) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_CREATE_PAGES;
        }
        return 0;
    }

    i32 mode = 1;
    if (flags & 0x80) {
        mode = 2;
    }
    if (!m_soundStream->PlaySoundDefaulted(hWnd, mode)) {
        delete m_soundStream;
        m_soundStream = NULL;
        if (flags & 8) {
            if (m_lastError == WORLDERR_NONE) {
                m_lastError = WORLDERR_SOUND_OUTPUT;
            }
            return 0;
        }
    }
    if (m_soundStream != NULL && (flags & 4)) {
        delete m_soundStream;
        m_soundStream = NULL;
    }
    if (!m_soundRegistry->BindSoundStream(1)) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_SOUND_REGISTRY;
        }
        return 0;
    }
    return 1;
}

RVA(0x00155e20, 0xd1)
void CDDrawSurfaceMgr::Cleanup() {
    if (m_level) {
        delete m_level;
        m_level = NULL;
    }
    if (m_soundRegistry) {
        delete m_soundRegistry;
        m_soundRegistry = NULL;
    }
    if (m_soundStream) {
        delete m_soundStream;
        m_soundStream = NULL;
    }
    if (m_drawTarget) {
        delete m_drawTarget;
        m_drawTarget = NULL;
    }
    if (m_childGroup) {
        delete m_childGroup;
        m_childGroup = NULL;
    }
    if (m_workerList) {
        delete m_workerList;
        m_workerList = NULL;
    }
    if (m_imageRegistry) {
        delete m_imageRegistry;
        m_imageRegistry = NULL;
    }
    if (m_workerCache) {
        delete m_workerCache;
        m_workerCache = NULL;
    }
    if (m_workerMap) {
        delete m_workerMap;
        m_workerMap = NULL;
    }
    if (m_animRegistry) {
        delete m_animRegistry;
        m_animRegistry = NULL;
    }
    if (m_ptrColl) {
        delete m_ptrColl;
        m_ptrColl = NULL;
    }
    m_callback = NULL;
}

RVA(0x00155f00, 0x41)
i32 CDDrawSurfaceMgr::IsReady() {
    CDDrawSubMgrPages* first = m_drawTarget;

    if (first == NULL) {
        goto fail;
    }
    if (m_childGroup == NULL) {
        goto fail;
    }
    if (m_workerList == NULL) {
        goto fail;
    }
    if (m_imageRegistry == NULL) {
        goto fail;
    }
    if (m_workerCache == NULL) {
        goto fail;
    }
    if (first->IsLoaded() == 0) {
        goto fail;
    }
    if (m_level != NULL) {
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
i32 CDDrawSurfaceMgr::SetDimensions(i32 x, i32 y, ColorDepth bpp) {
    CDDrawSurfaceChildA* child = m_drawTarget->m_frontPair;

    if (child->m_width != x || child->m_height != y) {
        if (m_drawTarget->ResizePages(x, y, bpp) == 0) {
            return 0;
        }
        if (m_level != NULL) {

            if (m_level->SetExtentsAndBuildAll(x, y) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_soundRegistry != NULL) {

        SoundStream* inner = m_soundRegistry->m_soundStream;
        if (inner != NULL) {
            inner->Stop();
        }
        m_soundRegistry->ClearMap();
    }
    if (m_soundStream != NULL) {
        m_soundStream->Free();
    }
}

RVA(0x00155ff0, 0x22)
i32 CDDrawSurfaceMgr::PlayDefaultSound() {
    if (m_soundStream != NULL && m_soundStream->m_initialized == 0) {
        return m_soundStream->PlaySoundDefaulted(m_hWnd, 1);
    }
    return 1;
}

// @early-stop
RVA(0x00156020, 0x505)
i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, char* path, char* name, LogicTypeId typeId) {
    if (path == NULL) {
        return 0;
    }
    m_callback = cb;

    CFileMem S;

    if (S.SetName(path, 0, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    CSnapshotHeader header;
    memset(&header, 0, sizeof(header));

    CTime now = CTime::GetCurrentTime();
    header.m_version = 1;
    header.m_month = now.GetLocalTm(0)->tm_mon + 1;
    header.m_day = now.GetLocalTm(0)->tm_mday;
    header.m_year = now.GetLocalTm(0)->tm_year + 0x76c;
    strcpy(header.m_name, name);
    i32 probe = m_childGroup->CountActive();
    header.m_objIdCounter = g_wwdObjIdCounter;
    header.m_childCount = probe;
    S.Write(&header, sizeof(header));

    if (InvokeCallbackInline(&S, SERIAL_SNAPSHOT_BEGIN, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachProbe(&S, typeId) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_PRESAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, SERIAL_PRESAVE, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, SERIAL_PRESAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_SAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachSerialize(&S, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, SERIAL_SAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_POSTSAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, SERIAL_POSTSAVE, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, SERIAL_POSTSAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }

    S.Ready();
    return 1;
}

// @early-stop
RVA(0x00156530, 0x557)
i32 CDDrawSurfaceMgr::RestoreChildren(HP_Callback cb, char* name, LogicTypeId typeId) {
    if (name == NULL) {
        return 0;
    }
    m_callback = cb;

    CFileMem S;

    if (S.SetName(static_cast<const char*>(name), 1, 0) == 0) {
        return 0;
    }
    if (S.Open() == 0) {
        return 0;
    }

    CSnapshotHeader header;
    S.Read(&header, sizeof(header));

    if (InvokeCallbackInline(&S, SERIAL_RESTORE_BEGIN, typeId, &header) == 0) {
        return 0;
    }
    g_wwdObjIdCounter = header.m_objIdCounter;
    m_childGroup->ClearChildren();
    if (m_childGroup->LoadObjects(&S, header.m_childCount, typeId) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_PRELOAD, typeId, &header) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, SERIAL_PRELOAD, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, SERIAL_PRELOAD, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_LOAD, typeId, &header) == 0) {
        return 0;
    }
    if (m_childGroup->Deserialize(&S, header.m_childCount, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, SERIAL_LOAD, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_POSTLOAD, typeId, &header) == 0) {
        return 0;
    }
    if (m_childGroup->ForEachDispatch(&S, SERIAL_POSTLOAD, typeId) == 0) {
        return 0;
    }
    if (m_level->EditDispatch(&S, SERIAL_POSTLOAD, LOGIC_UNSET, 0) == 0) {
        return 0;
    }

    S.Ready();
    m_level->DeactivateDistantObjectsOnMainPlane();
    return 1;
}

RVA(0x00156a90, 0x3a)
i32 CDDrawSurfaceMgr::InvokeCallback(void* ar, SerialMode mode, LogicTypeId typeId, void* payload) {
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
RVA(0x00156ad0, 0x1d2)
i32 __stdcall
LoadRecordFile(const char* name, CSnapshotHeader* hdrOut, void* buf, u32 len, i32 unused) {
    if (name == NULL) {
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
    if (buf != NULL && len > 0) {
        S.Read(buf, len);
    }
    S.Ready();
    return 1;
}

RVA_COMPGEN(0x001578b0, 0x51, ??1CFileMemBase@@UAE@XZ)
RVA_COMPGEN(0x00157960, 0x1e, ??_GCFileMemBase@@UAEPAXI@Z)
RVA_COMPGEN(0x00157980, 0x74, ??1CFileMem@@UAE@XZ)
RVA_COMPGEN(0x00157a20, 0x1e, ??_GCFileMem@@UAEPAXI@Z)
