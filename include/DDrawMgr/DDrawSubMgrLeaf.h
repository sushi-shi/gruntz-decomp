#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H

#include <Ints.h>
#include <Mfc.h> // real MFC CObject / CMapStringToPtr / CString / POSITION
#include <rva.h>
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject - the real leaf grand-base

// (CCatalogNode DISSOLVED: the +0x10 catalog's values ARE CAniElement entries -
// the ANI factories CreateAniEntry_1528d0/2_1529b0 populate it; the fake 2-slot
// view aliased CAniElement's CObject slots.)
class CAniElement; // <Gruntz/AniElement.h>

// CDDrawSubMgrGrandBase DISSOLVED: it was a duplicate view of CLoadable (identical
// m_04/-1 m_flags/0 m_0c/0 header + field-reset dtor + CDDrawSurfaceMgr owner @+0x0c);
// slot-8 GetClassId (@0x154a00) IS CLoadable::GetClassId, slot-7 the CLoadable::Unload
// slot. The leaf now derives from the real CLoadable; the typed owner is OwnerMgr().

class CDDrawSubMgrLeaf : public CLoadable {
public:
    // 9-slot vtable (??_7CDDrawSubMgrLeaf @0x5efc78): the CObject slots + slots 5/6/7
    // overriding CLoadable's, then slot 8 GetClassId INHERITED from CLoadable.
    virtual i32 IsLoaded() OVERRIDE; // [5] 0x1577a0 (overrides CLoadable::IsLoaded)
    virtual i32 IsReady() OVERRIDE;  // [6] 0x152640 (own return-1, overrides CWapObj::IsReady)
    virtual void Unload() OVERRIDE;  // [7] 0x152650 (overrides CLoadable::Unload; tail-jmps FreeAll)
    // slot 8 GetClassId INHERITED from CLoadable (@0x154a00 -> CLASSID_NONE); not
    // redeclared (the old "GetTypeId" was a phantom own-decl of that shared body).

    // Non-vtable members.
    // (ClearContext @0x157ae0 belongs to the sibling CDDrawSubMgrLeafScan - it is that
    // class's slot-7 virtual; it operated on a LeafScan `this`, so it was re-homed there.)
    CObject* LookupValue(const char* key);
    void RemoveValue(CAniElement* target);
    i32 FreeAll(); // i32 residue feeds the slot-7 Unload override
    i32 RemoveKeysEqual(const char* base, const char* str);
    i32 HasKeyPrefix(const char* str);
    CString KeyOfValue(CObject* target);
    virtual ~CDDrawSubMgrLeaf() OVERRIDE; // slot 1 (real ~ @0x1577e0; cl auto ??_G @0x1577c0)

    // The ANI catalog method set (the ex `CDDrawSubMgrAni` twin class, MERGED
    // 2026-07-16 - it was this class wearing a second name). Proof of identity:
    //  (1) one receiver: every dispatch site reads the SAME holder +0x2c slot
    //      (CDDrawSurfaceMgr::m_animRegistry) and calls both method sets on it -
    //      retail CCreditsState::ReleaseResources @0x38f00 does
    //      `mov ecx,[edx+0x2c]; call 0x1527d0` (a "Leaf" method) while
    //      CPlay::BuildAnizKeyTable pairs 0x152c50 probes with 0x152ad0 scans on
    //      the identical [m_c+0x2c] load;
    //  (2) one TU: 0x152640..0x152e30 is a single first-link obj weaving the two
    //      "classes" A-B-A (FreeAll 0x152720 / RemoveKeysEqual 0x1527d0 /
    //      CreateAniEntry 0x1528d0-0x1529b0 / ScanTree 0x152ad0 / HasKeyPrefix
    //      0x152c50 / KeyOfValue 0x152d30) - interleaved member defs, not two TUs;
    //  (3) identical layout (vptr + m_04/m_08/m_0c + the +0x10 CMapStringToPtr;
    //      mfc_class 0x1b847c => Ptr band for CreateAniEntry2's operator[]), and
    //      the retail image has NO Ani ctor / vtable / RTTI - nothing ever
    //      constructed a second class.
    // ScanTree is the ANIZ tree installer (recurses CSymTab scopes and
    // caches each 'ANI'-tagged record via CreateAniEntry).
    class CAniElement* CreateAniEntry(const char* key, void* entry);
    class CAniElement* CreateAniEntry2(const char* key, void* entry);
    i32 ScanTree(class CSymTab* tree, const char* prefix, const char* suffix);

    CMapStringToPtr m_10; // +0x10  m_map
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();


#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
