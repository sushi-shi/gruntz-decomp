#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDSurface.h>
#include <Gruntz/StateId.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

class CDDrawWorker;
class CDDrawWorker;

class CImage;
struct PidHeader;
class CSymTab;

class CDDrawWorkerRegistry : public CWapObj {
public:
    CDDrawWorkerRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual ~CDDrawWorkerRegistry() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual CImage*
    CreateBlankFrameByKey(i32 width, i32 height, const char* key, i32 index, i32 keyed);
    virtual CImage*
    CreateBlankFrameForWorker(i32 width, i32 height, CDDrawWorker* worker, i32 index, i32 keyed);

    virtual CImage* CreateDescriptorFrameForWorker(
        PidHeader* desc,
        FileImageFormat mode,
        CDDrawWorker* worker,
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

    virtual CImage* InsertFrameForWorker(void* rec, CDDrawWorker* worker, i32 index, i32 mode);

    virtual CImage* InsertFrameByKey(void* rec, const char* key, i32 index, i32 mode);

    virtual CImage* LoadFrameForWorker(char* path, CDDrawWorker* worker, i32 index, i32 keyed);
    virtual CImage* LoadFrameByKey(char* path, const char* key, i32 index, i32 keyed);

    virtual i32 ProbeWorkerKey(class CSymParser* parser, const char* key);

    virtual i32 InstallTree(void* tree, const char* szName, const char* szKey);

    virtual i32 LoadNamespace(void* tree, const char* szName, const char* szKey);

    virtual void RemoveWorker(CDDrawWorker* worker);
    virtual void RemoveByKey(const char* key);
    virtual void MapTeardown();

    CMapStringToOb m_workersByName;

    i32 RemoveKeysEqual(const char* base, const char* str);

    i32 SumSizesEqual(const char* str, i32 raw);
    i32 HasKeyEqual(const char* str);

    i32 AnyValueMatches(CImage* frame, char* outName, i32* outIndex);

    void ReadField(i32 handle, char* tmp, i32* outZero);
};

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
