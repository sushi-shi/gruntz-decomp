#ifndef GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
#define GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/Object.h>

#ifndef _WINDEF_
struct HWND__;
typedef struct HWND__* HWND;
#endif

#pragma pack(push, 1)
struct CSnapshotHeader {
    i32 m_reserved;
    i32 m_version;
    i32 m_month;
    i32 m_dayThenYear;
    char m_name[0x110 - 0x10];
    u32 m_childCount;
    u32 m_objIdCounter;
    i32 m_activeCount;
    char m_pad11c[0x120 - 0x11c];
};
SIZE(0x120);
#pragma pack(pop)

class CLoadable;
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

typedef i32(__cdecl* HP_Callback)(void*, void*, i32, i32, void*);

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

    virtual i32 Init(HWND hWnd, i32 w, i32 h, i32 bpp, i32 flags);
    virtual void Cleanup();

    void FreeContext();
    i32 PlayDefaultSound();
    i32 SetDimensions(i32 x, i32 y, i32 flags);

    void SetRestoreHandler(SurfaceRestoreFn handler);

    i32 InvokeCallback(void* ar, i32 mode, i32 typeId, void* payload);

    i32 SnapshotChildren(HP_Callback cb, char* path, char* name, i32 typeId);
    i32 RestoreChildren(HP_Callback cb, char* name, i32 typeId);

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
    i32 m_lastError;
    HP_Callback m_callback;
};
SIZE(0x40);
SIZE_UNKNOWN();

extern void __cdecl SetSurfaceRestoreHandler(SurfaceRestoreFn handler);

#endif // GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
