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
    virtual ~CDDrawSubMgr() OVERRIDE; // slot 1 (SubMgrScalarDtor 0x155720)
    virtual i32 IsReady();            // slot 5 readiness predicate (declared-only, reloc-masked)
    virtual void IsValidImage();      // slot 6 @0x001c08 (shared base thunk)
    virtual void Slot07_155740();     // slot 7 @0x155740
    virtual void Slot08_154a00();     // slot 8 @0x154a00 (CResolveNode)
};
SIZE_UNKNOWN(CDDrawSubMgr);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(CDDrawSubMgr, 0x001efc30); // ??_7CDDrawSubMgr@@6B@ (9-slot CObject-derived vtable)

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGR_H
