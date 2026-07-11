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
//   +0x0c  m_0c (parent/root)      +0x10 m_10 (CMapStringToOb, keyed by name)

#include <Ints.h>
#include <Mfc.h> // real MFC CObject / CMapStringToOb / CString / POSITION
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
    i32 m_0c; // +0x0c  parent/root handle
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

    CMapStringToOb m_10; // +0x10  m_map
};
SIZE_UNKNOWN(CDDrawSubMgrLeaf);
VTBL(CDDrawSubMgrLeaf, 0x001efc78); // ??_7CDDrawSubMgrLeaf (was g_catalogVtbl)

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAF_H
