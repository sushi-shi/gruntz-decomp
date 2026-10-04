#include <StdAfx.h>
#include <Io/StreamArchive.h>
#include <Io/FileTransaction.h>

#include <Ints.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>

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
#include <Gruntz/LevelCollisionInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <SafeDelete.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdObjMgr.h>

#include <dsound.h>
#include <string.h>

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
    SetSerializationCallback(NULL);
    g_soundCueTimeMs = 0;
    g_engineFrameDelta = 0;
}

CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {
    Cleanup();
}

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
        SetInitError(WORLDERR_CHILD_GROUP);
        return 0;
    }
    if (!m_workerList->IsReady()) {
        SetInitError(WORLDERR_WORKER_LIST);
        return 0;
    }
    if (!m_imageRegistry->IsReady()) {
        SetInitError(WORLDERR_IMAGE_REGISTRY);
        return 0;
    }
    if (!m_logicRegistry->IsReady()) {
        SetInitError(WORLDERR_WORKER_CACHE);
        return 0;
    }
    if (!m_paletteRegistry->IsReady()) {
        SetInitError(WORLDERR_WORKER_MAP);
        return 0;
    }
    if (!m_animRegistry->IsReady()) {
        SetInitError(WORLDERR_ANIM_REGISTRY);
        return 0;
    }
    if (!m_level->SetViewportSize(w, h)) {
        SetInitError(WORLDERR_LEVEL_EXTENTS);
        return 0;
    }
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_DIRECT_OBJECT_MOVEMENT)) {
        m_level->m_flags |= 4;
    }
    if (!m_drawTarget->CreateChildren(w, h, bpp, flags)) {
        SetInitError(WORLDERR_CREATE_PAGES);
        return 0;
    }

    i32 cooperativeLevel = DSSCL_NORMAL;
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_SOUND_PRIORITY)) {
        cooperativeLevel = DSSCL_PRIORITY;
    }
    if (!m_soundStream->InitializeDevice(hWnd, cooperativeLevel)) {
        delete m_soundStream;
        m_soundStream = NULL;
        if (HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_REQUIRE_SOUND)) {
            SetInitError(WORLDERR_SOUND_OUTPUT);
            return 0;
        }
    }
    if (m_soundStream != NULL
        && HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_DISABLE_SOUND)) {
        delete m_soundStream;
        m_soundStream = NULL;
    }
    if (!m_soundRegistry->BindSoundStream(true)) {
        SetInitError(WORLDERR_SOUND_REGISTRY);
        return 0;
    }
    return 1;
}

void CDDrawSurfaceMgr::Cleanup() {
    SAFE_DELETE(m_level);
    SAFE_DELETE(m_soundRegistry);
    SAFE_DELETE(m_soundStream);
    SAFE_DELETE(m_drawTarget);
    SAFE_DELETE(m_childGroup);
    SAFE_DELETE(m_workerList);
    SAFE_DELETE(m_imageRegistry);
    SAFE_DELETE(m_logicRegistry);
    SAFE_DELETE(m_paletteRegistry);
    SAFE_DELETE(m_animRegistry);
    SAFE_DELETE(m_deviceManager);
    SetSerializationCallback(NULL);
}

b32 CDDrawSurfaceMgr::IsReady() {
    CDDrawSubMgrPages* first = m_drawTarget;

    return first != NULL && ChildGroup() != NULL && m_workerList != NULL && m_imageRegistry != NULL
           && m_logicRegistry != NULL && first->IsLoaded() != 0 && m_level != NULL;
}

void CDDrawSurfaceMgr::SetRestoreHandler(SurfaceRestoreFn handler) {
    SetSurfaceRestoreHandler(handler);
}

