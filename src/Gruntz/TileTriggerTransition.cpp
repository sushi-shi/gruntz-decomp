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
#include <Gruntz/TileTriggerTransition.h>

#include <rva.h>

// The engine bute tree singleton (?g_buteTree@@3VCButeTree@@A, RVA 0x2bf620).
// ApplyAnimation swaps the aux's bute node for the "A" node via CButeTree::Find;
// reloc-masked DIR32 (the Stub CButeTree TU owns the DATA label).
DATA(0x002bf620)
extern CButeTree g_buteTree;

// ---------------------------------------------------------------------------
// CTileTriggerTransition - the CUserLogic leaf the state machine builds. Layout
// is plain CUserLogic (0x40) + the leaf tail; its methods are below.
// ---------------------------------------------------------------------------
class CTileTriggerTransition : public CUserLogic {
public:
    CTileTriggerTransition(CGameObject* obj); // 0x10faf0
    virtual ~CTileTriggerTransition() OVERRIDE;

    i32 GetTypeTag();                             // 0x011730
    i32 ApplyAnimation(char* sprite, char* geom); // 0x110070

    // Leaf fields: CUserLogic ends at +0x40, the leaf object is 0x54 (the size the
    // state pump's `operator new(0x54)` allocates). m_activeAnimDesc caches the
    // +0x1b4 animation descriptor (same field CGrunt's resolvers name m_activeAnimDesc).
    i32 m_activeAnimDesc;      // +0x40
    char m_pad44[0x54 - 0x44]; // +0x44..+0x53
};

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
