// original TU: filename unknown (@identity-TODO tile-trigger logic module)
//
// waveM-mech merged the tile-trigger logic obj: the 0x10cb10-0x10fac0 .text (plus
// the 0x10f20-0x116c0 low-RVA COMDAT-pool leaves the same obj emits) is ONE original
// TU. Evidence: the CRT $E init-frag run i143-i152 interleaves warpstonepad /
// tiletriggerswitch / tiletrigger / checkpointtrigger / tiletriggertransition
// (impossible across objs at first link), and the six free /GX state pumps + the
// trigger-class bodies + their exiled leaf virtuals all sit in this contiguous band.
//
// Absorbed the ex tilelogicpump (the 6 pumps) + warpstonepad + tiletriggerswitch +
// tiletrigger + checkpointtrigger units. The exiled COMDAT leaves (each class's small
// slot-1 SerializeMove / dtor / no-arg ctor at ~0x10f20-0x116c0) move WITH their class
// bodies (they are emitted by this obj's vtable instantiation) and are laid out first
// here so the file stays strictly RVA-ascending (leaf pool ~0x10f20-0x116c0 all sit
// below the class band ~0x10cb10-0x10fac0).
//
// BOUNDARY (left separate, frag-woven but out of the task's 5-unit scope): the
// tiletriggertransition body (StepController@0x10d150 + its 0x110110-tail),
// logicrecorddispatch (LogicDispatchB@0x10d3d0) and cbrickz (CBrickz@0x10e800) sit
// interleaved in this band; tiletriggertransition is $E-frag-proven this obj (i143/
// i150) and SHOULD fold here in a follow-up.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
#include <Gruntz/ActNameRegistry.h> // g_buteTree / s_actKeyA / g_nextActId / g_nameReg* / ActNameLookup
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h>                // CActReg archetype + CCheckpointActReg
#include <Gruntz/TileTrigger.h>           // CTileTrigger + the 3 leaves (new-sites)
#include <Gruntz/TileTriggerSwitch.h>     // CTileTriggerSwitch (new-site)
#include <Gruntz/WarpStonePad.h>          // CWarpStonePad (new-site)
#include <Gruntz/CheckpointTrigger.h>     // CCheckpointTrigger
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State + default step
#include <Gruntz/SerialObjRef.h>          // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object
#include <Gruntz/SerialArchive.h>         // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/GameRegistry.h>          // g_gameReg->m_134 (play sub-mode gate in the warp ctor)
#include <string.h>                       // memset (inlined rep stosd)
#include <rva.h>

// The activation key "B" (0x60d1bc) CTileSecretTrigger's second registration interns;
// s_actKeyA/g_buteTree/g_nextActId come from <Gruntz/ActNameRegistry.h>.
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"

// The game registry singleton the warp ctor polls for the play sub-mode (m_134).
extern CGameRegistry* g_gameReg;

// --- The per-class activation-coordinate registry singletons + handler-entry views.
// Each ActReg is the shared <Gruntz/ActReg.h> CActReg archetype (distinct instance);
// each ActEntry's first dword is the per-frame handler PMF (4-byte code ptr). -------

// CWarpStonePad's registry (@0x64e6a0, range [2000,2010]).
typedef i32 (CWarpStonePad::*WarpStonePadHandler)();
struct CWarpStonePadActEntry {
    WarpStonePadHandler m_fn;
};
struct CWarpStonePadActReg : public CActReg {};
DATA(0x0024e6a0)
extern CWarpStonePadActReg g_warpStonePadActReg; // 0x64e6a0

// CTileTriggerSwitch's registry (@0x64e798).
typedef i32 (CTileTriggerSwitch::*TileTriggerSwitchHandler)();
struct CTileTriggerSwitchActEntry {
    TileTriggerSwitchHandler m_fn;
};
struct CTileTriggerSwitchActReg : public CActReg {};
DATA(0x0024e798)
extern CTileTriggerSwitchActReg g_tileTriggerSwitchActReg; // 0x64e798

// CCheckpointTrigger's registry (@0x64e7c0; CCheckpointActReg from <Gruntz/ActReg.h>).
struct CCheckpointActEntry {
    i32 (CCheckpointTrigger::*m_fn)();
};
DATA(0x0024e7c0)
extern CCheckpointActReg g_checkpointActReg; // 0x64e7c0

