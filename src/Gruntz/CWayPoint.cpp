// CWayPoint.cpp - the waypoint marker (C:\Proj\Gruntz), a CUserLogic leaf. The /GX
// leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj) prologue: this TU
// inlines the built-in logic-type registration, see LogicTypeTableInline.h).
#include <Gruntz/CWayPoint.h>
#include <Gruntz/LogicTypeTableInline.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CWayPoint::~CWayPoint (0x102e0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// leaf vptr store is dead-eliminated.
RVA(0x000102e0, 0x44)
CWayPoint::~CWayPoint() {}

// CWayPoint::CWayPoint (0xae3f0) - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), then flag the sub-object (+0x40 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled built-in logic-type registration); residual
// is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_40 |= 1;
}
