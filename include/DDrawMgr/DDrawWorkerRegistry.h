#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

#include <Mfc.h>
#include <Gruntz/StateId.h>
#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <rva.h>

class CDDrawWorker;
class CDDrawWorker;

class CImage;
struct PidHeader;
class CSymTab;

class CDDrawWorkerRegistry : public CLoadable {
public:
    CDDrawWorkerRegistry(CDDrawSurfaceMgr* owner) : CLoadable(owner) {}

    virtual ~CDDrawWorkerRegistry() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual i32 GetClassId() OVERRIDE;

    virtual CImage* DispatchKeyed2C(i32 width, i32 height, const char* key, i32 index, i32 keyed);
    virtual CImage* Forward2C(i32 width, i32 height, CDDrawWorker* worker, i32 index, i32 keyed);

    virtual CImage* Forward30(PidHeader* desc, i32 mode, CDDrawWorker* worker, i32 index, u32 size);

    virtual CImage*
    DispatchKeyed30(PidHeader* desc, i32 mode, const char* key, i32 index, u32 size);

    virtual CImage* Forward38(void* rec, CDDrawWorker* worker, i32 index, i32 mode);

    virtual CImage* DispatchKeyed38(void* rec, const char* key, i32 index, i32 mode);

    virtual CImage* Forward34(char* path, CDDrawWorker* worker, i32 index, i32 keyed);
    virtual CImage* DispatchKeyed34(char* path, const char* key, i32 index, i32 keyed);

    virtual i32 ProbeWorkerKey(class CSymParser* parser, const char* key);

    virtual i32 InstallTree(void* tree, const char* szName, const char* szKey);

    virtual i32 LoadNamespace(void* tree, const char* szName, const char* szKey);

    virtual void RemoveWorker(CDDrawWorker* worker);
    virtual void RemoveByKey(const char* key);
    virtual void MapTeardown();

    CMapStringToOb m_10map;

    i32 RemoveKeysEqual(const char* base, const char* str);

    i32 SumSizesEqual(const char* str, i32 raw);
    i32 HasKeyEqual(const char* str);

    i32 AnyValueMatches(CImage* frame, char* outName, i32* outIndex);

    void ReadField(i32 handle, char* tmp, i32* outZero);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
