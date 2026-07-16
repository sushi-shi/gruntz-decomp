// View.h - documents the CState::m_c holder's render facets (each on its real
// canonical class).
//
// The holder at CState::m_c (+0x0c) is the canonical `CDDrawSurfaceMgr`
// (<Gruntz/GameRegistry.h>) - the SAME object as CGameRegistry::m_world (+0x30), verified
// NON-polymorphic (offset 0 is padding, no vtable on the object itself). The former fake
// `CView` name (a MISATTRIBUTION: `sema class CView` shows 0x5ee1c4 is the *real MFC*
// `CView : CWnd` vtable, an unrelated library class that collided with MFC's CView and
// blocked pulling <afxwin.h> for CRgn) is GONE - CView is folded onto
// CDDrawSurfaceMgr, and its sub-object facets onto the ONE real classes:
//   * +0x04 render-pump / draw target  -> CDDrawSubMgrPages      (<Gruntz/ResMgr.h>)
//   * +0x08 renderer A / factory       -> CDDrawChildGroup (<DDrawMgr/DDrawChildGroup.h>)
//   * +0x0c renderer B / worker pump   -> CDDrawWorkerList (<DDrawMgr/DDrawWorkerList.h>)
//   * +0x10 image/name registry        -> CImageRegistry   (<Gruntz/ResMgr.h>)
//   * +0x24 draw surface / level       -> CGameLevel       (<Gruntz/GameLevel.h>)
//   * +0x28 sound registry (+0x2c res) -> CDDrawSubMgrLeafScan    (<Gruntz/ResMgr.h>)
//   * +0x2c anim registry              -> CDDrawSubMgrLeaf  (<DDrawMgr/DDrawSubMgrLeaf.h>)
//
// REMAINING CASTS (binary-authentic dual-use, NOT removable views - the doctrine's
// "int-pair overlaid as a struct view" allowance): the MFC state TUs reach these from
// the canonical holder members by cast because the field is genuinely used two ways:
//   * (CDrawSurface*)holder->m_24 - +0x24's m_5c is an int for `m_5c+0x40` pointer-arith in
//                                 40+ Grunt.cpp sites but a CameraGeom* here (int-vs-pointer).
//   * (CDDrawSubMgrLeafScan*)holder->m_28 - +0x28 is CSndHost (cue: m_10 CSndFinder, m_2c stream)
//                                 AND CDDrawSubMgrLeafScan (named-set: m_10map hash, m_2c pooled).
//
// This header is pulled by the MFC state TUs (CPlay.h, GameMode.h) AFTER <Mfc.h>;
// CState.h keeps only a forward decl so the ~60 pure-Win32 TUs stay afx-neutral.
#ifndef GRUNTZ_GRUNTZ_CVIEW_H
#define GRUNTZ_GRUNTZ_CVIEW_H

#include <Mfc.h> // RECT (CDrawSurface::SetClipRect / m_viewport)
#include <rva.h>

// (The per-frame "input object" the credits poll reaches (m_4->m_10->m_2c->m_8) is
// the real IDirectDrawSurface - the poll is IsLost, COM slot 24; the dispatching TUs
// pull <ddraw.h>.)

// The render-flip surface at each CDDrawSubMgrPages page's m_surface: the real CDDSurface
// (<DDrawMgr/DDSurface.h>; Fill @0x13e760, Restore @0x13e7d0 - ret 8, two args -
// held IDirectDrawSurface at its +0x08). Pointer-only here; the dispatching TUs
// include the real header. (The former nested CDDrawSubMgrPages::SurfaceA/SurfaceB page
// views are dissolved onto CDDrawSurfacePair - see ResMgr.h.)
class CDDSurface;

// The `CRenderer` conflation is TWO real classes:
//   * renderer A (holder->m_8)  IS the canonical CDDrawChildGroup
//     (<DDrawMgr/DDrawChildGroup.h>, 17-slot vtable 0x1efdc0): proven by
//     CDDrawSurfaceMgr::Init @0x155900, which news the 0x6c-byte object (ctor
//     0x157630 / inline stamp 0x5efdc0) and stores it at +0x08. "BeginScene(i32)"
//     was its slot 9, TickKillCues_159a70(i32) - same slot, same arity, same
//     dispatch bytes. "Refresh" (0x159ef0) is its DestroyChildren_159ef0.
//     The placed-object list head/count ("CWarlordListHead/Node", "+0x10/+0x1c")
//     are its +0x10 CObList facet - m_head @+0x14 / m_count @+0x1c, node =
//     CDDrawGroupNode (whose union carries the game-side CGameObject reading).
//   * renderer B (holder->m_workerList, +0x0c) IS the canonical CDDrawWorkerList
//     (<DDrawMgr/DDrawWorkerList.h>, 14-slot vtable 0x1efd88): Init news the
//     0x2c-byte object and stores it at +0x0c. "Present(a, b)" was its slot 13,
//     PruneWorkers(CDDrawSurfacePair*, CDDrawSurfacePair*) - same slot, same
//     arity. "DisposeWorkers" (0x163c60) is its non-virtual ClearWorkers.
// Both real classes carry every slot evidence-backed.

// The draw-surface object at m_c->m_level is the ONE real CGameLevel
// (<Gruntz/GameLevel.h>) - the former per-TU CDrawSurface render-facet view AND the
// former GameRegistry.h `CGameViewport` facet are both folded onto it ("PushView" IS
// ?VisitVisible@CGameLevel@@ @0x15dc90, "SetClipRect" IS ?BuildAllPlanes@CGameLevel@@
// @0x15da80; the "+0x10 viewport rect" is CGameLevel::m_planeCtx, the "+0x5c camera
// geom" is CGameLevel::m_mainPlane, a CLevelPlane). No separate view here.

// (CDDrawSurfaceMgr::Init @0x155900 is the writer; it news both objects with their
// inline ctors stamping 0x5efdc0 (A, CDDrawChildGroup) and 0x5efd88 (B,
// CDDrawWorkerList) - both from the old slot-scan shortlist. The other two
// shortlist vtables were CGameLevel's (0x1f0150, ctor 0x15ccd0) and
// CDDrawSurfacePair's (0x1eff30). The old VTBL(CRenderer, 0x001ee1c4) misbinding
// story - 0x1ee1c4 is the real MFC ??_7CView@@6B@, config/library_vtables.csv -
// remains recorded there.)

#endif // GRUNTZ_GRUNTZ_CVIEW_H
