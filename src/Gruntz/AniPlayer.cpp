// AniPlayer.cpp - the timed-play SBI leaf's own obj (dossier #16, waveM-judgment):
// the [0xe5800-frag-group .. 0xe5d17] block of the per-class SBI item obj sequence
// (each class block in [0xe5ad0..0xe8733] is one original one-file-per-class TU,
// headed by its 9-frag {0,1,1} static group). The ex-CAniPlayer slot bodies moved
// to their vtable-proven owners (SBI_Image.cpp / SBI_ImageSet.cpp /
// SBI_ImageSetAni.cpp); the 0xe6020 stub to SBI_WellGoo.cpp (slot 2). Only the
// four timed-play leaf methods remain - see <Gruntz/AniPlayer.h> (@identity-TODO).
#include <Gruntz/AniPlayer.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical g_gameReg->m_world view (CDDrawSurfaceMgr + CDDrawSubMgrPages)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <Image/CImage.h>                 // the cel/frame record (RenderFrame @0x153790)
#include <Mfc.h>                          // CMapStringToPtr (the config-host lookup)
#include <rva.h>

#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton
#include <Gruntz/SbiConfig.h>    // CDDrawSurfaceMgr / CImageSet (the resolved record)


RVA(0x000e5ad0, 0x84)
i32 CAniPlayer::Start(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 a2,
    i32 a3,
    SbRect rc,
    const char* key,
    i32 b0,
    i32 b1,
    i32 b2,
    i32 b3,
    i32 b4
) {
    if (Init(owner, host, a2, a3, rc, key, b0, b1, b2, b3, b4) == 0) {
        return 0;
    }
    m_60 = m_interval;
    m_64 = 0;
    m_58 = g_frameTime;
    m_5c = 0;
    return 1;
}

// ===========================================================================
// CAniPlayer::TickToggle (0x0e5b90) - a timed frame flip: when the timed-play window
// (start clock @+0x58, i64) has elapsed against the running clock g_frameTime, flip the
// frame between the two range endpoints, restamp the window (duration = m_3c,
// start = now). Returns 1. The command param is ignored.
// @early-stop
// 92% - logic + control flow byte-exact; residual is the i64 window compare's
// register/store scheduling (m_38/duration/start restamp) - the codegen
// entropy tail shared by the timed-play family. Final sweep.
// ===========================================================================
RVA(0x000e5b90, 0x51)
i32 CAniPlayer::TickToggle(i32 param) {
    if (static_cast<__int64>(g_frameTime) - *reinterpret_cast<__int64*>(&m_58) >= *reinterpret_cast<__int64*>(&m_60)) {
        m_38 = (m_38 == m_frameStart) ? m_frameEnd : m_frameStart;
        m_60 = m_interval;
        m_64 = 0;
        m_58 = g_frameTime;
        m_5c = 0;
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::RenderCel (0x0e5c10) - the render half of Tick: resolve the current cel
// from the record table by frame (0 when out of range), record it, and - when present -
// blit it onto the active surface context at the rect base + cel offset. Returns 1.
// @early-stop
// 98.1% - logic + externs byte-exact; residual is the m_34/m_38 register
// pairing in the range test (same regalloc entropy tail as the slot-5 Ticks). Final sweep.
// ===========================================================================
RVA(0x000e5c10, 0x54)
i32 CAniPlayer::RenderCel() {
    CImageSet* tbl = m_34;
    CImage* cel;
    if (m_38 >= tbl->m_minIndex && m_38 <= tbl->m_maxIndex) {
        cel = static_cast<CImage*>(tbl->m_items.GetAt(m_38));
    } else {
        cel = 0;
    }
    m_frame = cel;
    if (cel != 0) {
        i32 surfaceCtx = reinterpret_cast<i32>(g_gameReg->m_world->m_drawTarget->m_backPair);
        cel->RenderFrame(
            reinterpret_cast<void*>(surfaceCtx),
            reinterpret_cast<void*>((cel->m_anchorX + m_rect14.m_0)),
            reinterpret_cast<void*>((cel->m_anchorY + m_rect14.m_4)),
            static_cast<void*>(0)
        );
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::Serialize (0x0e5c90) - bail on a null archive; chain the direct base
// leg (CSBI_ImageSetAni::Serialize, 0xe7cd0); then round-trip the timed-play window
// (start clock + duration, +0x58/+0x60) through the archive (mode 4 = Write @+0x30,
// mode 7 = Read @+0x2c). Returns 1.
// @early-stop
// ~88%: logic byte-correct. Residue is the base-serialize call reloc naming + minor
// field-address regalloc - the reloc-scoring artifact + regalloc family shared by
// the serialize cluster.
// ===========================================================================
RVA(0x000e5c90, 0x87)
i32 CAniPlayer::Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    // The base leg is CSBI_ImageSetAni's vtable slot-1 override, renamed Serialize ->
    // SerializeFields with the rest of the family (StatusBarItem.h). NB: this class's own
    // `Serialize` below is deliberately NOT renamed - CAniPlayer has no VTBL binding and
    // no RTTI row, so whether 0xe5c90 is CAniPlayer's own slot 1 is unproven. Naming it
    // SerializeFields would give it the base virtual's exact name+signature and silently
    // make it an override (C++ implicit virtual), claiming a slot on no evidence.
    if (CSBI_ImageSetAni::SerializeFields(static_cast<CImageSetStream*>(arc), mode, a3, a4) == 0) {
        return 0;
    }
    if (mode == 4) {
        arc->Write(&m_58, 8);
        arc->Write(&m_60, 8);
    } else if (mode == 7) {
        arc->Read(&m_58, 8);
        arc->Read(&m_60, 8);
    }
    return 1;
}
