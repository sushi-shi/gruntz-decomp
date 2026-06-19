#include "../rva.h"
// CDDrawSubMgrFamily.cpp - the CGruntzMgr DirectDraw sub-manager family (the
// ex-"Harry Potter" codenames), consolidated into ONE file so the real
// single-inheritance hierarchy can be expressed in one place.
//
// Why one file: All.cpp #includes every stub into a SINGLE translation unit, so
// a shared base must be DEFINED EXACTLY ONCE in that TU - it can't be redeclared
// inline in each per-class stub. None of these classes carry RTTI (the leaked
// C:\Proj\DDrawMgr module was built without /GR), so the hierarchy is recovered
// from the vtable prologues, each delimited by the shared base subobject
// [FUN_1bef01, <dtor>, FUN_0028ec, FUN_00106e, FUN_004034]
// (see scripts/analysis/rtti_classify.py --vtable 0x..).
//
// Hierarchy (a method is filed under the class whose vtable INTRODUCES it):
//
//   CDDrawSubMgrBase                          base subobject (vtable slots 0..4)
//   |- CDDrawSubMgr        (was Lucius)       shared base of the leaf managers
//   |  |- CDDrawMapHolder  (was Minerva)
//   |  '- CDDrawSubMgrLeaf (was Pettigrew)
//   '- CDDrawSurfaceMgr    (was HarryPotter)  the root that owns the sub-managers
//
// SCOPE: this PR only fixes the STRUCTURE of the backlog stubs - the bodies stay
// @stub (empty, RVA-labelled), the RVA set is unchanged, so the match is
// unaffected. The methods are kept non-virtual (as before): virtualizing the
// empty stubs would make MSVC emit unmatched vtables + `??_G` thunks into this
// 0%-stub object. They become `virtual ... OVERRIDE` (the OVERRIDE macro in
// rva.h) when the bodies are actually matched and the real vtable is modeled.
//
// NOT here (other PRs): the already-matched halves stay in src/Gruntz/CDDraw*
// (e.g. CDDrawSurfaceMgr::...14 @0x155f00, CDDrawSubMgrLeaf::...14 @0x1577a0, and
// the @0x157ae0 method currently mislabelled under CDDrawSubMgrLeaf that really
// belongs to CDDrawMapHolder's vtable - it is matched, so it moves in the
// matched-migration pass). CDDrawWorkerCache (Sirius, vt@0x1efd00) is the
// multiple-inheritance case and has its own PR.

// ---------------------------------------------------------------------------
// CDDrawSubMgrBase (was UnknownCGruntzMgrHogwarts) - the family's base subobject.
class CDDrawSubMgrBase {
public:
    void Constructor_156e10();
};

// @confidence: med
// @source: call-xref
// @stub
RVA(0x156e10, 0x68)
void CDDrawSubMgrBase::Constructor_156e10() {}

// ---------------------------------------------------------------------------
// CDDrawSubMgr (was UnknownCGruntzMgrLucius) - the shared base of the leaf
// managers (vtable @0x1efc30).
class CDDrawSubMgr : public CDDrawSubMgrBase {
public:
    ~CDDrawSubMgr();
    void Constructor_157630();
    void VirtualMethodUnknown1C();
    void VirtualMethodUnknown20();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x155720, 0x1e)
CDDrawSubMgr::~CDDrawSubMgr() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x157630, 0x82)
void CDDrawSubMgr::Constructor_157630() {}

// @confidence: med
// @source: tomalla
// @stub
RVA(0x1576c0, 0x6)
void CDDrawSubMgr::VirtualMethodUnknown1C() {}

// @confidence: med
// @source: tomalla
// @stub
RVA(0x157790, 0x6)
void CDDrawSubMgr::VirtualMethodUnknown20() {}

// ---------------------------------------------------------------------------
// CDDrawMapHolder (was UnknownMinerva) - leaf manager, vtable @0x1efca0.
class CDDrawMapHolder : public CDDrawSubMgr {
public:
    void VirtualMethodUnknown14();
    ~CDDrawMapHolder();
    void ClearUnknownMap();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157530, 0x17)
void CDDrawMapHolder::VirtualMethodUnknown14() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157550, 0x1e)
CDDrawMapHolder::~CDDrawMapHolder() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157bc0, 0xa2)
void CDDrawMapHolder::ClearUnknownMap() {}

// ---------------------------------------------------------------------------
// CDDrawSubMgrLeaf (was UnknownPettigrew) - leaf manager, vtable @0x1efc78.
class CDDrawSubMgrLeaf : public CDDrawSubMgr {
public:
    void VirtualMethodUnknown1C();
    ~CDDrawSubMgrLeaf();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x152650, 0x5)
void CDDrawSubMgrLeaf::VirtualMethodUnknown1C() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1577c0, 0x1e)
CDDrawSubMgrLeaf::~CDDrawSubMgrLeaf() {}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr (was UnknownClassCGruntzMgrHarryPotter) - the root that owns
// one sub-manager per slot (vtable @0x1efc58).
class CDDrawSurfaceMgr : public CDDrawSubMgrBase {
public:
    void UnknownVirtualMethod18();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x155900, 0x519)
void CDDrawSurfaceMgr::UnknownVirtualMethod18() {}
