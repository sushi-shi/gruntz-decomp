#ifndef GRUNTZ_CDDRAWWORKER_H
#define GRUNTZ_CDDRAWWORKER_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/Loadable.h>
#include <Image/CImage.h>
#include <Ints.h>

struct PidHeader;
class CImage;

class CSymTab;
struct CParseSource;
class CDDrawSurfaceMgr;

class CDDrawWorker : public CLoadable {
public:
    CDDrawWorker(CDDrawSurfaceMgr* owner, i32 id) : CLoadable(owner, id, 0, CLoadable::NO_SEED) {
        m_minIndex = 99999;
        m_maxIndex = 0;
    }
    virtual ~CDDrawWorker() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetKey(const char* key);
    virtual i32 BuildFramesFromSymTab(CSymTab* tab);

    virtual CImage* CreateBlankFrame(i32 width, i32 height, i32 index, i32 keyed);
    virtual CImage*
    CreateDescriptorFrame(PidHeader* desc, FileImageFormat mode, i32 index, u32 size);
    virtual CImage* LoadFrame(char* path, i32 index, i32 keyed);

    virtual CImage* InsertFrame(void* rec, i32 n, i32 flag);
    virtual i32 ValidateFramesFromSymTab(CSymTab* tab);

    virtual i32 ReloadFrame(CParseSource* rec, i32 n, i32 flag);

    i32 SetAllTypes(ShadeMode type);
    i32 SetAllFormats(CShadeTable* shade);
    i32 SetAllLightLevels(i32 value);
    ShadeMode GetFirstFrameState();
    i32 GetMemoryUsage(i32 raw);
    i32 FindFrame(CImage* frame, char* outName, i32* outIndex);

    CImage* GetAt(i32 index) {
        if (index < m_minIndex || index > m_maxIndex) {
            return 0;
        }

        return static_cast<CImage*>(m_items.GetAt(index));
    }

    CImage* GetFrame(i32 n);

    CDDrawSurfaceMgr* Owner() const {
        return OwnerMgr();
    }

    void AddFrameAt(void* elem, i32 index);

    CObArray m_items;
    char m_name[0x40];

    i32 m_minIndex;
    i32 m_maxIndex;
};
SIZE(0x6c);

#endif // GRUNTZ_CDDRAWWORKER_H
