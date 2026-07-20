#ifndef SRC_IMAGE_CIMAGE_H
#define SRC_IMAGE_CIMAGE_H

struct
    CParseSource; // folded CImageSource (struct: the class-key is MANGLING-load-bearing - PAU in the CGameLevel/CImage source-taking methods)

class CDDrawPtrCollections; // folded CImageSurfacePool

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - the abstract intermediate (slots 5/6)

class CString;   // real MFC CString (4-byte ptr); completed via <Mfc.h> in the .cpp
class CBlitInfo; // the sprite blit/draw request (esi); defined in CImageSpriteBlit.cpp

class CImageParent; // +0x0c parent (CDDrawPtrCollections); defined below

class CDDSurface;

class CDDrawShadeBlit;

class CDDrawSurfaceDesc {
public:
    char _00[0x10];
    i32* m_10; // +0x10  active surface (its +0x18 is the format)
};

typedef struct tagRECT BlitRect;

class CBlitClipOwner {
public:
    char _00[0x10];
    BlitRect m_10; // +0x10  clip RECT {left, top, right, bottom}
};

class CImageParent {
public:
    char _00[0x04];
    CDDrawSurfaceDesc* m_04; // +0x04  display-mode descriptor
    char _08[0x1c - 0x08];
    CDDrawPtrCollections* m_1c; // +0x1c  the surface pool
    char _20[0x24 - 0x20];
    CBlitClipOwner* m_24; // +0x24  clip-region owner (its +0x10 is the clip RECT)
};

class CImageFrameDesc {
public:
    char _00[0x04];
    i32 m_04; // +0x04  flag word (0x20 / 0x40)
    char _08[0x10 - 0x08];
    i32 m_10; // +0x10
    i32 m_14; // +0x14
};

extern i32 g_resourceInstallActive;
extern i32 g_surfaceColorKey;

class CResolveNode; // the shared clip/resolve singleton (RenderImage arg); defined in the .cpp

class CImage : public CWapObj {
public:
    // vptr @+0x00 (inherited from CObject via CWapObj); the base subobject
    // re-stamp (masks 0x5e8cb4) folds into ~CImage as its last store. CImage keeps
    // CWapObj's slot 5 (IsLoaded @0x0013b6) and slot 6 (IsReady @0x001c08) defaults
    // unchanged - it is the only member of the family that overrides neither.
    // @name-conflict (+0x04): this header calls it m_status ("-1 inactive"), while BOTH of
    // the hand-rolled stand-ins that WERE this class (WwdGameObject.cpp's CFrameWorker and
    // <Image/ImageFrame.h>'s CImageFrame) called it the frame index/number - and
    // CSprite::InsertFrame stores `n` (the frame index) straight into it. Kept as m_status
    // to avoid churning CImage.cpp; the index reading is better-evidenced and is a rename
    // for a follow-up. (The rest of the 0x34 layout was ALREADY complete below - the views
    // added no field knowledge this class did not have, only worse names.)
    i32 m_status;           // +0x04  status word (-1 inactive) / frame index
    i32 m_08;               // +0x08
    CImageParent* m_parent; // +0x0c  parent CDDrawPtrCollections (its surface pool at +0x1c)

    // The frame ctor (inline; the 4 construction sites - CImageSet::CreateFrame24/28/30
    // @0x151fb0/152060/152110 and CSprite::InsertFrame @0x151f00 - all build a CImage
    // with the SAME 7-field seed). Modeled as a real ctor (not spelled-out stores) so
    // cl schedules the vptr store AMONG the member inits (retail emits it 4th: after
    // m_status/m_08/m_parent, before m_width) - see
    // docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md. Member-init ORDER here
    // reproduces retail's store order.
    CImage(i32 index, CImageParent* parent) {
        m_status = index;
        m_08 = 0;
        m_parent = parent;
        m_width = 0;
        m_height = 0;
        m_surface = 0;
        m_owned = 0;
    }

    virtual ~CImage()
        OVERRIDE; // 0x0d5e80 (overrides CObject slot 1; cl stamps ??_7CImage at entry)

