// CDroppedObjectShadow.cpp - the dropped-object shadow eyecandy (C:\Proj\Gruntz),
// a CUserLogic leaf. The 1-arg leaf ctor + the /GX leaf dtor are reconstructed here.
#include <Gruntz/CDroppedObjectShadow.h>
#include <Gruntz/CGameRegistry.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190) the leaf interns "A"
// into; named by mangled symbol so the Find call reloc-masks.
extern CButeTree g_buteTree;

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @0x64556c). The ctor
// reads its +0x78 sub-object's +0x28 field into m_object->m_4c. Modeled minimally with
// only the touched offsets; address-pinned so the `mov ds:g_gameReg` reloc-masks.
struct WwdGameRegSub {
    char m_pad00[0x28];
    i32 m_28; // +0x28
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CDroppedObjectShadow::CDroppedObjectShadow(CGameObject*) @0xc7490 - the 1-arg
// leaf ctor: the standard CUserLogic(obj) init (folded inline) plus the shadow
// tail - cache the anim-set node off the "A" bute key, snapshot m_38->m_1b4,
// apply the shadow sprite/geometry to the bound object, raise its logic/collision
// flag bits, and seed the bound object's render state (m_4c from the game
// registry, m_50=7/m_58=1, the 0xcf84f tile-key + its dirty bit). Constructs a
// throwing CUserBaseLink, so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical to retail (the CUserLogic init, the "A" anim-set cache, the
// ApplyName/ApplyLookupGeometry pair, the m_38->m_08 RMW, the m_10 render-state
// seed); the residue is this ctor's own __ehfuncinfo state numbering + the
// 1-slot callee-saved scheduling delta MSVC coin-flips. The SAME plateau as
// CBrickz::CBrickz (~92%); not source-steerable. Parked for the final sweep.
RVA(0x000c7490, 0x1a6)
CDroppedObjectShadow::CDroppedObjectShadow(CGameObject* obj) : CUserLogic(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->ApplyName("LEVEL_OBJECTDROPPER_SHADOW");
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_DROPPEDOBJECTSHADOW", 0);
    m_38->m_flags |= 0x2000002;
    m_object->m_drawFillArg = ((WwdGameRegSub*)g_gameReg->m_78)->m_28;
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    if (m_object->m_latchedAnimId != 0xcf84f) {
        m_object->m_latchedAnimId = 0xcf84f;
        m_object->m_flags |= 0x20000;
    }
}

// CDroppedObjectShadow::~CDroppedObjectShadow (0x12670) - the /GX leaf dtor folds
// the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-
// destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame; the
// leaf vptr store is dead-eliminated.
RVA(0x00012670, 0x44)
CDroppedObjectShadow::~CDroppedObjectShadow() {}

#include <rva.h>
SIZE_UNKNOWN(WwdGameRegSub);
