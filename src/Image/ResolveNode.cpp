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
    m_20 = static_cast<i32>(0x80000000);
    m_38 = -1;
    m_5c = static_cast<i32>(0x80000000);
    m_64 = static_cast<i32>(0x80000000);
    m_3c = 0;
    m_40 = 0;
}

// (~CResolveNode is defined out-of-line in WwdFactoryObject.cpp @0x154a50 - the
// family-dtor TU, so the wide-object dtors fold its content; this pocket's ??_G
// emits retail's `call 0x154a50` against the extern.)
