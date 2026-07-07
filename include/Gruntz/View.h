// View.h - the two MFC-dependent render sub-object classes of the CState::m_c holder.
//
// The holder at CState::m_c (+0x0c) is the canonical `CSpriteFactoryHolder`
// (<Gruntz/GameRegistry.h>) - the SAME object as CGameRegistry::m_world (+0x30), verified
// NON-polymorphic (offset 0 is padding, no vtable on the object itself). The former fake
// `CView` name (a MISATTRIBUTION: `sema class CView` shows 0x5ee1c4 is the *real MFC*
// `CView : CWnd` vtable, an unrelated library class that collided with MFC's CView and
// blocked pulling <afxwin.h> for CRgn) is GONE - CView is folded onto
// CSpriteFactoryHolder, and its sub-object facets onto the ONE real classes:
//   * +0x04 render-pump / draw target  -> CDrawTarget      (<Gruntz/ResMgr.h>)
//   * +0x10 image/name registry        -> CImageRegistry   (<Gruntz/ResMgr.h>)
//   * +0x28 sound registry (+0x2c res) -> CSoundRegistry    (<Gruntz/ResMgr.h>)
//   * +0x2c anim registry              -> CAnimRegistry     (<Gruntz/ResMgr.h>)
// Only the +0x08/+0x0c renderer and the +0x24 draw-surface facet stay HERE (they need the
// polymorphic renderer vtable / the MFC RECT that the afx-neutral ResMgr.h can't carry).
//
// REMAINING CASTS (binary-authentic dual-use, NOT removable views - the doctrine's
// "int-pair overlaid as a struct view" allowance): the MFC state TUs reach these two from
// the canonical holder members by cast because the field is genuinely used two ways:
//   * (CRenderer*)holder->m_8   - +0x08 is the sprite factory (CreateSprite, 60+ sites) AND
//                                 renderer A (BeginScene/plane-list); one polymorphic object.
//   * (CDrawSurface*)holder->m_24 - +0x24's m_5c is an int for `m_5c+0x40` pointer-arith in
//                                 40+ Grunt.cpp sites but a CameraGeom* here (int-vs-pointer).
//   * (CSoundRegistry*)holder->m_28 - +0x28 is CSndHost (cue: m_10 CSndFinder, m_2c stream)
//                                 AND CSoundRegistry (named-set: m_10map hash, m_2c pooled).
//
// This header is pulled by the MFC state TUs (CPlay.h, GameMode.h) AFTER <Mfc.h>;
// CState.h keeps only a forward decl so the ~60 pure-Win32 TUs stay afx-neutral.
#ifndef GRUNTZ_GRUNTZ_CVIEW_H
#define GRUNTZ_GRUNTZ_CVIEW_H

#include <Mfc.h> // RECT (CDrawSurface::SetClipRect / m_viewport)
#include <rva.h>

// The per-frame input object the credits poll reaches (m_4->m_10->m_2c->m_8); its
// full vtable layout lives in GameMode.h (the only TU that polls it).
struct CGMInputObj;

// The render-flip surface at CDrawTarget::SurfaceB::m_2c: the real CDDSurface
// (<DDrawMgr/DDSurface.h>; Fill @0x13e760, Restore @0x13e7d0 - ret 8, two args -
// held IDirectDrawSurface at its +0x08). Pointer-only here; the dispatching TUs
// include the real header.
class CDDSurface;

// The placed-object display list the warlord-sprite loader walks (hung off
// renderer A at +0x10; see CRenderer::m_10). Each node's +0x8 is a placed object.
struct CWarlordListNode; // fully defined in CPlay.cpp
SIZE_UNKNOWN(CWarlordListHead);
struct CWarlordListHead {
    char p0[0x4];
    CWarlordListNode* m_4; // +0x04  first node
};

