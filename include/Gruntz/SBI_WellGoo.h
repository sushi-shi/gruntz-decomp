#ifndef SBI_WELLGOO_H
#define SBI_WELLGOO_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_Image.h> // CSBI_Image base
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Image/ImageSet.h> // CDDrawWorker == CDDrawWorker (the ONE frame-set class)
#include <Gruntz/SpriteRefTable.h>

class CFileMemBase;

class CImage;
class CDDSurface;
class CDDrawShadeBlit;

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
    virtual i32 SerializeFields(CFileMemBase* arc, i32 mode, i32 typeId, i32 pObj)
        OVERRIDE; // 0xe64c0
    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
        RECT rc,
        const char* key,
        i32 fillScale // read by THIS override (the definition names it so)
    ) OVERRIDE;       // slot 2 (0xe6020; args 5..8 are ONE by-value RECT - see StatusBarItem.h)
    virtual void Reset() OVERRIDE;       // slot 3 (ex Free)
    virtual i32 Refresh(i32 a) OVERRIDE; // slot 4
    virtual i32 Render() OVERRIDE;       // slot 5 - (ex Tick)

    // vtable slot 5 (0xe6380): the per-frame goo Tick. Goo state reuses the base
    // region: fillBase=m_rect14.top (@0x18), fillTop=m_rect14.bottom (@0x20), countdown=m_28.

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
    // The layout contradiction the old @identity-TODO parked here is SETTLED (2026-07-29,
    // by reconstructing Setup @0xe6020): +0x4c and +0x5c are TWO REAL 16-byte RECTs.
    //   * SerializeFields @0xe64c0 round-trips each with a single `Write(&r, 0x10)`;
    //   * Setup fills m_srcRect with ONE ::SetRect(&rc, 0, 0, m_frame->m_width - 1,
    //     m_frame->m_height - 1) followed by a four-dword struct copy, and seeds
    //     m_dstRect's .left/.right/.bottom from the widget rect (+1 on the far edges).
    // So the "m_drawGuard"/"m_blitGuard" counters that used to sit at +0x54/+0x58 were
    // m_srcRect's .right/.bottom all along, and Render's inc-before-BltEx / dec-after
    // pair is the INCLUSIVE->EXCLUSIVE rect fixup DirectDraw wants - not a re-entrancy
    // guard. Likewise the ex "m_fgTop" at +0x60 is m_dstRect.top, the goo water line.
    RECT m_srcRect; // +0x4c  source rect (SetRect-inclusive; Render widens it for the blit)
    RECT m_dstRect; // +0x5c  destination rect; Setup seeds .left/.right/.bottom, Render .top
};
SIZE_UNKNOWN();

#endif // SBI_WELLGOO_H