i32 CDDrawSurfaceMgr::SetDimensions(i32 x, i32 y, ColorDepth bpp) {
    CDDrawFrontSurface* child = m_drawTarget->GetFrontSurface();

    if (child->GetWidth() != x || child->GetHeight() != y) {
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

i32 CDDrawSurfaceMgr::EnsureSoundInitialized() {
    if (m_soundStream != NULL && m_soundStream->m_initialized == false) {
        return m_soundStream->InitializeDevice(m_hWnd, DSSCL_NORMAL);
    }
    return 1;
}

i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, char* path, char* name, LogicTypeId typeId) {
    if (!path || !name) return 0;
    io::FileTransaction file(path);
    return file.good() && SnapshotChildren(cb, file, name, typeId) && file.commit();
}

i32 CDDrawSurfaceMgr::SnapshotChildren(HP_Callback cb, io::Output& target, const std::string& name, LogicTypeId typeId) {
    SetSerializationCallback(cb);
    CStreamArchive S(target);
    if (!S.Open()) return 0;

    CSnapshotHeader header;
    memset(&header, 0, sizeof(header));

    CTime now = CTime::GetCurrentTime();
    header.m_version = 1;
    header.m_month = now.GetMonth();
    header.m_day = now.GetDay();
    header.m_year = now.GetYear();
    if (!copyTextToBuffer(name, header.m_name, sizeof(header.m_name))) return 0;
    header.m_childCount = ChildGroup()->CountActive();
    header.m_objIdCounter = g_wwdObjIdCounter;
    if (!S.Write(&header, sizeof(header))) return 0;

    if (!InvokeCallbackInline(&S, SERIAL_SNAPSHOT_BEGIN, LOGIC_UNSET, NULL)) {
        return 0;
    }
    if (!ChildGroup()->WriteObjectSnapshots(&S, typeId)) {
        return 0;
    }
    if (!InvokeCallbackInline(&S, SERIAL_PRESAVE, LOGIC_UNSET, NULL)) {
        return 0;
    }
    if (!ChildGroup()->DispatchSerializationToObjects(&S, SERIAL_PRESAVE, typeId)) {
        return 0;
    }
    if (!LevelOf(this)->SerializeDispatch(&S, SERIAL_PRESAVE, LOGIC_UNSET, 0)) {
        return 0;
    }
    if (!InvokeCallbackInline(&S, SERIAL_SAVE, LOGIC_UNSET, NULL)) {
        return 0;
    }
    if (!ChildGroup()->SerializeObjects(&S, typeId)) {
        return 0;
    }
    if (!LevelOf(this)->SerializeDispatch(&S, SERIAL_SAVE, LOGIC_UNSET, 0)) {
        return 0;
    }
    if (!InvokeCallbackInline(&S, SERIAL_POSTSAVE, LOGIC_UNSET, NULL)) {
        return 0;
    }
    if (!ChildGroup()->DispatchSerializationToObjects(&S, SERIAL_POSTSAVE, typeId)) {
        return 0;
    }
    if (!LevelOf(this)->SerializeDispatch(&S, SERIAL_POSTSAVE, LOGIC_UNSET, 0)) {
        return 0;
    }

    return S.Ready();
}

i32 CDDrawSurfaceMgr::RestoreChildren(HP_Callback cb, const std::string& path, LogicTypeId typeId) {
    if (path.empty()) return 0;
    io::File file;
    return file.open(path, io::ReadOnly) && RestoreChildren(cb, file, typeId) && file.finish();
}

i32 CDDrawSurfaceMgr::RestoreChildren(HP_Callback cb, io::Input& source, LogicTypeId typeId) {
    SetSerializationCallback(cb);
    CStreamArchive S(source);
    if (!S.Open()) return 0;

    CSnapshotHeader header;
    if (!S.Read(&header, sizeof(header))) return 0;

    if (!InvokeCallbackInline(&S, SERIAL_RESTORE_BEGIN, typeId, &header)) {
        return 0;
    }
    g_wwdObjIdCounter = header.m_objIdCounter;
    ChildGroup()->ClearChildren();
    if (!ChildGroup()->LoadObjects(&S, header.m_childCount, typeId)) {
        return 0;
    }
    if (!InvokeCallbackInline(&S, SERIAL_PRELOAD, typeId, &header)) {
        return 0;
    }
    if (!ChildGroup()->DispatchSerializationToObjects(&S, SERIAL_PRELOAD, typeId)) {
        return 0;
    }
    if (!LevelOf(this)->SerializeDispatch(&S, SERIAL_PRELOAD, LOGIC_UNSET, 0)) {
        return 0;
    }
    if (!InvokeCallbackInline(&S, SERIAL_LOAD, typeId, &header)) {
        return 0;
    }
    if (!ChildGroup()->DeserializeObjects(&S, header.m_childCount, typeId)) {
        return 0;
    }
    if (!LevelOf(this)->SerializeDispatch(&S, SERIAL_LOAD, LOGIC_UNSET, 0)) {
        return 0;
    }
    if (!InvokeCallbackInline(&S, SERIAL_POSTLOAD, typeId, &header)) {
        return 0;
    }
    if (!ChildGroup()->DispatchSerializationToObjects(&S, SERIAL_POSTLOAD, typeId)) {
        return 0;
    }
    if (!LevelOf(this)->SerializeDispatch(&S, SERIAL_POSTLOAD, LOGIC_UNSET, 0)) {
        return 0;
    }

    if (!S.Ready()) return 0;
    LevelOf(this)->DeactivateDistantObjectsOnMainPlane();
    return 1;
}

i32 CDDrawSurfaceMgr::DispatchSerializationCallback(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    void* payload
) {
    if (!ar) {
        return 0;
    }
    if (!SerializationCallback()) {
        return 0;
    }
    return SerializationCallback()(this, ar, mode, typeId, payload) != 0;
}

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

    if (!hdrOut || !S.Read(hdrOut, sizeof(CSnapshotHeader))) return 0;
    if (buf != NULL && len > 0) {
        if (len > 0x7fffffffU || !S.Read(buf, len)) return 0;
    }
    return S.Ready();
}
