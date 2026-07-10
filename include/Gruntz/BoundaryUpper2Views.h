// BoundaryUpper2Views.h - shared referent/owner views for the second-pass upper-half
// (RVA >= 0x133370) engine_boundary leaf methods reconstructed in BoundaryUpper2.cpp
// (DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules).
//
// RTTI cannot attribute these COMDAT-folded methods, so the owning class names are
// placeholders; only OFFSETS + emitted code bytes are load-bearing (campaign
// doctrine). Formerly per-TU inline views; consolidating them here is pure code
// motion (matching-neutral). Three placeholders were renamed off real class names
// they collided with (they are DIFFERENT classes): the page-table store (was the
// misleading "CDDPageMgr", the real one is the DirectDraw device mgr in
// CDirectDrawMgr.h) and the two flat volatile-tuned worker-reset views (were
// "CDDrawWorkerA/B", whose real polymorphic shape lives in CDDrawWorkerNode.h - this
// TU deliberately keeps the redundant-store `volatile` flat copy).
#ifndef GRUNTZ_BOUNDARYUPPER2VIEWS_H
#define GRUNTZ_BOUNDARYUPPER2VIEWS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Blk6c.h> // the 0x6c-byte CImageOwned transform descriptor

// (0x184b70 re-homed to src/Gruntz/DebugPrintf.cpp as RezDebugInit (CDebugConfig::
// InitFromEnv tail-forward on the g_debugConfig singleton); the CHashTail proximity
// view is dissolved.)

// (0x133370 DICfgC::DtorC re-homed to src/DinMgr2/DirectInputMgr2.cpp next to
// CInputDevRoot; the DICfgC view is dissolved.)

// (0x1396f0 CParseSource::Init re-homed to src/Gruntz/ParseSource.cpp; the HashNode1396f0
// + CParseSource views dissolved onto the real CParseSource (ParseSource.h), which now
// models the embedded hash-node + self-link and carries VTBL(HashNode1396f0, 0x1ef740).)

// (0x1570d0 / 0x157240 re-homed to src/DDrawMgr/DDrawWorkers.cpp as the real destructors
// CDDrawWorkerA::~CDDrawWorkerA / CDDrawWorkerB::~CDDrawWorkerB (CDDrawWorkerNode.h); the
// CDDWorkerFlatA/B volatile placeholder views are dissolved onto the real classes.)

// (0x17b510 / 0x17b570 / 0x17b5a0 / 0x17b840 re-homed to src/Crypto/FecCrypt.cpp as
// CFecFile::Init/Close/OnFail/Lookup; the CPageStore17b510 / DDPageSub / DDPageArr
// placeholder views were a third view of CFecFile (== CMovieDecodeStore) and are
// dissolved onto the real CFecFile / FecStream / FecIndex.)

// 0x174ed0 / 0x175780 - CImagePool free/clear pair: RE-HOMED to the imagepool unit
// (src/Image/ImagePool.cpp), reconciled onto the real CImagePool / CRezImage types.

// 0x137200 - SoundDevice::StartPrimary: RE-HOMED to the directsoundmgr unit
// (src/Dsndmgr/DirectSoundMgr.cpp, real SoundDevice in SoundDevice.h).

// (0x13e7d0 CDDSurface::Restore re-homed to src/DDrawMgr/DirectDrawMgr.cpp; the
// RestoreDesc/CDDSurface views dissolved onto the real CDDSurface (DDSurface.h) +
// the real DDBLTFX.)

// (CDDPalette / CDDrawPtrCollections factory views dissolved: Create @0x143040
// re-homed to src/DDrawMgr/DDrawPtrCollections.cpp onto the canonical classes.)

// (0x1509c0 CWwdGameObject::Test re-homed to src/Wwd/WwdGameObject.cpp onto the real
// CWwdGameObject; the WwdExtent/WwdCamRect/WwdCamHolder/WwdGridLim/WwdGridHolder/WwdCtx/
// CWwdGameObject views dissolved - the 0x1f0020 vtable is bound by the real
// VTBL(CWwdGameObjectE) in WwdGameObject.cpp, so the RELOC_VTBL alias here is redundant.)

// (0x13a530 CSymTab::AddNodeSubEntry re-homed to src/Bute/SymTab.cpp; the CHashBase/
// SymEntry1/SymEntry2/SymList18/CSymTab views dissolved onto the canonical CSymTab/
// CSymRec/CHashTable/CSymParser.)

// (0x17e230 re-homed to src/Gruntz/FaderMgr.cpp as Fader_TraceStr, taking a real MFC
// CString by value; the DBuf17e230 placeholder view is dissolved.)

// (0x143950 CDDrawPtrCollections::Make950 re-homed to DDrawPtrCollections.cpp; the
// CPalObj143950 view dissolved onto the real CDDrawPtrCollections palette fields.)

// (0x148af0 / 0x148a50 / 0x148c40 re-homed to src/DDrawMgr/DDrawPtrCollections.cpp as the
// real slot-9 virtuals CPoolItemAB8/A88/AE8::v24 (Setup/Blit7/Blit47); the ImgOwnedX/
// ImgOwnedY placeholder views are dissolved onto the real CPoolItem* surface family -
// the descriptor is the surface's own DDSURFACEDESC (m_ddsd), "Apply" is CDDSurface::
// Init1, "Commit" is InstallColorFormat.)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_BOUNDARYUPPER2VIEWS_H
