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
class CDDrawSurfaceMgr;

class CDDrawWorker : public CWapObj {
public:
    CDDrawWorker(CDDrawSurfaceMgr* owner, i32 id) : CWapObj(owner, id, 0, CWapObj::NO_SEED) {
        m_minIndex = 99999;
        m_maxIndex = 0;
    }
    virtual ~CDDrawWorker() OVERRIDE;

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
    virtual i32 ValidateFramesFromArchive(CRezDir* tab);

    virtual i32 ReloadFrame(CRezItm* rec, i32 n, i32 flag);

    i32 SetAllTypes(ShadeMode type);
    i32 SetAllFormats(CShadeTable* shade);
    i32 SetAllLightLevels(i32 value);
    ShadeMode GetFirstFrameState();
    i32 GetFirstFrameLightLevel();
    i32 GetMemoryUsage(i32 raw);
    i32 FindFrame(CImage* frame, char* outName, i32* outIndex);

    CImage* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return NULL;
        }

        return FrameAtUnchecked(index);
    }

    inline b32 ContainsFrame(i32 index) const {
        return index >= m_minIndex && index <= m_maxIndex;
    }

    inline CImage* FrameAtUnchecked(i32 index) {
        return static_cast<CImage*>(m_items.GetAt(index));
    }

    CImage* GetFrame(i32 n);

    CDDrawSurfaceMgr* Owner() const {
        return OwnerMgr();
    }

    RVA(0x001521c0, 0x2b)
    inline void AddFrameAt(CObject* elem, i32 index) {
        m_items.SetAtGrow(index, elem);
        if (index < m_minIndex) {
            m_minIndex = index;
        }
        if (index > m_maxIndex) {
            m_maxIndex = index;
        }
    }

    CObArray m_items;
    char m_name[0x40];

    i32 m_minIndex;
    i32 m_maxIndex;
};

#endif // GRUNTZ_CDDRAWWORKER_H
