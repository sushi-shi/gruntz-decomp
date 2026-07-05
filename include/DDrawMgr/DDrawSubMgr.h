#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGR_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGR_H

// DDrawSubMgr.h - the polymorphic child-sub-manager base the CDDrawSurfaceMgr owner
// stores in its generically-typed slots (m_workerList/m_surfaceDesc/m_workerCache/
// m_workerMap/m_leaf, +0x0c..+0x18/+0x2c). Every child in retail carries the shared
// Wap::CObject grand-base 5-slot interface (scalar-deleting dtor at slot 1: the owner
// teardown 0x155e20 does `push 1; call [eax+4]` per child) plus a readiness predicate
// at slot 5 (`call [eax+0x14]`, dispatched by the owner's Init validate phase).
//
// PROVENANCE: this replaces the former DDrawSurfaceMgr.cpp-local 6-slot view (same
// slot layout, now derived from the real grand-base instead of re-declaring its
// slots). The concrete children are separate real classes (CDDrawSubMgrPages,
// CDDrawChildGroup, CDDrawSubMgrLeafScan, ... own headers); this base carries no
// proven data members past the vptr.
//
// NAME-CONFLATION FLAG: src/DDrawMgr/DDrawSubMgr.cpp locally defines a DIFFERENT
// class under the same CDDrawSubMgr name (CLoadable-family, vtable 0x5efc30, ctor
// 0x156cb0) - that one is the CDDrawSubMgrPages-side base ("~CDDrawSubMgrDraco"
// family) and needs an identity split/rename; see the view-burndown report.

#include <Ints.h>
#include <Wap32/Object.h> // the 5-slot grand-base (vtable 0x5e8cb4 family)
#include <rva.h>

class CDDrawSubMgr : public Wap::CObject {
public:
    virtual i32 IsReady(); // [5] readiness predicate (declared-only, reloc-masked)
};
SIZE_UNKNOWN(CDDrawSubMgr);

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGR_H
