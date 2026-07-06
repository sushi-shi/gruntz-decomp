// CheckpointTrigger.cpp - the checkpoint-trigger tile-logic object
// (C:\Proj\Gruntz), a CUserLogic leaf. Only the /GX leaf dtor is reconstructed.
#include <Gruntz/CheckpointTrigger.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

#include <Gruntz/ActReg.h> // shared activation-registrar archetype (CCheckpointActReg)

// ~CCheckpointTrigger @0x011480 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame; the empty body is enough for cl.
RVA(0x00011480, 0x44)
CCheckpointTrigger::~CCheckpointTrigger() {}

// The class's activation-coordinate registry singleton (@0x64e7c0), built by the
// shared registry ctor (0x408710) over the fixed [2000,2010] range. CCheckpointActReg
// is the shared <Gruntz/ActReg.h> CActReg-derived alias; only Construct is used here
// (RegisterActs, which needs its full ResolveEntry, lives in ActRegSiblings.cpp).
SIZE_UNKNOWN(CCheckpointActReg);
DATA(0x0024e7c0)
extern CCheckpointActReg g_checkpointActReg; // 0x64e7c0

// CCheckpointTrigger::InitActReg @0x10ea00 - construct the class's activation-
// coordinate registry singleton over [2000, 2010]. Free init thunk; reloc-masked.
RVA(0x0010ea00, 0x15)
void CCheckpointTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_checkpointActReg)->Construct(2000, 2010);
}

#include <Bute/ButeMgr.h> // CButeTree (g_buteTree Find)
#include <string.h>       // memset (inlined rep stosd)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

extern CButeTree g_buteTree; // ?g_buteTree@@3VCButeTree@@A @0x6bf620
DATA(0x0020a454)
extern char s_actKeyA[]; // "A"

// The bound object is the inherited CUserLogic m_10 (CGameObject*); the ctor uses
// it directly (the CTeleporter idiom - no per-TU view cast). CGameObject models the
// touched fields: +0x08 flag word, +0x198 layer (its +0x1c base offset), +0x60
// screen Y / +0x74 z-key, and the 15-dword captured checkpoint state spread over
// the bute-config blocks at +0x134..+0x160 (12) plus +0x64..+0x6c (3). Several of
// those blocks carry a 0x80000000 sentinel the ctor clamps to 0 before capturing.

// CCheckpointTrigger::CCheckpointTrigger(CGameObject*) @0x10ee20 - the 1-arg leaf
// ctor: the standard CUserLogic(obj) init (folded inline) plus the checkpoint tail
// - cl emits the implicit leaf vftable (??_7CCheckpointTrigger @0x5e7ebc) stamp,
// then cache the "A" bute node, raise the bound object's two logic bits, recompute
// its z-key from the layer base + screen Y, then capture the checkpoint state: zero
// the 15-dword block, clamp the four 0x80000000 sentinels to 0, copy the 12 config
// dwords (+0x134..+0x160) and 3 more (+0x64..+0x6c) into the leaf, and find the
// first empty slot. Constructs a throwing CUserBaseLink, so MSVC emits the /GX EH
// frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful (the CUserLogic init, the implicit leaf vptr stamp, the "A" cache,
// the two flag RMWs, the z-key recompute, the memset, the four sentinel clamps, the
// 15-dword capture, the first-empty find loop); the residue is this ctor's own
// __ehfuncinfo state numbering + the zero-register-pinning callee-saved choice (the
// shared CUserLogic-init wall). The SAME plateau as CTimeBomb / the other bute
// ctors; not source-steerable. Parked for the final sweep.
RVA(0x0010ee20, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_actKeyA);
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    i32 zk = m_object->m_layer->m_1c + m_object->m_screenY + 0x186a0;
    if (m_object->m_latchedAnimId != zk) {
        m_object->m_latchedAnimId = zk;
        m_object->m_flags |= 0x20000;
    }
    memset(m_state, 0, sizeof(m_state));
    if (m_object->m_extentL == 0x80000000) {
        m_object->m_extentL = 0;
    }
    if (m_object->m_areaL == 0x80000000) {
        m_object->m_areaL = 0;
    }
    if (m_object->m_154 == 0x80000000) {
        m_object->m_154 = 0;
    }
    if (m_object->m_64 == 0x80000000) {
        m_object->m_64 = 0;
    }
    m_state[0] = m_object->m_extentL;
    m_state[1] = m_object->m_extentT;
    m_state[2] = m_object->m_extentR;
    m_state[3] = m_object->m_extentB;
    m_state[4] = m_object->m_areaL;
    m_state[5] = m_object->m_areaT;
    m_state[6] = m_object->m_areaR;
    m_state[7] = m_object->m_areaB;
    m_state[8] = m_object->m_154;
    m_state[9] = m_object->m_158;
    m_state[10] = m_object->m_15c;
    m_state[11] = m_object->m_160;
    m_state[12] = m_object->m_64;
    m_state[13] = m_object->m_68;
    m_state[14] = m_object->m_6c;
    for (m_firstEmpty = 0; m_firstEmpty < 15; m_firstEmpty++) {
        if (m_state[m_firstEmpty] == 0) {
            break;
        }
    }
}
