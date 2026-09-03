#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawPaletteRegistry.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCueRegistry.h>
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
    m_logicRegistry = NULL;
    m_paletteRegistry = NULL;
    m_deviceManager = NULL;
    m_soundStream = NULL;
    m_level = NULL;
    m_soundRegistry = NULL;
    m_animRegistry = NULL;
    m_flags = 0;
    m_lastError = WORLDERR_NONE;
    m_callback = NULL;
    g_soundCueTimeMs = 0;
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
    m_logicRegistry = new CLogicRecordRegistry(this);
    m_paletteRegistry = new CDDrawPaletteRegistry(this);
    m_level = new CGameLevel(this, 0, 0);
    m_soundRegistry = new SoundCueRegistry(this);
    m_animRegistry = new AnimationRegistry(this);
    m_deviceManager = new CDDrawDeviceManager();
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
    if (!m_logicRegistry->IsReady()) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_WORKER_CACHE;
        }
        return 0;
    }
    if (!m_paletteRegistry->IsReady()) {
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
    if (!m_level->SetViewportSize(w, h)) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_LEVEL_EXTENTS;
        }
        return 0;
    }
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_DIRECT_OBJECT_MOVEMENT)) {
        m_level->m_flags |= WWD_LEVEL_FLAG_DIRECT_MOVEMENT;
    }
    if (!m_drawTarget->CreateChildren(w, h, bpp, flags)) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = WORLDERR_CREATE_PAGES;
        }
        return 0;
    }

    i32 cooperativeLevel = 1;
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_SOUND_PRIORITY)) {
        cooperativeLevel = 2;
    }
    if (!m_soundStream->InitializeDevice(hWnd, cooperativeLevel)) {
        delete m_soundStream;
        m_soundStream = NULL;
        if (HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_REQUIRE_SOUND)) {
            if (m_lastError == WORLDERR_NONE) {
                m_lastError = WORLDERR_SOUND_OUTPUT;
            }
            return 0;
        }
    }
    if (m_soundStream != NULL
        && HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_DISABLE_SOUND)) {
        delete m_soundStream;
        m_soundStream = NULL;
    }
    if (!m_soundRegistry->BindSoundStream(true)) {
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
    if (m_logicRegistry) {
        delete m_logicRegistry;
        m_logicRegistry = NULL;
    }
    if (m_paletteRegistry) {
        delete m_paletteRegistry;
        m_paletteRegistry = NULL;
    }
    if (m_animRegistry) {
        delete m_animRegistry;
        m_animRegistry = NULL;
    }
    if (m_deviceManager) {
        delete m_deviceManager;
        m_deviceManager = NULL;
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
    if (m_logicRegistry == NULL) {
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
    CDDrawFrontSurface* child = m_drawTarget->m_frontSurface;

    if (child->m_width != x || child->m_height != y) {
        if (m_drawTarget->ResizePages(x, y, bpp) == BPP_UNSET) {
            return 0;
        }
        if (m_level != NULL) {

            if (m_level->SetViewportSizeAndUpdatePlanes(x, y) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00155fc0, 0x2e)
void CDDrawSurfaceMgr::FreeContext() {
    if (m_soundRegistry != NULL) {

        SoundStream* inner = m_soundRegistry->m_soundStream;
        if (inner != NULL) {
            inner->StopAllStreams();
        }
        m_soundRegistry->ClearCues();
    }
    if (m_soundStream != NULL) {
        m_soundStream->ShutdownStreams();
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00155ff0, 0x22)
i32 CDDrawSurfaceMgr::EnsureSoundInitialized() {
    if (m_soundStream != NULL && m_soundStream->m_initialized == false) {
        return m_soundStream->InitializeDevice(m_hWnd, 1);
    }
    return 1;
}

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
    header.m_month = now.GetLocalTm(NULL)->tm_mon + 1;
    header.m_day = now.GetLocalTm(NULL)->tm_mday;
    header.m_year = now.GetLocalTm(NULL)->tm_year + 0x76c;
    strcpy(header.m_name, name);
    i32 probe = m_childGroup->CountActive();
    header.m_objIdCounter = g_wwdObjIdCounter;
    header.m_childCount = probe;
    S.Write(&header, sizeof(header));

    if (InvokeCallbackInline(&S, SERIAL_SNAPSHOT_BEGIN, LOGIC_UNSET, NULL) == 0) {
        return 0;
    }
    if (m_childGroup->WriteObjectSnapshots(&S, typeId) == LOGIC_UNSET) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_PRESAVE, LOGIC_UNSET, NULL) == 0) {
        return 0;
    }
    if (m_childGroup->DispatchSerializationToObjects(&S, SERIAL_PRESAVE, typeId) == 0) {
        return 0;
    }
    if (m_level->SerializeDispatch(&S, SERIAL_PRESAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_SAVE, LOGIC_UNSET, NULL) == 0) {
        return 0;
    }
    if (m_childGroup->SerializeObjects(&S, typeId) == LOGIC_UNSET) {
        return 0;
    }
    if (m_level->SerializeDispatch(&S, SERIAL_SAVE, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_POSTSAVE, LOGIC_UNSET, NULL) == 0) {
        return 0;
    }
    if (m_childGroup->DispatchSerializationToObjects(&S, SERIAL_POSTSAVE, typeId) == 0) {
        return 0;
    }
    if (m_level->SerializeDispatch(&S, SERIAL_POSTSAVE, LOGIC_UNSET, 0) == 0) {
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
    if (m_childGroup->LoadObjects(&S, header.m_childCount, typeId) == LOGIC_UNSET) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_PRELOAD, typeId, &header) == 0) {
        return 0;
    }
    if (m_childGroup->DispatchSerializationToObjects(&S, SERIAL_PRELOAD, typeId) == 0) {
        return 0;
    }
    if (m_level->SerializeDispatch(&S, SERIAL_PRELOAD, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_LOAD, typeId, &header) == 0) {
        return 0;
    }
    if (m_childGroup->DeserializeObjects(&S, header.m_childCount, typeId) == LOGIC_UNSET) {
        return 0;
    }
    if (m_level->SerializeDispatch(&S, SERIAL_LOAD, LOGIC_UNSET, 0) == 0) {
        return 0;
    }
    if (InvokeCallbackInline(&S, SERIAL_POSTLOAD, typeId, &header) == 0) {
        return 0;
    }
    if (m_childGroup->DispatchSerializationToObjects(&S, SERIAL_POSTLOAD, typeId) == 0) {
        return 0;
    }
    if (m_level->SerializeDispatch(&S, SERIAL_POSTLOAD, LOGIC_UNSET, 0) == 0) {
        return 0;
    }

    S.Ready();
    m_level->DeactivateDistantObjectsOnMainPlane();
    return 1;
}

RVA(0x00156a90, 0x3a)
i32 CDDrawSurfaceMgr::DispatchSerializationCallback(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    void* payload
) {
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
