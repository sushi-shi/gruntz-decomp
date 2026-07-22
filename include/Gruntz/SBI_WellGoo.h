#ifndef SBI_WELLGOO_H
#define SBI_WELLGOO_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_Image.h> // CSBI_Image base

class CFileMemBase;

class CImage;
class CDDSurface;
class CDDrawShadeBlit;

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Image/ImageSet.h> // CDDrawWorker == CDDrawWorker (the ONE frame-set class)
#include <Gruntz/SpriteRefTable.h>

class CSBI_WellGoo : public CSBI_Image {
public:
    // tag 7 (the Gruntz-tab WELLGOO widget).
    CSBI_WellGoo() {
        m_kind = 7;
        m_frame = 0;
    }
    // Real vtable shape (sema class: vtbl@0x1eadfc, 12 slots; overrides 0/1/2/3/4/5).
    // The out-of-line ~ (0x104bb0) lives in SBI_WellGoo.cpp via the CHAIN-DTOR device
    // (see StatusBarItem.h).
    virtual ~CSBI_WellGoo() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1eadfc thunk 0x3e90 -> 0xe64c0): the goo serialize. Round-trips the
    // fill/rect fields + two frame handles by name+index, (mode 8) re-resolves the goo
    // surface + rebinds the frames' shade nodes, then chains CSBI_Image::SerializeFields.
    virtual i32 SerializeFields(CFileMemBase* arc, i32 mode, i32 a3, i32 a4) OVERRIDE; // 0xe64c0
    virtual i32 Setup(CStatusBarMgr* owner, CDDrawSurfaceMgr* host, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10)
        OVERRIDE; // slot 2 (0xe6020; args 5..8 are ONE by-value SbRect - see StatusBarItem.h)
    virtual void Reset() OVERRIDE; // slot 3 (ex Free)
    virtual i32 Refresh(i32 a) OVERRIDE; // slot 4
    virtual i32 Render() OVERRIDE; // slot 5 - (ex Tick)

    // vtable slot 5 (0xe6380): the per-frame goo Tick. Goo state reuses the base
    // region: fillBase=m_rect14.m_4 (@0x18), fillTop=m_rect14.m_c (@0x20), countdown=m_28.

    // (0xe64c0 was declared here as a non-virtual `Serialize` - it IS the slot-1
    //  SerializeFields override declared above. Its base leg 0xe6e40 is CSBI_Image's and
    //  is reached with a QUALIFIED CSBI_Image::SerializeFields call - never re-declared
    //  here, which would emit a CSBI_WellGoo symbol that resolves nowhere.)

    // vtable slot 3 (0x104c80): release the owned goo source surface through the
    // cached manager's (+0x24) surface pool, then clear it.

    // ----- own fields (after CSBI_Image @0x34) -----
    CDDSurface* m_gooSrc;       // +0x34  goo source surface (Blit + BltEx `src`)
    CDDrawShadeBlit* m_blitter; // +0x38  owned shaded-sprite blitter (Blit `this`)
    CImage* m_fgFrame;          // +0x3c  foreground frame record (final RenderFrame `this`)
    CImage* m_baseFrame;        // +0x40  base frame record (first RenderFrame `this`)
    i32 m_fillScale;            // +0x44  fill scale factor (int, fimul); 0 => skip fill
    i32 m_drawX;                // +0x48  draw x origin
    i32 m_srcRect; // +0x4c  src-rect base (lea &m_srcRect: Blit p0/clip, BltEx srcRect)
    char m_pad50[0x54 - 0x50];
    i32 m_drawGuard; // +0x54  draw-depth guard counter (inc around BltEx, dec after)
    i32 m_blitGuard; // +0x58  draw-depth guard counter (inc around BltEx, dec after)
    i32 m_dstRect;   // +0x5c  dest-rect base (lea &m_dstRect: BltEx dstRect)
    i32 m_fgTop;     // +0x60  ftol(fillTop - clampedFill); foreground y top (m_fgTop - 2)
};
SIZE_UNKNOWN();

#endif // SBI_WELLGOO_H
