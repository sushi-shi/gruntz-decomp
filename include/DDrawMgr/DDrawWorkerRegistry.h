#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <Gruntz/StateId.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

class CImageSet;
class CImageSet;

class CImage;
struct PidHeader;
class CRezDir;

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
class CImageSetRegistry : public CWapObj {
public:
    CImageSetRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual ~CImageSetRegistry() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual CImage*
    CreateBlankFrameByKey(i32 width, i32 height, const char* key, i32 index, i32 keyed);
    virtual CImage*
    CreateBlankFrameForImageSet(i32 width, i32 height, CImageSet* worker, i32 index, i32 keyed);

    virtual CImage* CreateDescriptorFrameForImageSet(
        PidHeader* desc,
        FileImageFormat mode,
        CImageSet* worker,
        i32 index,
        u32 size
    );

    virtual CImage* CreateDescriptorFrameByKey(
        PidHeader* desc,
        FileImageFormat mode,
        const char* key,
        i32 index,
        u32 size
    );

    virtual CImage*
    InsertFrameForImageSet(struct CRezItm* rec, CImageSet* worker, i32 index, i32 mode);

    virtual CImage* InsertFrameByKey(struct CRezItm* rec, const char* key, i32 index, i32 mode);

    virtual CImage* LoadFrameForImageSet(char* path, CImageSet* worker, i32 index, i32 keyed);
    virtual CImage* LoadFrameByKey(char* path, const char* key, i32 index, i32 keyed);

    virtual i32 LoadImageSetsFromDirectory(class CRezMgr* parser, const char* key);

    virtual i32 LoadImageSetsFromTree(CRezDir* tree, const char* szName, const char* szKey);

    virtual i32 ReloadImageSetsFromTree(CRezDir* tree, const char* szName, const char* szKey);

    virtual void RemoveImageSet(CImageSet* worker);
    virtual void RemoveByKey(const char* key);
    virtual void ClearImageSets();

    CMapStringToOb m_imageSetsByName;

    i32 RemoveWithPrefix(const char* prefix, const char* separator);

    i32 GetMemoryUsageByPrefix(const char* str, i32 raw);
    i32 HasWithPrefix(const char* prefix);

    i32 FindFrameIdentity(CImage* frame, char* outName, i32* outIndex);
};

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
