#ifndef GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
#define GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H

#include <rva.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/WorldInitError.h>
#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Wap32/Object.h>

#ifndef _WINDEF_
struct HWND__;
typedef struct HWND__* HWND;
#endif

#pragma pack(push, 1)
struct CSnapshotHeader {
    i32 m_version;
    i32 m_month;
    i32 m_day;
    i32 m_year;
    char m_name[0x110 - 0x10];
    u32 m_childCount;
    u32 m_objIdCounter;
    char m_reserved118[0x120 - 0x118];
};
#pragma pack(pop)

class CWapObj;
class CDDrawSubMgrPages;
class CDDrawWorkerList;
class CDDrawChildGroup;
class CDDrawWorkerRegistry;
class CLogicRecordRegistry;
class CDDrawPaletteRegistry;
class SoundCueRegistry;
class AnimationRegistry;
class CDDrawDeviceManager;
class SoundStream;

class CDDrawSurfaceMgr;
class CFileMemBase;

// The archive's per-object callback: (ctx, stream, phase, type id, payload).
typedef i32(__cdecl* HP_Callback)(CDDrawSurfaceMgr*, CFileMemBase*, SerialMode, LogicTypeId, void*);

typedef i32(__cdecl* SurfaceRestoreFn)();

// Options retained by CDDrawSurfaceMgr::Init and consumed by the renderer,
// sound setup, animation cursor, and worker loader. The bit positions are
// recovered from those consumers; 0x200 and above remain unenumerated because
// no surface-manager consumer establishes their meaning.
GZ_ENUM_FLAGS_BEGIN(DDrawSurfaceMgrFlags, i32)
    SURFACEMGR_SKIP_OVERLAY = 0x01,
    SURFACEMGR_TRIPLE_BUFFER = 0x02,
    SURFACEMGR_DISABLE_SOUND = 0x04,
    SURFACEMGR_REQUIRE_SOUND = 0x08,
    SURFACEMGR_EMULATION_ONLY = 0x10,
    SURFACEMGR_DIRECT_OBJECT_MOVEMENT = 0x20,
    SURFACEMGR_CONSUME_ANIMATION_DRAW_VALUES = 0x40,
    SURFACEMGR_SOUND_PRIORITY = 0x80,
    SURFACEMGR_SINGLE_FRAME_WORKERS = 0x100
GZ_ENUM_FLAGS_END(DDrawSurfaceMgrFlags, i32)
GZ_ENUM_FLAGS_OPS(DDrawSurfaceMgrFlags)

class CDDrawSurfaceMgr : public CObject {
public:
    CDDrawSurfaceMgr();

    void* operator new(size_t n) {
        return ::operator new(n);
    }

    virtual ~CDDrawSurfaceMgr() OVERRIDE;
    virtual i32 IsReady();

    virtual i32 Init(HWND hWnd, i32 w, i32 h, ColorDepth bpp, i32 flags);
    virtual void Cleanup();

    void FreeContext();
    i32 EnsureSoundInitialized();
    i32 SetDimensions(i32 x, i32 y, ColorDepth bpp);

    void SetRestoreHandler(SurfaceRestoreFn handler);

    i32 InvokeCallbackInline(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, void* payload) {
        return ar != NULL && m_callback != NULL && m_callback(this, ar, mode, typeId, payload) != 0;
    }
    i32 DispatchSerializationCallback(
        CFileMemBase* ar,
        SerialMode mode,
        LogicTypeId typeId,
        void* payload
    );

    i32 SnapshotChildren(HP_Callback cb, char* path, char* name, LogicTypeId typeId);
    i32 RestoreChildren(HP_Callback cb, char* name, LogicTypeId typeId);

    CDDrawSubMgrPages* m_drawTarget;

    CDDrawChildGroup* m_childGroup;
    CDDrawWorkerList* m_workerList;
    CDDrawWorkerRegistry* m_imageRegistry;

    CLogicRecordRegistry* m_logicRegistry;
    CDDrawPaletteRegistry* m_paletteRegistry;
    CDDrawDeviceManager* m_deviceManager;
    SoundStream* m_soundStream;

    class CGameLevel* m_level;
    SoundCueRegistry* m_soundRegistry;

    AnimationRegistry* m_animRegistry;

    HWND m_hWnd;
    i32 m_flags;
    GZ_ENUM_STORAGE(WorldInitError, u32) m_lastError;
    HP_Callback m_callback;
};

extern void __cdecl SetSurfaceRestoreHandler(SurfaceRestoreFn handler);

#endif // GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
