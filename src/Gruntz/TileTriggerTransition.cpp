// TileTriggerTransition.cpp - the CTileTriggerTransition tile-trigger state
// object + its aux-controller state pump (C:\Proj\Gruntz).
//
// CTileTriggerTransition is a CUserLogic leaf (vftable 0x5e7db4). Its ctor/dtor
// fold the shared CUserLogic family init (see <Gruntz/UserLogic.h>); the leaf's
// own methods + the controller's state-machine pump live here:
//
//   0x10d150  StepController  - the aux state pump (free __cdecl, /GX): builds the
//                               CTileTriggerTransition for state 0, then dispatches
//                               by state id to the state object's vtable slots.
//   0x110070  ApplyAnimation  - seed the +0x1b4 geometry/sprite then swap the aux's
//                               bute node for the "A" node.
//   0x011730  GetTypeTag      - the per-class logic-type id (0x405).
//   0x0117f0  ~CUserLogic     - the shared base destructor COMDAT the leaf folds.
//
// All engine callees are reloc-masked (no body). Functions in ascending retail-RVA
// order. The leaf's own fields are named from usage; the CUserLogic base fields
// (m_14/m_30/m_38/...) keep their m_<hexoffset> placeholders (shared base, named
// elsewhere). Only OFFSETS + emitted bytes are load-bearing.
#include <Gruntz/ActNameRegistry.h> // shared activation-name registry archetype (g_buteTree etc.)
#include <Gruntz/TileTriggerTransition.h>

#include <rva.h>

// The CTileTriggerTransition activation-coordinate registry @0x64e720 - the same
// VActLookup / CKSlimeColl coord-map archetype as CSecretActReg (see
// src/Gruntz/CSecretLevelTrigger.cpp): the [lo,hi] fast path + the slow
// Find/ActAlloc/Insert rebuild. Its 0x8710 ctor reserves the [0x7d0, 0x7da]
// coordinate range for this class. The registry object + its methods are external
// (no body) so the loads/calls reloc-mask. g_buteTree, the shared name registry,
// and the CActColl/CActColl2/ActAlloc helpers come via <Gruntz/ActNameRegistry.h>.
struct TileActReg {
    void* m_vptr;       // +0x00
    CActColl2* m_coll2; // +0x04
    i32 m_lo;           // +0x08
    i32 m_hi;           // +0x0c
    char* m_base;       // +0x10
    char* m_cur;        // +0x14
    i32 m_stride;       // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_scratch; // +0x20

    void Reserve8710(i32 lo, i32 hi); // 0x008710 (__thiscall ret 8, the shared ctor)

    // id -> handler-entry resolve (folds the VActLookup archetype): fast [lo,hi]
    // range, the slow Find path, then the ActAlloc/Insert rebuild.
    char* ResolveEntry(i32 id) {
        m_scratch = 0;
        if (id >= m_lo && id <= m_hi) {
            return m_base + (id - m_lo) * m_stride;
        }
        if (((CActColl*)this)->Find(id, 0)) {
            return m_base + (id - m_lo) * m_stride;
        }
        void* item = g_actCache;
        g_actAllocResult = (void*)ActAlloc();
        m_coll2->Insert(this, item, 0xc);
        return m_cur;
    }
};
SIZE_UNKNOWN(TileActReg);
DATA(0x0024e720)
extern TileActReg g_tileActReg;

// ---------------------------------------------------------------------------
// CTileTriggerTransition - the CUserLogic leaf the state machine builds. Layout
// is plain CUserLogic (0x40) + the leaf tail; its methods are below.
// ---------------------------------------------------------------------------
class CTileTriggerTransition : public CUserLogic {
public:
    CTileTriggerTransition(CGameObject* obj); // 0x10faf0
    virtual ~CTileTriggerTransition() OVERRIDE;

    i32 GetTypeTag();                             // 0x011730
    void Register_10fc90();                       // 0x10fc90
    void RegisterActs();                          // 0x10fe70  intern "A", bind Handler
    i32 ApplyAnimation(char* sprite, char* geom); // 0x110070
    i32 Handler_110110();                         // 0x110110  the per-frame handler bound here

    // Leaf fields: CUserLogic ends at +0x40, the leaf object is 0x54 (the size the
    // state pump's `operator new(0x54)` allocates). m_activeAnimDesc caches the
    // +0x1b4 animation descriptor (same field CGrunt's resolvers name m_activeAnimDesc).
    i32 m_activeAnimDesc;      // +0x40
    char m_pad44[0x54 - 0x44]; // +0x44..+0x53
};
SIZE_UNKNOWN(CTileTriggerTransition);

// The per-class registry entry: its first dword receives the per-frame handler PMF
// (a 4-byte code pointer on this complete single-inheritance class). RegisterActs
// stores &Handler_110110 - the retail thunk (LAB_00404129 -> 0x110110).
typedef i32 (CTileTriggerTransition::*TileActHandler)();
struct TileActEntry {
    TileActHandler m_fn;
};
SIZE_UNKNOWN(TileActEntry);

// ---------------------------------------------------------------------------
// CTileTriggerTransition::~CTileTriggerTransition - empty leaf dtor; folds the
// CUserLogic base teardown (the 0x117f0 ~CUserLogic COMDAT).
// ---------------------------------------------------------------------------
CTileTriggerTransition::~CTileTriggerTransition() {}

