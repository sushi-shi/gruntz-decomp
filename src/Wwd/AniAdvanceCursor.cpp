// AniAdvanceCursor.cpp - the WWD animation-advance cursor sub-object (trace
// class CAniAdvanceCursor). It is the WWD sub-object class whose vtable is
// 0x5f0128 (the same table WwdFile.cpp's ReadPlaneObjects stamps into the +0x1A0
// embedded sub-object). REAL POLYMORPHIC: this TU owns ??_7CAniAdvanceCursor@@6B@
// @0x5f0128, so cl auto-stamps the vptr in the ctor.
//
// CAniAdvanceCursor is a CLoadable leaf (canonical include/Gruntz/Loadable.h): the
// three header fields m_4/m_8/m_c ARE CLoadable's m_04/m_08/m_0c, and its 9-slot
// vtable is CObject(0..4) + CWapObj IsLoaded/IsReady(5/6) + CLoadable Unload/
// GetClassId(7/8). It OVERRIDES slot 5 (IsLoaded @0x15b6a0) and slot 7 (Unload/
// Reset @0x15c2c0) and INHERITS slot 6 (IsReady @0x1c08) and slot 8 (GetClassId
// @0x154a00 = CLASSID_NONE). No destructible locals -> no /GX frame.
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>

#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : Wap::CObject

VTBL(CAniAdvanceCursor, 0x001f0128);

// cl auto-stamps the ??_7CAniAdvanceCursor vptr @+0, seeds the three CLoadable
// header fields (m_0c=owner, m_04=field04, m_08=field08) then zeroes m_10/m_14/m_18.
// 100%: deriving from the real CLoadable base reproduces retail's vptr-first store
// schedule exactly (was 97.6% as a hand-rolled standalone class).
RVA(0x0015b730, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(i32 owner, i32 field04, i32 field08)
    : CLoadable(owner, field04, field08) {
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
}
