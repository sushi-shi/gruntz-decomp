// BoundaryUpper2.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// reconstructed (second pass over the harder remainder). These sit at class
// boundaries in GRUNTZ.EXE across the DinMgr2 / Dsndmgr / DDrawMgr / Rez engine
// modules; RTTI cannot attribute the COMDAT-folded leaf methods so the owning
// class names here are placeholders. Only OFFSETS + the code shape are
// load-bearing (campaign doctrine). Non-EH (base /O2) bodies only; the /GX
// EH-frame siblings live in BoundaryUpper2Eh.cpp. The per-use owner/referent
// views now live in <Gruntz/BoundaryUpper2Views.h> (pure code motion).
#include <Ints.h>
#include <string.h> // memset -> rep stos at /O2
#include <rva.h>
#include <Gruntz/BoundaryUpper2Views.h> // owner/referent views for this TU (pulls Blk6c.h)
#include <Globals.h>

// The engine __cdecl allocator/deallocator (operator new/delete; reloc-masked
// rel32). 0x1b9b46 / 0x1b9b82.
extern "C" void RezFree(void* p);
void* operator new(u32 n); // engine allocator @0x1b9b46 (same as RezAlloc)
inline void* operator new(u32, void* p) {
    return p;
} // placement new (construct-in-place; no allocation)

// The wap-object teardown grand-base vtable (0x5e8cb4); stamped by address
// (named elsewhere, reloc-masked).

// (0x184b70 re-homed to src/Gruntz/DebugPrintf.cpp as RezDebugInit - it is a
// CDebugConfig::InitFromEnv (0x185000) tail-forward on the g_debugConfig singleton
// (0x6bf848), NOT a hash clear; the "ClearHash_184b70" name + the CHashTail/Obj1397a0/
// CSymParser proximity views were a mis-attribution, dissolved. Byte-exact.)

// (0x133370 DICfgC::DtorC re-homed to src/DinMgr2/DirectInputMgr2.cpp next to
// CInputDevRoot - the out-of-line grand-base ~CInputDevRoot copy; kept a placeholder
// identity since CInputDevRoot's dtor is inline for the leaf dtors. The DICfgC view is
// dissolved.)

// (0x1396f0 CParseSource::Init re-homed to src/Gruntz/ParseSource.cpp onto the real
// CParseSource - the embedded +0x1c hash-node (HashNode1396f0) + self-link (+0x30) are
// now modeled in ParseSource.h (which became a struct so Init's PAU return mangling pairs
// with the CSymParser::PopParseSlot call site). The HashNode1396f0/CParseSource views
// are dissolved.)

// (0x1437e0 RelayHwnd + 0x1437f0 RestoreLostSurfaces_1437f0 re-homed to
// src/DDrawMgr/DirectDrawMgr.cpp - the g_restoreHandler install + the CDDSurface::
// RestoreLost global fallback, physically between GetDisplayMode 0x143740 and
// CreateDirectDrawVia 0x143880 in that TU. Both byte-exact.)

// (0x1570d0 CDDrawWorkerA::~CDDrawWorkerA + 0x157240 CDDrawWorkerB::~CDDrawWorkerB
// re-homed to src/DDrawMgr/DDrawWorkers.cpp as the real non-deleting destructors (their
// ??_G scalar-deleting wrappers are 0x1570b0/0x157220); the CDDWorkerFlatA/B volatile
// views are dissolved onto the real CDDrawWorkerA/B - cl now emits the g_wapObjectDtorVtbl
// (0x5e8cb4) base-subobject stamp instead of the dropped manual one. @early-stop
// redundant-store/scheduling walls kept in place.)

// (0x17b510 / 0x17b570 / 0x17b5a0 / 0x17b840 re-homed to src/Crypto/FecCrypt.cpp as
// CFecFile::Init/Close/OnFail/Lookup - the CMoviePlayer decode-store (m_540) lifecycle.
// The placeholder CPageStore17b510 was a THIRD view of CFecFile (== CMovieDecodeStore):
// same +0x124 stream, +0x138 index, interleaved in the CFecFile RVA band with
// ReadArchive 0x17b5f0 / CreateArchive 0x17b8a0. CPageStore17b510/DDPageSub/DDPageArr
// views dissolved onto CFecFile/FecStream/FecIndex. Init/OnFail byte-exact.)

// ---------------------------------------------------------------------------
// 0x174ed0 / 0x175780 - CImagePool free/clear pair: RE-HOMED to the imagepool unit
// (src/Image/ImagePool.cpp) alongside their siblings (RemovePalette 0x174f30,
// CRezImage::Free 0x175c90 / SetPalette 0x176ad0). See that TU.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 0x137200 - SoundDevice::StartPrimary: RE-HOMED to the directsoundmgr unit
// (src/Dsndmgr/DirectSoundMgr.cpp) alongside CreatePrimaryBuffer (0x137260); it is
// the real StartPrimary (SoundDevice.h), not a "Restore". See that TU.
// ---------------------------------------------------------------------------

