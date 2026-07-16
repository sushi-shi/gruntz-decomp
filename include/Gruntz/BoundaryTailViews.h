// BoundaryTailViews.h - shared referent/owner views for the hard CString/CSymParser/
// CImage tail of the engine_boundary vein reconstructed in BoundaryTail.cpp (DinMgr2 /
// Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). Formerly per-TU inline views; consolidating them here is pure code
// motion (matching-neutral) and gives the /GX EH-frame sibling (BoundaryTailEh.cpp) /
// a final sweep one definition to reuse.
#ifndef GRUNTZ_BOUNDARYTAILVIEWS_H
#define GRUNTZ_BOUNDARYTAILVIEWS_H

#include <rva.h>
#include <Mfc.h> // real MFC CString (copy-ctor 0x1b9ba3 / dtor 0x1b9cde, reloc-masked)

// 0x176d20 CImage scanline fill + 0x176da0 wrapper: folded onto the real CRezImage
// (src/Image/ImageRectFill.cpp, <Image/Image.h>) - the delinker-guessed CImg176d20 /
// FillRect176d20 placeholders removed.

// (The 0x788d0 "sound-emitter" view family - CSnd788d0/Elem*/Holder*/Emitter788d0 -
// is DISSOLVED: the method is CTriggerMgr::ScrollToActiveRecord (TriggerMgr.cpp);
// the walk was m_grid/m_recX/m_recY/m_level and the "emitter" the canonical
// CPlaneRender/CDDrawWorkerHost plane.)

// (Obj85500 is DISSOLVED, 2026-07-16: 0x85500 IS CGruntzMgr::GetRezPath - both
// callers (Run @0x83450 + LoadWorldMode) invoke it on the manager `this`, and the
// +0xec CString is the mgr's m_strRezPath. 0x38120 was folded onto the real
// CLatencyItem::GetName in src/Gruntz/SlotComboFill.cpp.)

// 0x148250 - CDDPalette::Flush: RE-HOMED to the directdrawmgr unit
// (src/DDrawMgr/DirectDrawMgr.cpp, real CDDPalette in DirectDrawMgr.h).

// (The 0x23d90 grid-snap blit view chain - CObj23d90/Outer23d90/Mid23d90/P23d90/
// R23d90 - is DISSOLVED 2026-07-16: 0x23d90 IS CGruntzCmdMgr::BlitTileMarker
// (<Gruntz/GruntzCmdMgr.h>), dispatched on [CGruntzMgr+0x6c] == m_cmdSubMgr; the
// chain was m_38(CGruntzMgr)->m_world->m_level(CGameLevel)'s m_planeCtx/m_mainPlane.)

// 0xbdd0 - look a key up in arg1's embedded map (at +0x10) then dispatch.
struct Entry_bdd0 {
    char _0[0x10];
    void* m_10; // 0x10
};
SIZE_UNKNOWN(Entry_bdd0);
// (The ex-`CMapStringToOb` view is DISSOLVED: an empty phantom aliasing the MFC library
// CMapStringToPtr::Lookup @0x1b8438 - the member is the real map. The class named here
// was inverted; 0x1b8438 is CMapStringToPtr's, 0x1b8008 is CMapStringToOb's. The
// declaration below was already right.)
struct Arg1_bdd0 {
    char _0[0x10];
    CMapStringToPtr m_10; // 0x10  ::CMapStringToPtr (its Lookup is 0x1b8438; mfc_class)
};
SIZE_UNKNOWN(Arg1_bdd0);
// (CObj_bdd0 dissolved: its Dispatch (0xbdd0) is CRandomAmbientSound::Dispatch and
//  its DispatchEntry tail call binds to CRandomAmbientSound::Setup @0xbe50.)

// 0x118330 - populate an output record from three successive iterator reads.
struct Node118330 {
    char _0[0xc];
    i32 m_c, m_10, m_14; // 0xc,0x10,0x14
};
SIZE_UNKNOWN(Node118330);
struct Iter118330 {
    void* pos;
    Node118330* GetNext(int x); // 0x1b30f0 (__thiscall)
};
SIZE_UNKNOWN(Iter118330);
struct Out118330 {
    char _0[0xc];
    i32 m_c, m_10, m_14; // 0xc,0x10,0x14
};
SIZE_UNKNOWN(Out118330);

#endif // GRUNTZ_BOUNDARYTAILVIEWS_H