// ---------------------------------------------------------------------------
// GetTypeTag (0x011730) - returns the class's logic-type id.
// ---------------------------------------------------------------------------
RVA(0x00011730, 0x6)
i32 CTileTriggerTransition::GetTypeTag() {
    return 0x405;
}

// ---------------------------------------------------------------------------
// ~CUserLogic (0x0117f0) - the out-of-line base destructor COMDAT: store the
// CUserLogic vptr, tear the +0x18 link's EngStr down, store the CUserBase vptr.
// ~CTileTriggerTransition (inline) folds it into the leaf dtor, so MSVC emits one
// standalone ??1CUserLogic COMDAT (called by the leaf's scalar-deleting dtor) that
// lands at 0x117f0. An inline-defined dtor can't hang an RVA() (it would also tag
// the synthesized ??_G -> duplicate-RVA), so the COMDAT is pinned by mangled name:
// @rva-symbol: ??1CUserLogic@@UAE@XZ 0x000117f0 0x44

// ---------------------------------------------------------------------------
// CTileTriggerTransition::CTileTriggerTransition (0x10faf0) - the 1-arg leaf ctor:
// fold the shared CUserLogic(obj) init then stamp the leaf vptr (0x5e7db4) +
// the +0x1000000 object flag / +0x74 type write the original tail does.
// ---------------------------------------------------------------------------
// @early-stop
// EH-ctor vptr-restamp wall (94.9%, was already 94.9% in UserLogic.cpp pre-migration):
// the leaf vptr re-stamp lands in EH state 0 + the EH scope-cookie initializes to 8
// not 0 - see docs/patterns/eh-ctor-vptr-restamp-position.md (non-steerable EH-state
// machine ordering). Body byte-identical otherwise.
RVA(0x0010faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 0x1000000;
    if (m_10->m_74 != 0) {
        m_10->m_74 = 0;
        m_10->m_08 |= 0x20000;
    }
}

// ---------------------------------------------------------------------------
// Register_10fc90 (0x10fc90) - reserve this class's activation coordinate range
// [0x7d0, 0x7da] in the global tile-trigger activation registry.  A trivial
// forwarder (mov ecx,&reg; push hi; push lo; call); ecx (this) is unused.
// ---------------------------------------------------------------------------
RVA(0x0010fc90, 0x15)
void CTileTriggerTransition::Register_10fc90() {
    g_tileActReg.Reserve8710(0x7d0, 0x7da);
}

// ---------------------------------------------------------------------------
// RegisterActs (0x10fe70) - intern this class's activation key "A" into the shared
// bute-tree name map (assigning it a fresh id + name slot on first sight), then
// bind that id to this class's per-frame handler (Handler_110110) in the class
// registry. The SAME activation-name-intern archetype as
// CSecretLevelTrigger::RegisterActs (same 0x18d size, same global set + order);
// only the registry (g_tileActReg) and the bound handler differ.
// ---------------------------------------------------------------------------
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Handler` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the name-list free-loop count materialization -
// identical wall to CSecretLevelTrigger::RegisterActs. Deferred to the final sweep.
RVA(0x0010fe70, 0x18d)
void CTileTriggerTransition::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((TileActEntry*)g_tileActReg.ResolveEntry(id))->m_fn = &CTileTriggerTransition::Handler_110110;
}

// ---------------------------------------------------------------------------
// ApplyAnimation (0x110070) - reads the object's +0x1b4 animation descriptor,
// applies the geometry token, picks the seed frame from element[0], applies the
// sprite, then swaps the aux's bute node for the "A" node (caching the old one).
// ---------------------------------------------------------------------------
RVA(0x00110070, 0x71)
i32 CTileTriggerTransition::ApplyAnimation(char* sprite, char* geom) {
    m_activeAnimDesc = m_38->m_1b4;
    if (m_38->ApplyLookupGeometry(geom, 0) == 0) {
        return 0;
    }
    CAnimDescColl* desc = (CAnimDescColl*)m_38->m_1b4;
    CAnimElem* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    m_38->ApplyLookupSprite(sprite, elem->m_14);
    m_30 = m_14->m_1c; // save the prev anim-set node (CUserLogic base field)
    m_14->m_1c = g_buteTree.Find("A");
    return 1;
}

// ---------------------------------------------------------------------------
// StepController (0x10d150) - the aux-controller state pump. `obj` is the bound
// CGameObject; the controller lives at obj->m_7c. Dispatches on the state id:
//   0      -> build the CTileTriggerTransition, Activate it, install it
//   0x1d   -> state->Vfunc2C   0x1e -> state->Vfunc28
//   0x50   -> state->Vfunc38   0x51 -> state->Vfunc34
//   0x52   -> state->Vfunc30   0x53 -> state->Vfunc3C
//   0x3e8  -> idle             other-> the default engine step
// ---------------------------------------------------------------------------
RVA(0x0010d150, 0xf1)
i32 StepController(CGameObject* obj) {
    CTileTransitionController* ctl = (CTileTransitionController*)obj->m_7c;
    switch (ctl->m_stateId) {
        case 0: {
            ctl->m_stateId = 0x3e8;
            CTileTriggerTransition* t = new CTileTriggerTransition(obj);
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
