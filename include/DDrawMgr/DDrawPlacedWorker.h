#ifndef GRUNTZ_DDRAWMGR_DDRAWPLACEDWORKER_H
#define GRUNTZ_DDRAWMGR_DDRAWPLACEDWORKER_H

#include <rva.h>

#include <Gruntz/ResolveNode.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

class CImageSet;

class CDDrawSurfacePair;

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
class CTransientDrawItem : public CRenderState {
public:
    virtual ~CTransientDrawItem() OVERRIDE {
        m_dirty.Reset();
    }

    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetPosition(i32 x, i32 y) OVERRIDE;

    virtual void Render(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay);

    i32 m_renderPassesRemaining;

    union {
        i32 m_contentValue;
        class CImage* m_image;
        char m_pixelValue;
    };

    CTransientDrawItem() {}

    CTransientDrawItem(CDDrawSurfaceMgr* ctx) : CRenderState(NO_SEED) {
        m_id = 0;
        m_ownerCtx = ctx;
        m_flags = 0;
        m_dirty.m_rect.left = COORD_UNSET;
        m_dirty.Invalidate();
        m_screenX = COORD_UNSET;
        m_clip.left = COORD_UNSET;
        m_level = NULL;
        m_stateFlags = SPRITE_STATE_NONE;
    }
};

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
struct CTransientPixel : public CTransientDrawItem {
    virtual ~CTransientPixel() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual void Render(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) OVERRIDE;
    CTransientPixel() {}
    CTransientPixel(CDDrawSurfaceMgr* ctx) : CTransientDrawItem(ctx) {
        m_pixelValue = 0;
    }
    virtual i32 PlacePixel(i32 x, i32 y, i32 pixelValue);
};

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
struct CTransientImage : public CTransientDrawItem {
    virtual ~CTransientImage() OVERRIDE;

    virtual void Render(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) OVERRIDE;
    CTransientImage() {}
    CTransientImage(CDDrawSurfaceMgr* ctx) : CTransientDrawItem(ctx) {
        m_contentValue = 0;
    }
    virtual i32 PlaceImage(i32 x, i32 y, const char* imageSetName, i32 frameIndex);
    virtual i32 PlaceImage(i32 x, i32 y, CImageSet* imageSet, i32 frameIndex);
    virtual i32 PlaceImage(i32 x, i32 y, CImage* image);

    i32 SetImageByName(const char* imageSetName, i32 frameIndex);
};

#endif // GRUNTZ_DDRAWMGR_DDRAWPLACEDWORKER_H