// CTileTrigger / CTileSecretTrigger's registries (@0x64e810 / @0x64e7e8).
struct CLeafActReg : public CActReg {};
DATA(0x0024e810)
extern CLeafActReg g_tileTriggerActReg; // 0x64e810
DATA(0x0024e7e8)
extern CLeafActReg g_tileSecretTriggerActReg; // 0x64e7e8
typedef i32 (CTileTrigger::*TileTriggerHandler)();
struct CTileTriggerActEntry {
    TileTriggerHandler m_fn;
};
typedef i32 (CTileSecretTrigger::*TileSecretTriggerHandler)();
struct CTileSecretTriggerActEntry {
    TileSecretTriggerHandler m_fn;
};

// ---------------------------------------------------------------------------
// The per-object tile-logic state pump: read the per-object state machine
// (CTileTransitionController) at obj->m_7c and dispatch on its state id - state 0
// builds the leaf (operator new(0x54)), Activates it and installs it; the 0x1d/0x1e/
// 0x50..0x53 states dispatch to the state object's vtable slots; 0x3e8 is idle;
// anything else runs the default engine step.
// ---------------------------------------------------------------------------
#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                               \
    CTileTransitionController* ctl = (CTileTransitionController*)obj->m_7c;                        \
    switch (ctl->m_stateId) {                                                                      \
        case 0: {                                                                                  \
            ctl->m_stateId = 0x3e8;                                                                \
            LEAF* t = new LEAF(obj);                                                               \
            ((CTileTransitionState*)t)->Activate();                                                \
            ctl->m_state = (CTileTransitionState*)t;                                               \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            ctl->m_state->Vfunc2C();                                                               \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            ctl->m_state->Vfunc28();                                                               \
            break;                                                                                 \
        case 0x50:                                                                                 \
            ctl->m_state->Vfunc38();                                                               \
            break;                                                                                 \
        case 0x51:                                                                                 \
            ctl->m_state->Vfunc34();                                                               \
            break;                                                                                 \
        case 0x52:                                                                                 \
            ctl->m_state->Vfunc30();                                                               \
            break;                                                                                 \
        case 0x53:                                                                                 \
            ctl->m_state->Vfunc3C();                                                               \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            TileTransitionDefaultStep(ctl->m_state);                                               \
            break;                                                                                 \
    }                                                                                              \
    return 1;

// ===========================================================================
// Exiled COMDAT-pool leaves (each class's small slot-1 SerializeMove / no-arg ctor /
// dtor), ascending in the ~0x10f20-0x116c0 pool region - laid out first so the whole
// file stays RVA-ascending (they all sit below the 0x10cb10 class band).
// ===========================================================================

// CWarpStonePad::SerializeMove @0x10f20, vtable slot 1 - chain the shared serialize
// helper on `this`, then (only on success) the +0x34 CSerialObjRef sub-object.
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

// CTileTriggerSwitch::SerializeMove @0x11050, vtable slot 1.
RVA(0x00011050, 0x47)
i32 CTileTriggerSwitch::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!SerializeChain((i32)ar, mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) != 0;
}

// CTileTriggerSwitch::~CTileTriggerSwitch @0x110f0 - the 0x44 folded CUserLogic teardown.
RVA(0x000110f0, 0x44)
CTileTriggerSwitch::~CTileTriggerSwitch() {}

RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

// --- CTileTrigger::SerializeMove (0x111f0), vtable slot 1 --- base impl shared
// (inherited) by CGiantRock/CCoveredPowerup/CTileSecretTrigger (no leaf override).
RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!SerializeChain((i32)ar, mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) != 0;
}

// ~CTileTrigger is inline (header) so it folds into the three leaf dtors instead of
// being called; MSVC still emits one out-of-line COMDAT copy (called by its scalar-
// deleting dtor) at 0x011290, pinned by mangled name (an inline-defined dtor can't
// hang an RVA()):
// @rva-symbol: ??1CTileTrigger@@UAE@XZ 0x00011290 0x44

// ~CCheckpointTrigger @0x011480 - the bare folded CUserLogic teardown (store the
// CUserLogic vptr, inline-destruct the +0x18 link via ~EngStr, store CUserBase vptr;
// the destructible link forces the /GX EH frame).
RVA(0x00011480, 0x44)
CCheckpointTrigger::~CCheckpointTrigger() {}