    // slot 5 IsLoaded (0x0d5dc0) / slot 6 IsReady (0x0d5da0): CImage inherits BOTH
    // CWapObj defaults unchanged (the only family member to do so). Kept declared
    // here as anchors (declared-only) - REMOVING them is structurally correct but
    // perturbs the /O2 regalloc of the CImage.h includers (sbi_image/aniplayer/...),
    // a heavy butterfly for no match/binding gain (the 0x0d5dc0 body is bound as
    // CWapObj::IsLoaded in the .cpp regardless). Cleanup deferred to a coordinated pass.
    virtual i32 IsLoaded() OVERRIDE; // slot 5 (== CWapObj default, body 0x0d5dc0)
    virtual i32 IsReady() OVERRIDE;  // slot 6 (== CWapObj default, body 0x0d5da0)
    virtual void FreeAll();          // slot 7  0x153260
    virtual i32 GetClassId();        // slot 8  0x0042aa -> 0x0d5de0: return 10 (class type tag)
    virtual i32 Create24(CImageFrameDesc* desc, i32 mode, i32 keyed);          // slot 9  0x1530e0
    virtual i32 LoadDispatch(CImageFrameDesc* desc, u32 mode, void* a, i32 b); // slot 10 0x152fb0
    virtual i32 Resolve(CParseSource* src, i32 arg);                           // slot 11 0x152f20
    virtual i32 Create(CImageFrameDesc* desc, i32 keyed);                      // slot 12 0x152e90
    virtual i32 Reload(CParseSource* src, i32 arg);                            // slot 13 0x153380
    virtual void RenderImage(CBlitInfo* info, CImage* dst);                    // slot 14 0x153470
    virtual void FlipVertical(void* a);                                       // slot 15 (external)
    virtual void FlipHorizontal(void* a); // slot 16 (ILT 0x002d6a; no-op sink)
    virtual void FlipBoth(void* a); // slot 17: forward arg to slots 15 then 16

    // Non-virtual members (direct-called; not in the vtable).
    i32
    BuildSlot13(CImageFrameDesc* desc, void* a); // 0x153180  (non-virtual /GX builder, external)
    i32 CopyFrom(CImage* other);                 // 0x1532b0  (clone the surface from another image)
    i32
    SetOrigin(CImageFrameDesc* desc, i32 mode); // 0x153330 (copy desc origin for mode 3/4, else 0)
    void RenderFrame(void* a, void* b, void* c, void* d);                    // 0x153790
    void RenderFrameClipped(void* a, void* b, void* c, void* d, void* rect); // 0x153810

    // The 5 sprite blit/clip routines (0x1538c0..0x1544d0): compute the on-screen
    // sprite rect (with optional X/Y flip in the anchor math + an optional m_3c
    // coordinate transform), clip it against either the parent clip RECT or the
    // worker clip box, then blit via CDDSurface::BltEx (surface variants) or
    // CDDrawShadeBlit::Blit (shaded variants). See src/Image/ImageSpriteBlit.cpp.
    void BlitNorm(CBlitInfo* info, CImage* dst);        // 0x1538c0  no flip, surface
    void BlitFlipV(CBlitInfo* info, CImage* dst);       // 0x153b20  Y flip, surface
    void BlitFlipH(CBlitInfo* info, CImage* dst);       // 0x153d90  X flip, surface
    void BlitShadeFlipHV(CBlitInfo* info, CImage* dst); // 0x153ff0  X+Y flip, shaded
    void BlitShadeNorm(CBlitInfo* info, CImage* dst);   // 0x154270  no flip, shaded
    void BlitShadeFlipV(CBlitInfo* info, CImage* dst);  // 0x1544d0  Y flip, shaded
    void BlitShadeFlipH(CBlitInfo* info, CImage* dst);  // 0x154750  X flip, shaded

    // --- layout (continues from m_parent at +0x0c) ----------------------------
    // (m_width..m_originY also serve the game-object frame-cache consumers - the ex
    // CGameObjLayer view: width/height gate the eyecandy BigActHeight test; the
    // anchors double as HALF-EXTENTS for the symmetric +-7 wander box (PathHazard/
    // CObjectDropper) and the z-sort key sy + m_anchorY (TileLogicPump/FrontCandyAni/
    // FortressFlag); the origins place the CGruntVoice speech bubble.)
    i32 m_width;              // +0x10  width  (from item->m_1c)
    i32 m_height;             // +0x14  height (from item->m_18)
    i32 m_anchorX;            // +0x18  draw anchor x (width>>1; doubles as half-width)
    i32 m_anchorY;            // +0x1c  draw anchor y (height>>1; doubles as half-height)
    i32 m_originX;            // +0x20  origin x (desc->m_10, or 0)
    i32 m_originY;            // +0x24  origin y (desc->m_14, or 0)
    i32 m_loadResult;         // +0x28  load-result code (0x10 / 0x11)
    CDDSurface* m_surface;    // +0x2c  the held DirectDraw surface (CPoolItemA), pool-removed
    CDDrawShadeBlit* m_owned; // +0x30  owned shaded sprite (teardown + RezFree)
};

#endif // SRC_IMAGE_CIMAGE_H
