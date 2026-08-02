#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H

#include <Ints.h>
#include <Mfc.h>
#include <rva.h>
#include <Gruntz/Loadable.h>

class CAniElement;

class CDDrawSubMgrLeaf : public CLoadable {
public:
    CDDrawSubMgrLeaf(CDDrawSurfaceMgr* owner) : CLoadable(owner) {}

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
SIZE_UNKNOWN();

extern const char g_fmtPathJoin[];

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
