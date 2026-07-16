#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H

// DDrawSubMgrLeaf.h - CDDrawSubMgrLeaf, the CObject-derived string-keyed catalog
// of the DDraw surface-manager family (primary vftable @0x1efc78 / VA 0x5efc78),
// hoisted from DDrawSubMgrLeaf.cpp for the wave4-L original-TU partition: the
// catalog meat (0x152640-0x152d30) lives in the S2 obj (DDrawSubMgrLeaf.cpp), the
// IsReady/dtor quartet + ClearContext/ClearMap (0x1577a0-0x157bc0) in the G
// (submgr-family) obj (DDrawSubMgr.cpp).
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (CObject-derived)  +0x04 m_04 (-1 inactive)  +0x08 m_08
//   +0x0c  m_0c (parent/root)      +0x10 m_10 (CMapStringToPtr, keyed by name)

#include <Ints.h>
// THE +0x10 MAP IS CMapStringToPtr, NOT CMapStringToOb (mfc_class --audit, 2026-07-12):
// every map rva retail calls from these methods - Lookup 0x1b8438, RemoveKey 0x1b84de,
// GetNextAssoc 0x1b8546, ~map 0x1b8322 - lies in [0x1b8247, 0x1b85b1), the band whose
// ctor stamps ??_7CMapStringToPtr@@6B@ (0x1eb014).  CMapStringToOb's band is
// [0x1b7e17, 0x1b8247) (Lookup 0x1b8008) and NOTHING here enters it.  The two classes
// are byte-identical, so the FID rows are all AMBIG and the tree had guessed wrong;
// `python -m gruntz.analysis.mfc_class 0x1b8438` asks the binary.  (A sibling map -
// CDDrawWorkerRegistry::m_map - really IS CMapStringToOb, so this is per-site.)
#include <Mfc.h> // real MFC CObject / CMapStringToPtr / CString / POSITION
#include <rva.h>
#include <Wap32/Object.h>

// The looked-up catalog value: only the scalar-deleting destructor slot (+0x04)
// is load-bearing. Declarations only - never defined, so no ??_7 is emitted.
class CCatalogNode {
public:
    virtual void GetRuntimeClass(); // [0] 0x1bef01 (shared thunk, declared-only)
    virtual ~CCatalogNode();        // slot 1 (deleting dtor -> cl-emitted ??_G)
};
SIZE_UNKNOWN(CCatalogNode);

// CDDrawSubMgrGrandBase - the CObject-like family grand-base (vptr + the three
// header fields +0x04..+0x0c). Real polymorphic base (its 5-slot vtable is the
// shared g_wapObjectDtorVtbl @0x5e8cb4) so cl emits the implicit grand-base vptr
// re-stamp at the leaf dtor's tail. NAME-AUDIT: maps to RTTI CObject @0x1e8cb4 but
// KEPT as a real intermediate - it carries the m_04/m_08/m_0c header past the bare
// vptr. Do not rename to CObject (would ODR-clash + collapse the /GX teardown level).
class CDDrawSubMgrGrandBase : public CObject {
public:
    virtual ~CDDrawSubMgrGrandBase() OVERRIDE; // [1] real teardown dtor

    i32 m_04; // +0x04  -1 when inactive
    i32 m_08; // +0x08
    // +0x0c  the owning CDDrawSurfaceMgr (the world/display root). Typed (was an
    // i32 "parent/root handle"): the ANI factory reads its +0x28 m_soundRegistry as the
    // element Configure ctx, the same owner every family sibling binds at +0x0c.
    class CDDrawSurfaceMgr* m_0c;
    CDDrawSubMgrGrandBase() {}
};
inline CDDrawSubMgrGrandBase::~CDDrawSubMgrGrandBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}
SIZE_UNKNOWN(CDDrawSubMgrGrandBase);

class CDDrawSubMgrLeaf : public CDDrawSubMgrGrandBase {
public:
    // The leaf vtable (??_7CDDrawSubMgrLeaf @0x5efc78) is 9 slots: 5 shared CObject
    // slots from CDDrawSubMgrGrandBase (slot 1 overridden below), then 4 leaf
    // virtuals at slots 5..8 in declaration order.
    virtual i32 IsReady();        // [5] 0x1577a0 (G obj)
    virtual i32 Slot06_152640();  // [6] 0x152640 (S2 obj)
    virtual void Cleanup();       // [7] 0x152650 (S2 obj; tail-calls FreeAll)
    virtual void Slot08_154a00(); // [8] 0x154a00 (shared, declared-only)

    // Non-vtable members.
    // (ClearContext @0x157ae0 belongs to the sibling CDDrawSubMgrLeafScan - it is that
    // class's slot-7 virtual; it operated on a LeafScan `this`, so it was re-homed there.)
    CObject* LookupValue_06b2a0(const char* key);
    void RemoveValue_152660(CCatalogNode* target);
    void FreeAll_152720();
    i32 RemoveKeysEqual_1527d0(const char* base, const char* str);
    i32 HasKeyPrefix_152c50(const char* str);
    CString KeyOfValue_152d30(CObject* target);
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
    // ScanTree_152ad0 is the ANIZ tree installer (recurses CSymTab scopes and
    // caches each 'ANI'-tagged record via CreateAniEntry_1528d0).
    class CAniElement* CreateAniEntry_1528d0(const char* key, void* entry);
    class CAniElement* CreateAniEntry2_1529b0(const char* key, void* entry);
    i32 ScanTree_152ad0(class CSymTab* tree, const char* prefix, const char* suffix);

    CMapStringToPtr m_10; // +0x10  m_map
};

SIZE_UNKNOWN(CDDrawSubMgrLeaf);
VTBL(CDDrawSubMgrLeaf, 0x001efc78); // ??_7CDDrawSubMgrLeaf (was g_catalogVtbl)

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
