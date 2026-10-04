#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H

#include <rva.h>

#include <DDrawMgr/DDrawPlacedWorker.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Gruntz/ObList.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

class CImageSet;

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
class CTransientDrawList : public CWapObj {
public:
    CTransientDrawList(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {}

    virtual ~CTransientDrawList() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual i32 IsReady() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual CTransientPixel* AddPixel(i32 x, i32 y, i32 pixelValue);
    virtual CTransientImage*
    AddImage(i32 x, i32 y, const char* imageSetName, i32 frameIndex, i32 addHead);
    virtual CTransientImage*
    AddImage(i32 x, i32 y, CImageSet* imageSet, i32 frameIndex, i32 addHead);
    virtual CTransientImage* AddImage(i32 x, i32 y, CImage* image, i32 addHead);

    virtual void RenderAndPrune(CRenderBuffer* backBuffer, CRenderBuffer* overlay);

    void Clear();

    CObList m_items;
};

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERLIST_H
