#ifndef GRUNTZ_CDDRAWSURFACEPAIR_H
#define GRUNTZ_CDDRAWSURFACEPAIR_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - the shared grand-base (slots 0..6)

// ---------------------------------------------------------------------------
// CDDrawSurfacePair - a surface-backed drawing region in the DDrawMgr image
// family (own vtable ??_7CDDrawSurfacePair@@6B@ @0x5eff30, 15 slots). It owns ONE
// held DDraw surface (a CPoolItemA, the CDDSurface wrapper) borrowed from the
// parent CDirectDrawMgr's surface pool, plus a cached pixel geometry (width @+0x10
// / height @+0x14 / bpp @+0x18) and an x/y offset window @+0x1c.
//
// THE single-source shape. It is the polymorphic surface element the pages manager
// holds at +0x10/+0x14/+0x18 (m_frontPair/m_backPair/m_overlayPair, see
// DDrawSubMgrPages.h) and dispatches through the vptr; the flat readers (LevelPreview,
// SoundFxEmitter) touch only its +0x2c channel surface. Every consumer includes
// this header instead of re-declaring a per-TU view.
//
// The 15-slot own vtable @0x5eff30 (base slots 0..6 from CObject + CWapObj):
//   slot 1  (@0x04)  ~scalar-deleting dtor  (0x1590d0/0x1590f0)
//   slot 5  (@0x14)  IsLoaded               (0x159090)  "surface ready?" predicate
//   slot 6  (@0x18)  IsReady                (0x001c08)  inherited CWapObj default
//   slot 7  (@0x1c)  TeardownSurface        (0x163e20)  remove from pool, zero m_surface
//   slot 8  (@0x20)  Slot08                 (0x1590c0)
//   slot 9  (@0x24)  SetGeometry            (0x158fd0)  {w,h,bpp} cache
//   slot 10 (@0x28)  SetGeom                (0x164250)  geometry setter (3 args)
//   slot 11 (@0x2c)  InitFromSurface        (0x163db0)
//   slot 12 (@0x30)  Create                 (0x163c90)
//   slot 13 (@0x34)  LoadImage              (0x163e50)  BMP/PCX/DIR/DIP magic dispatch
//   slot 14 (@0x38)  ResolveImage           (0x163ee0)
// Only the dtor, TeardownSurface, and Create are reconstructed (in the owner TU
// DDrawSurfacePair.cpp); the rest are declared-only virtuals so through-vptr
// dispatch in the consumer TUs lowers exactly (their bodies live in unmatched TUs,
// which is why the owner-TU ??_7 is a vtable-realization plateau, not a source lever).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine). CDDSurface / CDDrawSurfaceMgr are
// pointer members here, so a forward decl keeps the DirectDraw + surface-manager
// chains out of this widely-included header (the owner TU includes their full defs).
// ---------------------------------------------------------------------------

class CDDSurface;       // +0x2c held surface (CPoolItemA); <DDrawMgr/DDSurface.h>
class CDDrawSurfaceMgr; // +0x0c parent manager (surface pool at +0x1c)
class CParseSource;     // LoadImage_163e50 arg (the 0x139xxx byte-reader)

// ---------------------------------------------------------------------------
// CSurfacePairBase - the polymorphic CWapObj-derived base. Slots 0..4 come from
// CObject (the 5-slot grand-base thunks + scalar-deleting dtor), slot 6
// (IsReady default @0x001c08) from CWapObj; the base declares only its slot-5
// override (IsLoaded @0x159090). The base subobject dtor is EMPTY so cl folds the
// implicit grand-base re-stamp (masks 0x5e8cb4) LAST into the leaf ~CDDrawSurfacePair;
// the base-field resets (m_status/m_flags/m_mgr) move into the DERIVED dtor body so
// they precede the grand-base fold (eh-dtor-implicit-vptr-stamp-first.md sub-case 2).
// The destructible base subobject supplies the leaf dtor's /GX EH frame.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CSurfacePairBase);
class CSurfacePairBase : public CWapObj {
public:
    virtual ~CSurfacePairBase() OVERRIDE; // slot 1 (scalar-deleting dtor)
    i32 IsLoaded() OVERRIDE; // slot 5 (@0x14) 0x159090 - the "surface ready?" predicate

    // vptr @+0x00 (implicit, polymorphic)
    i32 m_status;            // +0x04  status word (-1 inactive, 0x63 active)
    i32 m_flags;             // +0x08  create flags (& 0x10000 = make-and-add path)
    CDDrawSurfaceMgr* m_mgr; // +0x0c  parent manager (its surface pool at +0x1c)
};

