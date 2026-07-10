// WarpStonePad.cpp - the warp-stone pad tile-logic game-object (C:\Proj\Gruntz).
//
// CWarpStonePad : CUserLogic (RTTI game class, vtable 0x5e71ac). The whole class
// band, defined in ascending retail-RVA order: the ctor (0x10d650, the vtable-
// emission anchor for GetTypeTag @0x10f00 + ??_7CWarpStonePad), its activation-
// registry methods, the slot-1 SerializeMove and the /GX leaf dtor. Re-homed out of
// the UserLogic god-TU. Only offsets / code bytes are load-bearing.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/WarpStonePad.h>
#include <Gruntz/GameRegistry.h> // g_gameReg->m_134 (play sub-mode gate in the ctor)
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class). FireWarp invokes it __thiscall on the trigger.
typedef i32 (CWarpStonePad::*WarpStonePadHandler)();
struct CWarpStonePadActEntry {
    WarpStonePadHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x64e6a0), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). CWarpStonePadActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype; keeps its placeholder name so
// the DATA-pinned global symbol is unchanged.
struct CWarpStonePadActReg : public CActReg {};
DATA(0x0024e6a0)
extern CWarpStonePadActReg g_warpStonePadActReg; // 0x64e6a0

// The game registry singleton the ctor polls for the play sub-mode (m_134).
// Declared extern only (wwdfile owns the 0x24556c DATA label).
extern CGameRegistry* g_gameReg;

// --- CWarpStonePad (0x10d650), vptr 0x5e71ac --- the ctor anchors GetTypeTag @0x10f00
// + the ??_7CWarpStonePad vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x0010d650, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    if (g_gameReg->m_134 == 1) {
        m_38->m_stateFlags |= 1;
        m_38->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// CWarpStonePad::InitActReg @0x10d840 - construct the class's activation-coordinate
// registry singleton (g_warpStonePadActReg @0x64e6a0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0010d840, 0x15)
void CWarpStonePad::InitActReg() {
    ((CZDArrayDerived*)&g_warpStonePadActReg)->Construct(2000, 2010);
}

// CWarpStonePad::FireWarp @0x10d8c0, vtable slot 4 - look the activation coordinate
// up in the class registry; if the resolved entry carries a registered handler,
// re-resolve and dispatch it __thiscall on this. Same archetype as
// CTileTrigger::FireActivation.
RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireWarp(i32 coord) {
    CWarpStonePadActEntry* e = (CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CWarpStonePadActEntry* e2 =
            (CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// CWarpStonePad::RegisterActs @0x10da20 - bind the class's per-frame handler
// (AdvanceAnim @0x10dc20) to the activation key "A" via the shared name registry.
// The SAME archetype as CSingleAnimation::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0010da20, 0x18d)
void CWarpStonePad::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(id))->m_fn =
        &CWarpStonePad::AdvanceAnim;
}

// CWarpStonePad::SerializeMove @0x10f20, vtable slot 1 - the two-chain Serialize
// archetype: chain the shared serialize helper on `this`, then (only on success) the
// +0x34 CSerialObjRef sub-object; normalize to a strict bool.
RVA(0x00010f20, 0x47)
i32 CWarpStonePad::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!SerializeChain((i32)ar, mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) != 0;
}

// CWarpStonePad::~CWarpStonePad @0x10fc0 - empty vtable-anchor dtor; folds the bare
// CUserLogic teardown (the destructible +0x18 link forces the /GX EH frame).
RVA(0x00010fc0, 0x44)
CWarpStonePad::~CWarpStonePad() {}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CWarpStonePadActEntry);
SIZE_UNKNOWN(CWarpStonePadActReg);
