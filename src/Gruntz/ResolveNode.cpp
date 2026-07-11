// ResolveNode.cpp - the CResolveNode default ctor/dtor COMDAT pocket
// (0x1549d0-0x154a90, wave4-L dossier #15 block D): the pocket rides the E obj's
// contribution head (the first obj emitting ??_7WwdBResolve keeps these inline
// COMDATs + the shared base-slot bodies 0x154a00-0x154a80). The class's split
// method set lives in its retail objs: the 3-arg ctor (I obj,
// src/Wwd/WwdFactoryObject.cpp), Init (T obj, src/DDrawMgr/DDrawSurfacePair.cpp).
// Class shape: <Gruntz/ResolveNode.h>.
#include <Gruntz/ResolveNode.h>
#include <rva.h>

// Default ctor: seeds the resolution/scaling sentinels and zeroes the base
// fields, then stamps the node vftable.
RVA(0x001549d0, 0x29)
CResolveNode::CResolveNode() {
    m_0c = 0;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_5c = (i32)0x80000000;
    m_64 = (i32)0x80000000;
    m_3c = 0;
    m_40 = 0;
}

// ~CResolveNode: resets the sentinels/base fields and restamps the base-subobject
// dtor vftable. Trivial base -> no member teardown, no EH frame.
// @early-stop
// sentinel-seed store-scheduling wall (docs/patterns/sentinel-seed-ctor-store-schedule.md):
// the body is byte-identical EXCEPT the single immediate vptr restamp store.
// Retail defers it to just after the eax sentinel chain; MSVC5 here eagerly hoists
// the data-independent immediate store to position 2. No source order steers it
// (tried vptr-first/mid/last; all ~78.8%). Logic complete.
RVA(0x00154a50, 0x23)
CResolveNode::~CResolveNode() {
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    // ~CLoadable folds here: m_04=-1, m_08=0, m_0c=0, then the grand-base vptr
    // re-stamp (masks 0x5e8cb4) via ~CWapObj -> ~CObject.
}
