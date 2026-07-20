#ifndef GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
#define GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base

#ifndef _WINDEF_
struct HWND__;
typedef struct HWND__* HWND;
#endif

class CLoadable;
class CDDrawSubMgrPages;    // +0x04 the page/child factory (front/back/overlay surfaces)
class CDDrawWorkerList;     // +0x0c the per-frame worker pump (vtbl 0x1efd88; slot-13 PruneWorkers)
class CDDrawChildGroup;     // +0x08 the broadcast child-group (intrusive list + 2 maps)
class CDDrawWorkerRegistry; // +0x10 the name->sprite registry (: CLoadable, vtbl 0x5efd28, m_10map)
class CDDrawWorkerCache;    // +0x14 the string-keyed worker cache (its +0x10 map is the
class CDDrawWorkerMapSmall; // +0x18 the polymorphic sprite/palette registry (: CObject, 13 slots)
class CDDrawSubMgrLeafScan; // (class, not struct - the PAU/PAV fwd-mangling trap)
class CDDrawSubMgrLeaf;     // +0x2c the label sub-manager (KeyOfValue_152d30 / m_10 map)
struct CDDrawPtrCollections; // the +0x1c surface pool (heap object)
struct SoundStream;          // the +0x20 foreign Dsndmgr sound stream

typedef i32(__cdecl* HP_Callback)(void*, void*, i32, i32, i32);

SIZE(CDDrawSurfaceMgr, 0x40);
class CDDrawSurfaceMgr : public CObject {
public:
    CDDrawSurfaceMgr();
    // `new CDDrawSurfaceMgr` (CGruntzMgr::Init / RezSync) needs an accessible
    // operator new; MFC CObject's PASCAL one is not usable under MSVC5, so forward
    // to global new (byte-identical: the same `push 0x40; call ??2@YAPAXI@Z`).
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }
    // The real 8-slot retail vtable @0x1efc58 (== ??_7CDDrawSurfaceMgr@@6B@): CObject's
    // 5 slots (0-4, dtor override @1) + THREE CDDrawSurfaceMgr-own slots. The per-slot
    // vtable audit proves IsReady/Init/Cleanup are the ONLY new virtuals; FreeContext/
    // SetDimensions/SetHwnd/InvokeCallback occupy NO retail slot -> plain methods.
    virtual ~CDDrawSurfaceMgr() OVERRIDE; // slot 1  0x1558b0 (scalar-del ??_G 0x155890)
    virtual i32 IsReady();                // slot 5  0x155f00
    // slot 6  0x155900 (@stub): the display/video-mode bring-up - heap-allocate the 11
    // owned sub-managers, validate each (m_lastError 0x3e9..0x3f1) and configure the
    // display. Retail `ret 0x14` = FIVE args (the old no-arg decl under-declared it);
    // CGruntzMgr::LoadWorldMode dispatches it as its "SetVideoMode" (slot 6, +0x18).
    virtual i32 Init(void* hWnd, i32 w, i32 h, i32 bpp, i32 flags);
    virtual void Cleanup_155e20(); // slot 7  0x155e20 (owned-child teardown; ~ calls it;
                                   //         LoadWorldMode's pre-Init "Notify" dispatch)

    // Non-virtual methods (census-proven OFF the retail vtable - plain, not slots):
    void FreeContext();                                           // 0x155fc0
    i32 PlayDefaultSound();                                       // 0x155ff0
    i32 SetDimensions(i32 x, i32 y, i32 flags);                   // 0x155f60
    void SetHwnd(void* hWnd);                                     // 0x155f50
    i32 InvokeCallback(void* arg1, i32 arg2, i32 arg3, i32 arg4); // 0x156a90

    // The recursive child serializer / deserializer (owner-TU DDrawSurfaceMgrSerialize
    // holds the bodies; GameSave drives SnapshotChildren). Non-virtual __thiscall /GX.
    i32 SnapshotChildren(HP_Callback cb, i32 arg1, char* name, i32 arg3); // 0x156020
    i32 RestoreChildren(HP_Callback cb, char* name, i32 arg3);            // 0x156530

    // +0x04  the page/child factory (front/back/overlay surfaces) - the game-side
    // draw target (the ex CDDrawSubMgrPages/StateMgrBZ views; every CState::m_c consumer
    // reaches the flip pump + the three CDDrawSurfacePair pages through it).
    CDDrawSubMgrPages* m_drawTarget;
    union { // +0x08 one object, two facet types (the sprite factory IS the walked list host)
        CDDrawChildGroup* m_childGroup;
        class CQueueDrainHost* m_walkHost; // ListGetFirst/Next facet (ex the m_objList->m_coll view)
    }; // +0x08  broadcast child-group
    CDDrawWorkerList* m_workerList; // +0x0c  the per-frame worker pump (real type; ex "renderer B")
    CDDrawWorkerRegistry* m_imageRegistry; // +0x10  name->sprite/image registry (m_10map; vtbl
                                           //        0x5efd28; ex "m_imageRegistry" - it is a
                                           //        worker/name registry, not a DDSURFACEDESC)
    CDDrawWorkerCache* m_workerCache;      // +0x14  the string-keyed worker cache (real type)
    CDDrawWorkerMapSmall* m_workerMap;     // +0x18  the sprite/palette registry (real type)
    CDDrawPtrCollections* m_ptrColl;       // +0x1c  surface pool
    SoundStream* m_soundStream;            // +0x20  foreign Dsndmgr sound stream
    // +0x24: "CDDrawResolveSubMgr" IS the canonical CGameLevel - PROVEN: Init news
    // it with new(0x6d4) + ctor 0x15ccd0 == SIZE(CGameLevel, 0x6d4) + ??0CGameLevel.
    class CGameLevel* m_level;             // (ex "m_level" - the canonical CGameLevel)
    CDDrawSubMgrLeafScan* m_soundRegistry; // +0x28  sound/cue registry (CSndHost typedef;
                                           //        ex "m_soundRegistry")
    CDDrawSubMgrLeaf*
        m_animRegistry;     // +0x2c  the ANI catalog / anim registry (ex "m_animRegistry";
                            //        KeyOfValue_152d30 + the ANI factory set)
    HWND m_hWnd;            // +0x30  bound window / device handle
    i32 m_flags;            // +0x34  caps flags
    i32 m_lastError;        // +0x38  last-error code
    HP_Callback m_callback; // +0x3c  run/config callback
};

#endif // GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
