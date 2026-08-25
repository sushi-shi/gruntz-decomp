#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

class CAniElement;

class CDDrawSubMgrLeaf : public CWapObj {
public:
    CDDrawSubMgrLeaf(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;

    CObject* LookupValue(const char* key);
    void RemoveValue(CAniElement* target);
    void FreeAll();
    i32 RemoveWithPrefix(const char* prefix, const char* separator);
    i32 HasKeyPrefix(const char* str);
    CString KeyOfValue(CObject* target);
    virtual ~CDDrawSubMgrLeaf() OVERRIDE;

    class CAniElement* CreateAniEntry(const char* key, struct CParseSource* entry);
    class CAniElement* CreateAniEntry2(const char* key, const char* entry);
    CAniElement* AddFromSource(CParseSource* src);
    void AddEntry(CAniElement* elem, const char* key);
    i32 LoadFromTree(class CSymTab* tree, const char* prefix, const char* separator);

    CMapStringToPtr m_animations;
};

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
