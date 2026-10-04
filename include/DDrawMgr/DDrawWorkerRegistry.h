#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERREGISTRY_H

#include <map>
#include <string>

#include <Ints.h>

#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/FrameReference.h>
#include <Gruntz/StateId.h>
#include <Wap32/WapObj.h>

class CDDrawWorker;

class CImage;
struct PidHeader;
class CRezDir;

class CDDrawWorkerRegistry : public CWapObj {
public:
    CDDrawWorkerRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual ~CDDrawWorkerRegistry()  ;
    virtual i32 IsLoaded()  ;
    virtual i32 IsReady()  ;
    virtual void Unload()  ;
    virtual LoadableClassId GetClassId()  ;

    virtual CImage*
    CreateBlankFrameByKey(i32 width, i32 height, const std::string& key, i32 index, i32 keyed);
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
        const std::string& key,
        i32 index,
        u32 size
    );

    virtual CImage*
    InsertFrameForWorker(struct CRezItm* rec, CDDrawWorker* worker, i32 index, i32 mode);

    virtual CImage* InsertFrameByKey(struct CRezItm* rec, const std::string& key, i32 index, i32 mode);

    virtual CImage* LoadFrameForWorker(char* path, CDDrawWorker* worker, i32 index, i32 keyed);
    virtual CImage* LoadFrameByKey(char* path, const std::string& key, i32 index, i32 keyed);

    virtual i32 ProbeWorkerKey(class CRezMgr* parser, const std::string& key);

    virtual i32 InstallTree(CRezDir* tree, const std::string& szName, const std::string& szKey);

    virtual i32 LoadNamespace(CRezDir* tree, const std::string& szName, const std::string& szKey);

    virtual void RemoveWorker(CDDrawWorker* worker);
    virtual void RemoveByKey(const std::string& key);
    virtual void MapTeardown();

    CDDrawWorker* FindWorker(const std::string& key) const;
    const std::map<std::string, CDDrawWorker*>& Entries() const { return m_workersByName; }

    i32 RemoveWithPrefix(const std::string& prefix, const std::string& separator);

    i32 SumSizesEqual(const std::string& str, i32 raw);
    i32 HasWithPrefix(const std::string& prefix);

    FrameReference FindFrameReference(CImage* frame) const;

    void ReadField(i32 handle, char* tmp, i32* outZero);

private:
    std::map<std::string, CDDrawWorker*> m_workersByName;
};

#endif
