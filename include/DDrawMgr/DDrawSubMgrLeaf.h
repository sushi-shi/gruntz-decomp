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
    i32 RemoveKeysEqual(const char* base, const char* str);
    i32 HasKeyPrefix(const char* str);
    CString KeyOfValue(CObject* target);
    virtual ~CDDrawSubMgrLeaf() OVERRIDE;

    class CAniElement* CreateAniEntry(const char* key, void* entry);
    class CAniElement* CreateAniEntry2(const char* key, void* entry);
    i32 ScanTree(class CSymTab* tree, const char* prefix, const char* suffix);

    CMapStringToPtr m_animations;
};
SIZE(0x2c);

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
