// CGruntStartingPoint.cpp - the grunt starting-point marker (C:\Proj\Gruntz), a
// CUserLogic leaf. The /GX leaf dtor + the 1-arg ctor (shared CUserLogic(obj)
// prologue + the per-class tail).
#include <Gruntz/CGruntStartingPoint.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CGruntStartingPoint::~CGruntStartingPoint (0x10670) - the /GX leaf dtor folds
// the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-
// destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The leaf vptr store is dead-eliminated.
RVA(0x00010670, 0x44)
CGruntStartingPoint::~CGruntStartingPoint() {}

// CGruntStartingPoint::CGruntStartingPoint (0x3df30) - name the bound object
// "GAME_EXIT", bind its "A" bute node, then flag the sub-object (+0x08 bits 1,2
// and +0x40 bit 1).
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0003df30, 0x161)
CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyName("GAME_EXIT");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 1;
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
}
