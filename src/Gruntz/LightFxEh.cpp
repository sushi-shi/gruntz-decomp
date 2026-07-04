// LightFxEh.cpp - the /GX EH-framed CLightFx method(s), split off the frameless
// clightfx TU (C:\Proj\Gruntz). MSVC5's /GX frames the leaf dtor (its destructible
// +0x18 CUserLogic link forces the EH frame), so it cannot share the base TU's
// frameless flags without re-framing its 100% leaves. The split is matching-neutral
// (each function is RVA-keyed); see split-tu-eh-dtor-vs-frameless-cstring.md.
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicTypeTableInline.h> // unrolled built-in logic-type registration

// CLightFx::~CLightFx (0x12430) - the /GX leaf dtor. CLightFx adds no destructible
// members beyond CUserLogic and shares its vtable, so the most-derived vptr store
// is dead-eliminated and the dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr
// call 0x16d2a0), store the CUserBase vptr (0x5e70b4). Byte-identical in shape to
// the established leaf-dtor archetype.
RVA(0x00012430, 0x44)
CLightFx::~CLightFx() {}

// CLightFx::CLightFx (0x9cf00) - the /GX EH-framed ctor (the EngStr temp the shared
// CUserLogic(obj) prologue builds forces the frame). This TU inlines the built-in
// logic-type registration (the "unrolled" prologue); the per-class tail seeds the
// leaf anchors (m_anchorA = 2, m_anchorB = 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0009cf00, 0x1a5)
CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj) {
    m_anchorA = 2;
    m_anchorB = 1;
}