// --- CTileTrigger leaf destructors (0x011540 / 0x011600 / 0x0116c0) --- the SAME
// folded CUserLogic teardown (leaf vptr store dead-eliminated).
RVA(0x00011540, 0x44)
CTileSecretTrigger::~CTileSecretTrigger() {}
RVA(0x00011600, 0x44)
CGiantRock::~CGiantRock() {}
RVA(0x000116c0, 0x44)
CCoveredPowerup::~CCoveredPowerup() {}

// ===========================================================================
// The class band, ascending in the 0x10cb10-0x10fac0 region.
// ===========================================================================

RVA(0x0010cb10, 0xf1)
i32 TileTriggerStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTrigger)}

RVA(0x0010cc50, 0xf1)
i32 TileTriggerSwitchStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerSwitch)}

RVA(0x0010cd90, 0xf1)
i32 TileSecretTriggerStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileSecretTrigger)}

RVA(0x0010ced0, 0xf1)
i32 GiantRockStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CGiantRock)}

RVA(0x0010d010, 0xf1)
i32 CoveredPowerupStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CCoveredPowerup)}

// CheckpointTriggerStep @0x10d290 - byte-identical to StepController bar the leaf TYPE
// (CCheckpointTrigger is 0x94, so `new` pushes 0x94 imm32 - the 3-byte-longer body).
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

RVA(0x0010d510, 0xf1)
i32 WarpStonePadStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CWarpStonePad)}

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

// CWarpStonePad::InitActReg @0x10d840 - construct g_warpStonePadActReg over [2000,2010].
RVA(0x0010d840, 0x15)
void CWarpStonePad::InitActReg() {
    ((CZDArrayDerived*)&g_warpStonePadActReg)->Construct(2000, 2010);
}

// CWarpStonePad::FireWarp @0x10d8c0, vtable slot 4 - resolve the coordinate; if the
// entry carries a handler PMF, re-resolve and dispatch it __thiscall on this.
RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireWarp(i32 coord) {
    CWarpStonePadActEntry* e = (CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CWarpStonePadActEntry* e2 =
            (CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// CWarpStonePad::RegisterActs @0x10da20 - bind AdvanceAnim to key "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful; residual is the slot-vs-id callee-saved register choice cascading
// into the free-loop count materialization. Deferred.
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

// --- CTileTriggerSwitch (0x10dc40), vptr 0x5e7f6c --- ctor anchors the vtable.
RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
}

// CTileTriggerSwitch::InitActReg @0x10de20 - construct g_tileTriggerSwitchActReg.
RVA(0x0010de20, 0x15)
void CTileTriggerSwitch::InitActReg() {
    ((CZDArrayDerived*)&g_tileTriggerSwitchActReg)->Construct(2000, 2010);
}

// CTileTriggerSwitch::FireActivation @0x10dea0, vtable slot 4.
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

// CTileTriggerSwitch::RegisterActs @0x10e000 - bind AdvanceAnim to key "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful; residual is the slot-vs-id callee-saved register choice. Deferred.
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

// --- CTileTrigger 1-arg (0x10e220), vptr 0x5e7f14 ---
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    m_object->m_04 = (m_object->m_164 << 8) + m_object->m_168;
}

// CTileTrigger::InitActReg (0x10e420) - construct g_tileTriggerActReg over [2000,2010].
RVA(0x0010e420, 0x15)
void CTileTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileTriggerActReg)->Construct(2000, 2010);
}

// CTileTrigger::FireActivation (0x10e4a0), vtable slot 4.
RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CTileTriggerActEntry* e = (CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileTriggerActEntry* e2 = (CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// CTileTrigger::RegisterActs (0x10e600) - bind AdvanceAnim to key "A".
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful; residual is the slot-vs-id callee-saved register choice. Deferred.
RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
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
    ((CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(id))->m_fn =
        &CTileTrigger::AdvanceAnim;
}

// CCheckpointTrigger::InitActReg @0x10ea00 - construct g_checkpointActReg over [2000,2010].
RVA(0x0010ea00, 0x15)
void CCheckpointTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_checkpointActReg)->Construct(2000, 2010);
}

