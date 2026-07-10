// CheckpointTrigger.cpp - the checkpoint-trigger tile-logic object
// (C:\Proj\Gruntz), a CUserLogic leaf. Only the /GX leaf dtor is reconstructed.
#include <Gruntz/CheckpointTrigger.h>
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/SerialArchive.h>         // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialObjRef.h>          // CSerialObjRef::Chain (0x8c00) on the +0x34 sub-object
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

// CheckpointTriggerStep @0x10d290 - the CCheckpointTrigger worker-pump (free __cdecl,
// /GX): the controller lives at obj->m_7c; dispatch on its state id, building the
// CCheckpointTrigger state object on state 0 and dispatching to the state object's
// vtable slots otherwise. Byte-identical to StepController @0x10d150 bar the leaf
// TYPE `new`d on state 0 (CCheckpointTrigger is 0x94, so `new` pushes 0x94 imm32 -
// the 3-byte-longer body, size 0xf4).
RVA(0x0010d290, 0xf4)
i32 CheckpointTriggerStep(CGameObject* obj) {
    CTileTransitionController* ctl = (CTileTransitionController*)obj->m_7c;
    switch (ctl->m_stateId) {
        case 0: {
            ctl->m_stateId = 0x3e8;
            CCheckpointTrigger* t = new CCheckpointTrigger(obj);
            ((CTileTransitionState*)t)->Activate();
            ctl->m_state = (CTileTransitionState*)t;
            break;
        }
        case 0x1d:
            ctl->m_state->Vfunc2C();
            break;
        case 0x1e:
            ctl->m_state->Vfunc28();
            break;
        case 0x50:
            ctl->m_state->Vfunc38();
            break;
        case 0x51:
            ctl->m_state->Vfunc34();
            break;
        case 0x52:
            ctl->m_state->Vfunc30();
            break;
        case 0x53:
            ctl->m_state->Vfunc3C();
            break;
        case 0x3e8:
            break;
        default:
            TileTransitionDefaultStep(ctl->m_state);
            break;
    }
    return 1;
}

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

// @early-stop
// 0x10f6a0 (565 B) = CCheckpointTrigger's per-frame "A" activation handler (homed from
// src/Stub/GapFunctions.cpp, matcher-5; the g_tileSecretTriggerActReg group @0x10f160..
// 0x10f970 is THIS class's - slot 1 = 0x10f9a0, slot 4 = 0x10f1e0). Operates on the 0x94
// checkpoint layout (m_state[15] @+0x54, m_firstEmpty @+0x90); ~10 callees (FindChild,
// CButeTree::Find, ApplyLookupGeometry, LeafCue::PlayIfElapsed, OnCheckpointReached,
// SpawnVoiceDriver + inline rand + a level sprite-ref hit-test). Homed pending leaf-first
// reconstruction (>512 B; needs the un-modeled CGameRegistry deep fields + FindChild ret type).
RVA(0x0010f6a0, 0x235)
i32 Gap_10f6a0(void) {
    return 0;
}

// CCheckpointTrigger::SerializeMove @0x10f9a0 - vtable slot 1. Read/Write the captured
// checkpoint state (the 15-dword m_state block @+0x54 + the m_firstEmpty index @+0x90)
// through the archive's mode-keyed slots (mode 4 = Write @+0x30, mode 7 = Read @+0x2c),
// then chain the shared CUserLogic serialize helper (SerializeChain, 0x16e7f0) and the
// +0x34 CSerialObjRef sub-object's Chain (0x8c00). Same two-chain archetype as
// CTimeBomb::SerializeMove. (The g_tileSecretTriggerActReg registry group @0x10f160..
// 0x10f970 is CCheckpointTrigger's - it holds this class's real slot-1/slot-4 bodies -
// currently mislabeled as CTileSecretTrigger in UserLogic.cpp; see the batch report.)
RVA(0x0010f9a0, 0x8f)
i32 CCheckpointTrigger::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    CSerialArchive* sa = (CSerialArchive*)arc;
    if (mode == 4) {
        sa->Write(m_state, 0x3c);
        sa->Write(&m_firstEmpty, 4);
    } else if (mode == 7) {
        sa->Read(m_state, 0x3c);
        sa->Read(&m_firstEmpty, 4);
    }
    if (!SerializeChain((i32)arc, mode, a3, a4)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain(sa, mode, a3, (CSerialObj*)a4) ? 1 : 0;
}
