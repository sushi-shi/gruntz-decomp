// BoundaryTailViews.h - shared referent/owner views for the hard CString/CSymParser/
// CImage tail of the engine_boundary vein reconstructed in BoundaryTail.cpp (DinMgr2 /
// Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
#ifndef GRUNTZ_BOUNDARYTAILVIEWS_H
#define GRUNTZ_BOUNDARYTAILVIEWS_H

#include <rva.h>
#include <Mfc.h> // real MFC CString (copy-ctor 0x1b9ba3 / dtor 0x1b9cde, reloc-masked)

// 0x176d20 CImage scanline fill + 0x176da0 wrapper are CRezImage methods
// (src/Image/ImageRectFill.cpp, <Image/Image.h>).

// 0x788d0 is CTriggerMgr::ScrollToActiveRecord (TriggerMgr.cpp); the walk is
// m_grid/m_recX/m_recY/m_level and the "emitter" is the canonical
// CPlaneRender/CDDrawWorkerHost plane.

// 0x85500 IS CGruntzMgr::GetRezPath - both callers (Run @0x83450 + LoadWorldMode)
// invoke it on the manager `this`, and the +0xec CString is the mgr's m_strRezPath.
// 0x38120 is CLatencyItem::GetName in src/Gruntz/SlotComboFill.cpp.

// 0x148250 - CDDPalette::Flush lives in the directdrawmgr unit
// (src/DDrawMgr/DirectDrawMgr.cpp, real CDDPalette in DirectDrawMgr.h).

// 0x23d90 IS CGruntzCmdMgr::BlitTileMarker (<Gruntz/GruntzCmdMgr.h>), dispatched on
// [CGruntzMgr+0x6c] == m_cmdSubMgr; the grid-snap blit chain is
// m_38(CGruntzMgr)->m_world->m_level(CGameLevel)'s m_planeCtx/m_mainPlane.

// (The ex Arg1_bdd0/Entry_bdd0 pair for the 0xbdd0 Dispatch is DISSOLVED: they
// were the canonical AmbSoundMapHolder/AmbSoundRecord
// (<Gruntz/RandomAmbientSound.h>) - the same +0x10 ::CMapStringToPtr holder and
// +0x10 DirectSoundMgr* record the SetupFromMap path already used.)

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

#endif // GRUNTZ_BOUNDARYTAILVIEWS_H