// CCheckpointTrigger::FireActivation (0x10ea80), vtable slot 4 - the double-ResolveEntry
// + PMF-fire archetype (ResolveEntry has side effects, so re-run for the actual call).
RVA(0x0010ea80, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CCheckpointActEntry* e = (CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CCheckpointActEntry* e2 = (CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// CCheckpointTrigger::RegisterActs (0x10ebe0) - the register-"A"-then-bind archetype.
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md): logic + every byte faithful; only
// the regalloc/free-loop-count materialization diverges. Deferred.
RVA(0x0010ebe0, 0x18d)
void CCheckpointTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        g_buteTree.Insert(s_actKeyA, (void*)g_nextActId);
        id = g_nextActId;
        char* slot = ActNameLookup(id);
        i32 cnt = g_nameRegScratch;
        void** list = g_nameRegCurList;
        if (cnt != 0) {
            do {
                if (list != 0) {
                    ((CString*)list)->CString::~CString();
                }
                list++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CCheckpointActEntry*)g_checkpointActReg.ResolveEntry(id))->m_fn =
        &CCheckpointTrigger::Trigger;
}

// CCheckpointTrigger::CCheckpointTrigger(CGameObject*) @0x10ee20 - the 1-arg leaf ctor:
// the standard CUserLogic(obj) init plus the checkpoint tail (leaf vftable stamp, "A"
// cache, two logic bits, z-key recompute, then capture the 15-dword checkpoint state).
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful; the residue is this ctor's own __ehfuncinfo state numbering + the
// zero-register-pinning callee-saved choice (the shared CUserLogic-init wall). Deferred.
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

// CTileSecretTrigger::InitActReg (0x10f160) - construct g_tileSecretTriggerActReg.
RVA(0x0010f160, 0x15)
void CTileSecretTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileSecretTriggerActReg)->Construct(2000, 2010);
}

// CTileSecretTrigger::FireActivation (0x10f1e0), vtable slot 4.
RVA(0x0010f1e0, 0x102)
void CTileSecretTrigger::FireActivation(i32 coord) {
    CTileSecretTriggerActEntry* e =
        (CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileSecretTriggerActEntry* e2 =
            (CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// CTileSecretTrigger::RegisterActs (0x10f340) - intern "A" and "B", bind each handler.
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// handler stores match retail); residual is the slot-vs-id callee-saved register
// choice cascading into the free-loop counts. Deferred.
RVA(0x0010f340, 0x2ac)
void CTileSecretTrigger::RegisterActs() {
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
    ((CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(id))->m_fn =
        &CTileSecretTrigger::Act_10f6a0;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        id2 = g_nextActId;
        g_buteTree.Insert(s_actKeyB, (void*)id2);
        char* slot = ActNameLookup(id2);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyB);
        g_nextActId++;
    }
    ((CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(id2))->m_fn =
        &CTileSecretTrigger::Act_10f970;
}

// @early-stop
// 0x10f6a0 (565 B) = CCheckpointTrigger's per-frame "A" activation handler (homed from
// src/Stub/GapFunctions.cpp, matcher-5). Operates on the 0x94 checkpoint layout;
// ~10 callees (FindChild, CButeTree::Find, ApplyLookupGeometry, LeafCue::PlayIfElapsed,
// OnCheckpointReached, SpawnVoiceDriver + inline rand + a level sprite-ref hit-test).
// Homed pending leaf-first reconstruction (>512 B).
RVA(0x0010f6a0, 0x235)
i32 Gap_10f6a0(void) {
    return 0;
}

// CCheckpointTrigger::SerializeMove @0x10f9a0 - vtable slot 1. Read/Write the captured
// checkpoint state (the 15-dword m_state block @+0x54 + the m_firstEmpty index @+0x90)
// through the archive's mode-keyed slots, then chain the shared serialize helper + the
// +0x34 CSerialObjRef sub-object's Chain.
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

// --- The three CTileTrigger leaves' 1-arg ctors (0x10fa60/90/c0) --- each chains
// CTileTrigger(obj) then the leaf vptr auto-stamps. vptrs: CTileSecretTrigger
// 0x5e7e64, CGiantRock 0x5e7d5c, CCoveredPowerup 0x5e7e0c.
RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

SIZE_UNKNOWN(CWarpStonePadActEntry);
SIZE_UNKNOWN(CWarpStonePadActReg);
SIZE_UNKNOWN(CTileTriggerSwitchActEntry);
SIZE_UNKNOWN(CTileTriggerSwitchActReg);
SIZE_UNKNOWN(CCheckpointActEntry);
SIZE_UNKNOWN(CCheckpointActReg);
SIZE_UNKNOWN(CLeafActReg);
SIZE_UNKNOWN(CTileTriggerActEntry);
SIZE_UNKNOWN(CTileSecretTriggerActEntry);