// (0x13ba70 MakeSymSeed [was Thunk13ba70], 0x13b910 PackTag [was PackTag_13b910],
// 0x13b970 UnpackTag [was UnpackTag_13b970] re-homed to src/Bute/SymParser.cpp - the
// ButeMgr clock-seed + string<->DWORD tag helpers, declared in SymParser.h. All three
// byte-exact. Attribution: xref --tree through the ILT thunks lands on CSymTab::Find /
// CSymParser::ParseRecords/ParseBuffer.)

// (0x13e7d0 CDDSurface::Restore re-homed to src/DDrawMgr/DirectDrawMgr.cpp, alongside
// its BltEx (0x13eef0) sibling - it is a DDBLT_COLORFILL Blt on the real CDDSurface,
// already declared in DDSurface.h. RestoreDesc view -> the real DDBLTFX.)

// (CDDrawPtrCollections::Create @0x143040 re-homed to
// src/DDrawMgr/DDrawPtrCollections.cpp as a sibling of MakeB2; the CDDPalette /
// CDDrawPtrCollections placeholder views are dissolved onto the canonicals.)

// ---------------------------------------------------------------------------
// 0x1509c0 - CWwdGameObject visibility test: derive the four edges from the
// object's centre (m_centerX/m_centerY) and half-extents (m_extent->m_halfW/m_halfH),
// then bounds-check against either the camera rect (+0x40 of m_ctx->m_camera->m_5c,
// flag 0x40000 set) or the grid extents (m_ctx->m_grid->m_limits). __thiscall, 0 args.
// ---------------------------------------------------------------------------
// (0x1509c0 CWwdGameObject::Test re-homed to src/Wwd/WwdGameObject.cpp onto the real
// CWwdGameObject, next to its Dispatch/ReadState/Setup siblings - the m_ctx/m_extent
// chain now uses the real m_mgr + WwdExtent/WwdGridHolder/WwdCamHolder sub-objects
// modeled in that TU. The CWwdGameObject/WwdCtx/WwdExtent/... views are dissolved.)

// (0x163710 re-homed to src/Wwd/WwdFile.cpp as PlaneSerializeDispatch - the plane
// Save/Load (0x163780/0x1638c0) op dispatcher CGameLevel::EditDispatch tail-calls,
// physically in the CPlaneRender band. @early-stop jump-table wall preserved.)

// (0x13a530 CSymTab::AddNodeSubEntry re-homed to src/Bute/SymTab.cpp, next to its sole
// caller ApplyRange and the declaration already in SymTab.h. SymEntry1/SymEntry2/CSymTab/
// CHashBase/SymList18 views dissolved onto CSymTab/CSymRec/CHashTable/CSymParser.)

// (0x17e230 re-homed to src/Gruntz/FaderMgr.cpp as Fader_TraceStr - the CString-by-value
// trace sink CFaderMgr::Add feeds; DBuf17e230 view dissolved onto the real MFC CString.)

// (0x143950 CDDrawPtrCollections::Make950 re-homed to src/DDrawMgr/DDrawPtrCollections.cpp
// onto the real class - m_pal/m_dirty/m_tag are m_palette(+0x53c)/m_hasPalette(+0x93c)/
// m_940(+0x940). The disasm proved this RVA is the RGB-triplet palette-install (its
// caller LoadPaletteMake950 tail-returns it), NOT a separate builder. CPalObj143950
// view dissolved.)

// (0x148af0 / 0x148a50 / 0x148c40 re-homed to src/DDrawMgr/DDrawPtrCollections.cpp as the
// real slot-9 virtuals CPoolItemAB8::v24 (Setup, byte-exact), CPoolItemA88::v24 (Blit7)
// and CPoolItemAE8::v24 (Blit47) - the pool-item surface-setup family (all derive from
// CDDSurface; RTTI vtable slot +0x24). The ImgOwnedX/ImgOwnedY placeholder views are
// dissolved: the +0x10..+0x7c descriptor region is the surface's own DDSURFACEDESC
// (m_ddsd word view added to CDDSurface); the "Apply" is CDDSurface::Init1 (slot 2,
// 0x13e0a0), "Commit" is InstallColorFormat (slot 10). Slot 9 takes 4/4/7 args (the
// Create factories pushed 4/4/7, not the old 6-arg guess). Blit7/Blit47 @early-stop
// descriptor-fill scheduling walls, kept in place.)
