#ifndef GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
#define GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H

// DDrawSurfaceMgr.h - THE single-source shape of CDDrawSurfaceMgr, the root object
// of the tomalla-named DDraw surface/page-manager family. It is owned off
// CGruntzMgr+0x30 (RezSync news it), holds one child sub-manager per slot, a foreign
// Dsndmgr sound stream at +0x20, the surface pool at +0x1c, and a run/config
// callback at +0x3c. A polymorphic CObject (implicit vptr @+0x00; the ctor
// 0x155840 stamps ??_7CDDrawSurfaceMgr, the dtor 0x1558b0 restamps the grand base).
//
// Field/offset provenance (union of the six former per-TU views, name-preserving):
//   +0x04  m_pages       CDDrawSubMgrPages*  - PROVEN by the Init store 0x15596a:
//                        `new(0x1c)` whose vtable is stamped ??_7CDDrawSubMgrPages
//                        (0x5efe08) at 0x15594e, then `mov [this+4],edi`. The former
//                        "CDDrawWorkerMgr" view (0x158b40..0x159ef0 surface ops) and
//                        RezSync's "SurfWorkerZ" are the SAME class, now one shared
//                        <DDrawMgr/DDrawSubMgrPages.h> (23-slot vtable 0x5efe08).
//   +0x08  m_childGroup  CDDrawChildGroup*   (Serialize's m_08 blit-op target;
//                        <DDrawMgr/DDrawChildGroup.h>)
//   +0x0c  m_workerList  CDDrawWorkerList
//   +0x10  m_surfaceDesc CDDrawSurfaceDesc submgr (static DDSURFACEDESC)
//   +0x14  m_workerCache CDDrawWorkerCache
//   +0x18  m_workerMap   CDDrawWorkerMapSmall
//   +0x1c  m_ptrColl     CDDrawPtrCollections - the surface pool (SurfacePair's m_pool);
//                        heap object, ctor 0x141cc0 / dtor 0x141d50, ~0x948 B.
//   +0x20  m_soundStream foreign Dsndmgr SoundStream
//   +0x24  m_resolveSubMgr CDDrawResolveSubMgr / CGameLevel (RezSync m_24 BuildAllPlanes;
//                        Serialize m_24 blit target)
//   +0x28  m_leafScan    CDDrawSubMgrLeafScan (RezSync m_28 ScanTree/HasKeyEqual/MatchSub)
//   +0x2c  m_leaf        CDDrawSubMgrLeaf
//   +0x30  m_hWnd        bound window handle (Init arg1; SurfacePair reads it as the
//                        "device" handle it hands the pool)
//   +0x34  m_flags       Init arg5 / caps flags (SurfacePair: bit4 fullscreen, bit1
//                        double-buffer)
//   +0x38  m_lastError   last-error code slot (Init 0x3e9..0x3ee, CreateChildren
//                        0x7d1..0x7d3, SurfacePair 0xfa1../0x80e9../0xbb9/0xbba;
//                        the owner's former m_initError / SubMgrPages' m_38)
//   +0x3c  m_callback    run/config callback (owner InvokeCallback + Serialize's m_3c
//                        run-callback; same __cdecl 5-arg signature)
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing. Every
// child is a forward-declared pointer member so this header does not drag in - or
// clash with - a consumer's local child views; the owner TU (DDrawSurfaceMgr.cpp)
// supplies the full child defs for its own method bodies.

#include <rva.h>
#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base

// HWND for the +0x30 window handle. When windows.h has NOT been pulled in (the lean
// consumers, e.g. GameSave), provide the STRICT (afx-default) incomplete-handle form
// so the pointer member compiles; when it HAS (the MFC/Win32 TUs, _WINDEF_ set), use
// the real one. m_hWnd is never dereferenced, so an incomplete HWND suffices.
#ifndef _WINDEF_
struct HWND__;
typedef struct HWND__* HWND;
#endif

// The owned child sub-managers (polymorphic; every child derives the shared 9-slot
// CLoadable base - scalar-deleting dtor at slot 1, IsLoaded/IsReady/Unload/GetStateId
// at 5-8; <Gruntz/Loadable.h>, the ex "CDDrawSubMgr" identity). Pointer members only.
class CLoadable;
class CDDrawSubMgrPages;     // +0x04 the page/child factory (front/back/overlay surfaces)
class CDDrawChildGroup;      // +0x08 the broadcast child-group (intrusive list + 2 maps)
class CDDrawWorkerCache;     // +0x14 the string-keyed worker cache (its +0x10 map is the
                             //       name->value resolve map the CWwdObjMgr factories read)
class CDDrawWorkerMapSmall;  // +0x18 the polymorphic sprite/palette registry (: CObject, 13 slots)
class CDDrawSubMgrLeafScan;  // (class, not struct - the PAU/PAV fwd-mangling trap)
struct CDDrawPtrCollections; // the +0x1c surface pool (heap object)
struct SoundStream;          // the +0x20 foreign Dsndmgr sound stream

// The +0x3c run/config callback: a __cdecl function pointer invoked with
// (this, arg1..arg4). Owner InvokeCallback dispatch == Serialize's run-callback.
typedef i32(__cdecl* HP_Callback)(void*, void*, i32, i32, i32);

SIZE(CDDrawSurfaceMgr, 0x40);
class CDDrawSurfaceMgr : public CObject {
public:
    CDDrawSurfaceMgr();
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

    CDDrawSubMgrPages* m_pages;       // +0x04  page/child factory (front/back/overlay)
    CDDrawChildGroup* m_childGroup;   // +0x08  broadcast child-group
    CLoadable* m_workerList;          // +0x0c  CDDrawWorkerList
    CLoadable* m_surfaceDesc;         // +0x10  CDDrawWorkerRegistry (1 map @0x10, vtbl 0x5efd28)
    CDDrawWorkerCache* m_workerCache; // +0x14  the string-keyed worker cache (real type)
    CDDrawWorkerMapSmall* m_workerMap; // +0x18  the sprite/palette registry (real type)
    CDDrawPtrCollections* m_ptrColl;  // +0x1c  surface pool
    SoundStream* m_soundStream;       // +0x20  foreign Dsndmgr sound stream
    // +0x24: "CDDrawResolveSubMgr" IS the canonical CGameLevel - PROVEN: Init news
    // it with new(0x6d4) + ctor 0x15ccd0 == SIZE(CGameLevel, 0x6d4) + ??0CGameLevel.
    class CGameLevel* m_resolveSubMgr;
    CDDrawSubMgrLeafScan* m_leafScan; // +0x28  CDDrawSubMgrLeafScan
    CLoadable* m_leaf;                // +0x2c  CDDrawSubMgrLeaf
    HWND m_hWnd;                      // +0x30  bound window / device handle
    i32 m_flags;                      // +0x34  caps flags
    i32 m_lastError;                  // +0x38  last-error code
    HP_Callback m_callback;           // +0x3c  run/config callback
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_DDRAWMGR_CDDRAWSURFACEMGR_H