// Empty body -> cl emits ONLY the implicit grand-base vptr re-stamp (0x5e8cb4),
// folded into the leaf dtor as the last store.
inline CSurfacePairBase::~CSurfacePairBase() {}

SIZE(CDDrawSurfacePair, 0x34); // new-size from CDDrawSubMgrPages::CreateChildren
VTBL(CDDrawSurfacePair, 0x001eff30);
class CDDrawSurfacePair
    : public CObject { // was : CSurfacePairBase:CWapObj (merged, CWapObj slots as own)
public:
    virtual i32 IsLoaded();  // slot 5 (was CWapObj)
    virtual i32 IsReady();   // slot 6 (was CWapObj)
    i32 m_status;            // +0x04 (from merged CSurfacePairBase)
    i32 m_flags;             // +0x08
    CDDrawSurfaceMgr* m_mgr; // +0x0c (from merged CSurfacePairBase)
public:
    // The spawned-child ctor: the CreateChildren path reuses the shared base-family
    // arg-ctor (??0CDDrawSubMgr @0x156cb0, which stamps the CLoadable base vtable
    // 0x5efc30) then manually re-stamps this class's own vtable (g_ddrawSurfacePairVtbl
    // @0x5eff30). Declared-only here (the body is the shared 0x156cb0 base ctor, not
    // in this TU); consumers that spawn a pair call it via placement-new.
    CDDrawSurfacePair(i32 mgr, i32 kind, i32 a3); // 0x156cb0 (shared base ctor)

    // --- own vtable slots 7..14 (declared-only where the body lives elsewhere) ---
    virtual void TeardownSurface();                    // slot 7  (@0x1c) 0x163e20
    virtual void Slot08_1590c0();                      // slot 8  (@0x20) 0x1590c0
    virtual i32 SetGeometry_158fd0(i32 w, i32 h, i32 bpp); // slot 9 (@0x24) 0x158fd0
    virtual i32 SetGeom_164250(i32 w, i32 h, i32 bpp); // slot 10 (@0x28) 0x164250
    virtual i32 InitFromSurface_163db0(CDDSurface* src); // slot 11 (@0x2c) 0x163db0
    virtual i32 Create(i32 w, i32 h, i32 bpp, i32 a3); // slot 12 (@0x30) 0x163c90
    virtual i32 LoadImage_163e50(CParseSource* src);   // slot 13 (@0x34) 0x163e50
    virtual i32 ResolveImage_163ee0();                 // slot 14 (@0x38) 0x163ee0

    virtual ~CDDrawSurfacePair() OVERRIDE; // 0x1590f0  slot 1 (scalar-deleting dtor)

    // --- non-virtual helpers (reconstructed in the owner TU) ------------------
    void BltSelf(CDDrawSurfacePair* src); // 0x03a1d0
    i32 RestoreIfLost();                  // 0x163f00  (surface-lost retry twin of Probe)
    void DrawBox(i32* rect, i32 color);   // 0x163f40
    void DrawCross(i32 x, i32 y);         // 0x164180
    // 0x1644a0 - the DirectDraw mode-surface creator: cache {w,h,bpp}, ask the pool
    // to create the device surface (mode 0x11 if w>320 else 0x51; fullscreen bit
    // from mgr->m_capsFlags), then attach + validate it; on any failure stash a
    // 0x80e9..ed / 0xbb9 / 0xbba error in mgr->m_lastError.
    i32 directx_wrapper_caller_1644a0_DirectDrawCreate_DirectDrawEnumerateA(i32 w, i32 h, i32 bpp);
    // 0x164650 - empty dirty-rect blit hook (retail `ret 0xc` no-op): the
    // CWwdGameObjectC blit dispatch (Slot34/38) calls it per (pos,size) region on
    // the front pair. Reconstructed as an empty body so the 3-byte stub matches.
    void BlitDirtyRect_164650(CDDrawSurfacePair* other, i32* pos, i32* size);
    i32 Probe_164660(); // 0x164660  (surface-lost probe)

    // --- layout (continues the base; base ends at +0x10) ----------------------
    i32 m_width;           // +0x10  width
    i32 m_height;          // +0x14  height
    i32 m_bpp;             // +0x18  bits-per-pixel (8/16/24/32)
    i32 m_srcRect[4];      // +0x1c  x/y offset window {x,y,w,h} (src RECT)
    CDDSurface* m_surface; // +0x2c  the held surface (CPoolItemA)
    i32 m_ownsSurface;     // +0x30  "owns surface" flag (free on teardown)
};

#endif // GRUNTZ_CDDRAWSURFACEPAIR_H
