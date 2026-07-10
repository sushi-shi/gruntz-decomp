// TileTriggerSwitch.cpp - the tile-trigger-switch tile-logic game-object
// (C:\Proj\Gruntz).
//
// The whole CTileTriggerSwitch band, defined in ascending retail-RVA order: the
// ctor (0x10dc40, the vtable-emission anchor), its activation-registry methods, the
// slot-1 SerializeMove and the /GX leaf dtor. Re-homed out of the UserLogic god-TU;
// the local view is dissolved onto the canonical <Gruntz/TileTriggerSwitch.h>.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/TileTriggerSwitch.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class). FireActivation invokes it __thiscall on the trigger.
typedef i32 (CTileTriggerSwitch::*TileTriggerSwitchHandler)();
struct CTileTriggerSwitchActEntry {
    TileTriggerSwitchHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x64e798), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). CTileTriggerSwitchActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype; keeps its placeholder name so
// the DATA-pinned global symbol is unchanged.
struct CTileTriggerSwitchActReg : public CActReg {};
DATA(0x0024e798)
extern CTileTriggerSwitchActReg g_tileTriggerSwitchActReg; // 0x64e798

// --- CTileTriggerSwitch (0x10dc40), vptr 0x5e7f6c --- the ctor anchors the
// ??_7CTileTriggerSwitch vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
}

// CTileTriggerSwitch::InitActReg @0x10de20 - construct the class's activation-
// coordinate registry singleton (g_tileTriggerSwitchActReg @0x64e798) over the
// fixed range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0010de20, 0x15)
void CTileTriggerSwitch::InitActReg() {
    ((CZDArrayDerived*)&g_tileTriggerSwitchActReg)->Construct(2000, 2010);
}

// CTileTriggerSwitch::FireActivation @0x10dea0, vtable slot 4 - look the activation
// coordinate up in the class registry; if the resolved entry carries a registered
// handler PMF, re-resolve and dispatch it __thiscall on `this`. Same archetype as
// CWarpStonePad::FireWarp.
RVA(0x0010dea0, 0x102)
void CTileTriggerSwitch::FireActivation(i32 coord) {
    CTileTriggerSwitchActEntry* e =
        (CTileTriggerSwitchActEntry*)g_tileTriggerSwitchActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileTriggerSwitchActEntry* e2 =
            (CTileTriggerSwitchActEntry*)g_tileTriggerSwitchActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// CTileTriggerSwitch::RegisterActs @0x10e000 - bind the class's per-frame handler
// (AdvanceAnim @0x10e200) to the activation key "A" via the shared name registry.
// The SAME archetype as CTileTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0010e000, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
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
    ((CTileTriggerSwitchActEntry*)g_tileTriggerSwitchActReg.ResolveEntry(id))->m_fn =
        &CTileTriggerSwitch::AdvanceAnim;
}

// CTileTriggerSwitch::SerializeMove @0x11050, vtable slot 1 - the two-chain Serialize
// archetype: chain the shared serialize helper on `this`, then (only on success) the
// +0x34 CSerialObjRef sub-object; normalize to a strict bool.
RVA(0x00011050, 0x47)
i32 CTileTriggerSwitch::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!SerializeChain((i32)ar, mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) != 0;
}

// CTileTriggerSwitch::~CTileTriggerSwitch @0x110f0 - the 0x44 folded CUserLogic
// teardown: the leaf vptr store (0x5e7f6c) is dead-eliminated by the immediately-
// inlined CUserLogic base dtor (store 0x5e705c, ~EngStr on +0x18, store 0x5e70b4).
RVA(0x000110f0, 0x44)
CTileTriggerSwitch::~CTileTriggerSwitch() {}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CTileTriggerSwitchActEntry);
SIZE_UNKNOWN(CTileTriggerSwitchActReg);