// The renderer/draw object (holder->m_8 = renderer A facet, holder->m_rendererB = renderer B). A
// FOREIGN engine class (its ??_7 and most slots are unreconstructed engine code),
// modeled POLYMORPHIC: placeholder slots anchor the two dispatched virtuals at
// their true vtable offsets - begin-scene at slot 9 (+0x24, 1 arg) and present at
// slot 13 (+0x34, 2 args), both __thiscall. Never instantiated here (reached only
// via pointer + virtual dispatch), so MSVC emits no ??_7 and the calls fold to
// `mov eax,[ecx]; call [eax+0x24]` / `call [eax+0x34]` - the same shape the old
// manual CRendererVtbl PMF table produced; real virtuals are the byte-correct
// form (the CImageRegistry/DecCounter precedent in ResMgr.h).
SIZE_UNKNOWN(CRenderer);
struct CRenderer {
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void BeginScene(i32 z); // slot 9  (+0x24)
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void Present(void* a, void* b); // slot 13 (+0x34)
    // Refresh @0x159ef0 = CDDrawSubMgrPages::Method_159ef0, DisposeWorkers @0x163c60 =
    // CDDrawWorkerList::ClearWorkers; cast at each call.
    // Renderer A owns the placed-object display list at +0x10 (LoadWarlordSprites
    // walks it in-level; the implicit vptr is at +0x00, so data starts at +0x04).
    char p04[0x10 - 0x4];
    CWarlordListHead m_10; // +0x10  placed-object list head (0x10..0x17)
    char p18[0x1c - 0x18];
    i32 m_1c; // +0x1c  live game-object count (CPlay::DrawDebugStats "Objs = %i")
};

// The draw-surface object at m_c->m_24 (the target of the thiscall PushView +
// the ViewPreStep/PostStep sub-steps). Its +0x5c holds the camera geometry the
// world blit reads (+0x84 / +0x88).
struct CDrawSurface {
    void PushView(void* view, void* renderer);
    void PreStep();
    void PostStep();
    void SetClipRect(RECT* r); // 0x15da80 (thiscall) ClampViewport apply-tail
    char p0[0x10];
    // +0x10: the viewport rect {left,top,right,bottom}; StepScroll reads .left/.top
    // as the scroll origin, DispatchHudClick reads all four as the bounds box.
    RECT m_viewport; // +0x10  viewport rect (also the scroll origin .left/.top)
    char p20[0x5c - 0x20];
    // +0x5c -> a geom block: StepScroll reads (m_5c+0x40).{m_originX,m_originY};
    // the world blit reads (m_5c).{m_84,m_88}.
    struct CameraGeom {
        void DrawA(); // 0x563300  per-frame world-draw sub-step A
        void DrawB(); // 0x563370  per-frame world-draw sub-step B
        char p0[0x40];
        i32 m_originX; // +0x40
        i32 m_originY; // +0x44
        char p48[0x84 - 0x48];
        i32 m_84;
        i32 m_88;
    }* m_5c; // +0x5c camera geom
};

// The remaining shared sub-objects of CSpriteFactoryHolder are the ONE real classes,
// NOT View.h views (all former CSpriteFactoryHolder facet views are folded away):
//   * +0x04 render-pump / draw target -> CDrawTarget      (<Gruntz/ResMgr.h>)
//   * +0x08 renderer A / +0x0c renderer B -> CRenderer    (below; polymorphic engine class)
//   * +0x10 image/name registry -> CImageRegistry         (<Gruntz/ResMgr.h>)
//   * +0x24 draw surface / viewport -> CDrawSurface        (below; the RECT/CameraGeom view)
//   * +0x28 sound registry (+0x2c pooled res) -> CSoundRegistry (<Gruntz/ResMgr.h>)
//   * +0x2c anim registry -> CAnimRegistry                 (<Gruntz/ResMgr.h>)
// The two classes kept here need the MFC/RECT + the polymorphic renderer vtable, so they
// stay in this afx-pulling header (the ResMgr.h classes are afx-neutral).

#endif // GRUNTZ_GRUNTZ_CVIEW_H
