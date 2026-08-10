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
SIZE(0x120);
#pragma pack(pop)

class CWapObj;
class CDDrawSubMgrPages;
class CDDrawWorkerList;
class CDDrawChildGroup;
class CDDrawWorkerRegistry;
class CDDrawWorkerCache;
class CDDrawWorkerMapSmall;
class CDDrawSubMgrLeafScan;
class CDDrawSubMgrLeaf;
class CDDrawPtrCollections;
class SoundStream;

// The archive's per-object callback: (ctx, stream, phase, type id, payload).
typedef i32(__cdecl* HP_Callback)(void*, void*, SerialMode, LogicTypeId, void*);

typedef i32(__cdecl* SurfaceRestoreFn)();

class CDDrawSurfaceMgr : public CObject {
public:
    CDDrawSurfaceMgr();

    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    virtual ~CDDrawSurfaceMgr() OVERRIDE;
    virtual i32 IsReady();

    virtual i32 Init(HWND hWnd, i32 w, i32 h, ColorDepth bpp, i32 flags);
    virtual void Cleanup();

    void FreeContext();
    i32 PlayDefaultSound();
    i32 SetDimensions(i32 x, i32 y, ColorDepth bpp);

    void SetRestoreHandler(SurfaceRestoreFn handler);

    // Two entities (docs/patterns/two-shapes-need-two-entities.md): retail's
    // out-of-line 0x156a90 body is what CDDrawChildGroup::LoadObjects calls, while
    // SnapshotChildren/RestoreChildren EXPAND the same logic - their first callback
    // site keeps the un-folded `lea eax,&S; test eax,eax` archive guard that only an
    // inline expansion leaves behind (a source-level `&S == NULL` folds away).
    i32 InvokeCallbackInline(void* ar, SerialMode mode, LogicTypeId typeId, void* payload) {
        return ar != NULL && m_callback != NULL && m_callback(this, ar, mode, typeId, payload) != 0;
    }
    i32 InvokeCallback(void* ar, SerialMode mode, LogicTypeId typeId, void* payload);

    i32 SnapshotChildren(HP_Callback cb, char* path, char* name, LogicTypeId typeId);
    i32 RestoreChildren(HP_Callback cb, char* name, LogicTypeId typeId);

    CDDrawSubMgrPages* m_drawTarget;

    CDDrawChildGroup* m_childGroup;
    CDDrawWorkerList* m_workerList;
    CDDrawWorkerRegistry* m_imageRegistry;

    CDDrawWorkerCache* m_workerCache;
    CDDrawWorkerMapSmall* m_workerMap;
    CDDrawPtrCollections* m_ptrColl;
    SoundStream* m_soundStream;

    class CGameLevel* m_level;
    CDDrawSubMgrLeafScan* m_soundRegistry;

    CDDrawSubMgrLeaf* m_animRegistry;

    HWND m_hWnd;
    i32 m_flags;
    GZ_ENUM_STORAGE(WorldInitError, u32) m_lastError;
    HP_Callback m_callback;
};
SIZE(0x40);
SIZE_UNKNOWN();

extern void __cdecl SetSurfaceRestoreHandler(SurfaceRestoreFn handler);

#endif // GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
