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
    // @identity-TODO: SnapshotChildren zeroes and writes the complete header;
    // RestoreChildren reads this tail but assigns no meaning to its bytes.
    char m_reserved118[0x120 - 0x118];
};
#pragma pack(pop)

class CWapObj;
class CDDrawSubMgrPages;
class CDDrawWorkerList;
class CImageSet;
class CDDrawChildGroup;
class CImageSetRegistry;
class CLogicRecordRegistry;
class CDDrawPaletteRegistry;
class SoundCueRegistry;
class AnimationRegistry;
class CDDrawDeviceManager;
class SoundStream;

class CDDrawSurfaceMgr;
class CFileMemBase;

typedef i32(__cdecl* HP_Callback)(CDDrawSurfaceMgr*, CFileMemBase*, SerialMode, LogicTypeId, void*);

typedef i32(__cdecl* SurfaceRestoreFn)();

GZ_ENUM_FLAGS_BEGIN(DDrawSurfaceMgrFlags, i32)
    SURFACEMGR_SKIP_OVERLAY = 0x01,
    SURFACEMGR_TRIPLE_BUFFER = 0x02,
    SURFACEMGR_DISABLE_SOUND = 0x04,
    SURFACEMGR_REQUIRE_SOUND = 0x08,
    SURFACEMGR_EMULATION_ONLY = 0x10,
    SURFACEMGR_DIRECT_OBJECT_MOVEMENT = 0x20,
    SURFACEMGR_CONSUME_ANIMATION_EVENTS = 0x40,
    SURFACEMGR_SOUND_PRIORITY = 0x80,
    SURFACEMGR_SINGLE_FRAME_IMAGE_SETS = 0x100
GZ_ENUM_FLAGS_END(DDrawSurfaceMgrFlags, i32)
GZ_ENUM_FLAGS_OPS(DDrawSurfaceMgrFlags)

class CDDrawSurfaceMgr : public CObject {
public:
    inline CImageSet* FindImageSet(LPCTSTR name);
    inline class CImage* FindFrame(LPCTSTR name, i32 index);
    CDDrawSurfaceMgr();

    virtual ~CDDrawSurfaceMgr() OVERRIDE;
    virtual b32 IsReady();

    virtual i32 Init(HWND hWnd, i32 w, i32 h, ColorDepth bpp, i32 flags);
    virtual void Cleanup();

    CDDrawSubMgrPages* const& GetDrawTarget() const {
        return m_drawTarget;
    }

    class CGameLevel* GetLevel() const {
        return m_level;
    }

    CDDrawChildGroup* ChildGroup() {
        return m_childGroup;
    }

    CDDrawDeviceManager* GetDeviceManager() {
        return m_deviceManager;
    }

    SoundStream* GetSoundStream() {
        return m_soundStream;
    }

    SoundCueRegistry* SoundRegistry() {
        return m_soundRegistry;
    }

    CImageSetRegistry* GetImageRegistry() {
        return m_imageRegistry;
    }

    CLogicRecordRegistry* GetLogicRegistry() {
        return m_logicRegistry;
    }

    AnimationRegistry* GetAnimationRegistry() {
        return m_animRegistry;
    }

    void FreeContext();
    i32 EnsureSoundInitialized();
    i32 SetDimensions(i32 x, i32 y, ColorDepth bpp);

    void SetRestoreHandler(SurfaceRestoreFn handler);

    void SetInitError(WorldInitError err) {
        if (m_lastError == WORLDERR_NONE) {
            m_lastError = err;
        }
    }

    HP_Callback SerializationCallback() {
        return m_callback;
    }

    void SetSerializationCallback(HP_Callback callback) {
        m_callback = callback;
    }

    i32 InvokeCallbackInline(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, void* payload) {
        return ar != NULL && SerializationCallback() != NULL
               && SerializationCallback()(this, ar, mode, typeId, payload) != 0;
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
    CImageSetRegistry* m_imageRegistry;

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
