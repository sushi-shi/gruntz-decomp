#ifndef GRUNTZ_CDDRAWWORKER_H
#define GRUNTZ_CDDRAWWORKER_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

struct PidHeader;
class CImage;

class CRezDir;
struct CRezItm;
class CGameWorld;

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
class CImageSet : public CWapObj {
public:
    CImageSet(CGameWorld* owner, i32 id) : CWapObj(owner, id, 0, CWapObj::NO_SEED) {
        m_minIndex = 99999;
        m_maxIndex = 0;
    }
    virtual ~CImageSet() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetKey(const char* key);
    virtual i32 BuildFramesFromArchive(CRezDir* tab);

    virtual CImage* CreateBlankFrame(i32 width, i32 height, i32 index, i32 keyed);
    virtual CImage*
    CreateDescriptorFrame(PidHeader* desc, FileImageFormat mode, i32 index, u32 size);
    virtual CImage* LoadFrame(char* path, i32 index, i32 keyed);

    virtual CImage* InsertFrame(struct CRezItm* rec, i32 n, i32 flag);
    virtual i32 ReloadFramesFromArchive(CRezDir* tab);

    virtual i32 ReloadFrame(CRezItm* rec, i32 n, i32 flag);

    i32 SetAllShadeModes(ShadeMode mode);
    i32 SetAllShadeTables(CShadeTable* shadeTable);
    i32 SetAllLightLevels(i32 value);
    ShadeMode GetFirstFrameShadeMode();
    i32 GetFirstFrameLightLevel();
    i32 GetMemoryUsage(i32 raw);
    i32 FindFrame(CImage* frame, char* outName, i32* outIndex);

    const char* GetName() const {
        return m_name;
    }

    i32 GetMinIndex() const {
        return m_minIndex;
    }

    i32 GetMaxIndex() const {
        return m_maxIndex;
    }

    CImage* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return NULL;
        }

        return static_cast<CImage*>(m_frames.GetAt(index));
    }

    CImage* GetFrame(i32 n);

    CGameWorld* Owner() const {
        return OwnerMgr();
    }

    void AddFrameAt(CObject* elem, i32 index);

    CObArray m_frames;
    char m_name[0x40];

    i32 m_minIndex;
    i32 m_maxIndex;
};

// Caller-shape fallbacks for sites where VC5 cannot preserve the GetAt expansion.
#define IMAGE_SET_CONTAINS_FRAME(imageSet, index)                                                  \
    imageSet->GetMinIndex() <= index && imageSet->GetMaxIndex() >= index
#define IMAGE_SET_FRAME_AT_UNCHECKED(imageSet, index)                                              \
    static_cast<CImage*>(imageSet->m_frames.GetAt(index))

#define ADD_FRAME_AT(elem, index)                                                                  \
    m_frames.SetAtGrow(index, elem);                                                               \
    if (index < m_minIndex) {                                                                      \
        m_minIndex = index;                                                                        \
    }                                                                                              \
    if (index > m_maxIndex) {                                                                      \
        m_maxIndex = index;                                                                        \
    }

#endif // GRUNTZ_CDDRAWWORKER_H
