// GruntVoice.cpp - a grunt-voice game/sound object (a CUserLogic-derived leaf).
// Four __thiscall methods reconstructed in ascending-RVA order:
//   0x119ae0  ~CGruntVoice  (the bare CUserLogic teardown, /GX frame)
//   0x119e40  Dispatch      (per-coordinate activation registry -> handler call)
//   0x11a7e0  Setup         (seed the voice from a sample + play params)
//   0x11a870  Reset         (clear the voice, re-bind the "A" idle-anim bute)
//
// CUserLogic / CUserBase / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// the bute keys ("B" g_iconBute / "A" g_voiceKeyA), the default-icon global and
// g_buteTree from <Gruntz/InGameIcon.h> / this header. Engine callees are
// reloc-masked (no body). Confirmed CGruntVoice by its CUserLogic teardown
// shape and the FireActivation-twin dispatch (see the header).
#include <Gruntz/GruntVoice.h>
#include <Dsndmgr/StreamVoice.h>

#include <rva.h>

// ===========================================================================
// CGruntVoice::CGruntVoice(CGameObject*)  (0x1198a0)
// ===========================================================================
// The 1-arg game-object ctor: fold the shared CUserLogic(obj) init (base vptr,
// the +0x18 link, the empty-string name, the one-shot logic-type table, the three
// built-in handler registrations, the data fields) - the /GX EH frame the link's
// throwing ctor forces - then run the voice tail: zero the play/icon state, stamp
// the most-derived CGruntVoice vptr (0x5eaf6c, compiler-emitted, reloc-masked),
// register the "GAME_EXCLAMATION" name on the bound object, set the object's
// render flags (m_74 sentinel + the +0x8/+0x40 bitsets), then bind the "A" idle-anim
// bute node (stashing the previous in m_prevAnimSetNode). Returns this. __thiscall, ret 4.
//
// @early-stop
// /GX EH-state + store-scheduling wall: the CUserLogic(obj) fold, the vptr stamp, the
// ApplyName/flag/Find chain and every member offset are byte-faithful, but MSVC5
// schedules the repeated {m_54..m_64}=0 zeroing and the trailing /GX trylevel numbers
// (states 3/4 around ApplyName) differently than retail. Logic complete; the same
// store-schedule + eh-state walls the sibling Setup (0x11a7e0) carries. Final sweep.
RVA(0x001198a0, 0x195)
CGruntVoice::CGruntVoice(CGameObject* obj) : CTileLogic(obj) {
    m_icon = 0;
    m_5c = 0;
    m_durationMs = 0;
    m_64 = 0;
    m_38->ApplyName("GAME_EXCLAMATION");
    if (m_object->m_latchedAnimId != 0xdbba1) {
        m_object->m_latchedAnimId = 0xdbba1;
        m_object->m_flags |= 0x20000;
    }
    m_sample = 0;
    m_icon = 0;
    m_durationMs = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->m_flags |= 0x4000002;
    m_38->m_stateFlags |= 1;
    m_playFlags = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(g_voiceKeyA);
    m_source = 0;
    m_owner = 0;
}

// ===========================================================================
// CGruntVoice::~CGruntVoice  (0x119ae0)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00119ae0, 0x44)
CGruntVoice::~CGruntVoice() {}

// ===========================================================================
// CGruntVoice::InitActReg  (0x119dc0)
// ===========================================================================
// Construct the class's activation-coordinate registry singleton (g_vactColl
// @0x6514d8) over the fixed range [2000, 2010] via the shared registry ctor
// (FUN_00408710, __thiscall ret 8). A free init thunk (no `this`); reloc-masked.
RVA(0x00119dc0, 0x15)
void CGruntVoice::InitActReg() {
    g_vactColl.Construct(2000, 2010);
}

// ===========================================================================
// CGruntVoice::Dispatch  (0x119e40)
// ===========================================================================
// The per-coordinate activation registry dispatch - the SAME ActLookup/
// FireActivation shape as CSecretTeleporterTrigger::FireActivation (0x042150),
// on CGruntVoice's OWN registry statics (0x6514xx). Look the coordinate up to an
// Entry*; if its handler slot is set, look it up again and invoke the handler
// __thiscall on `this`.
RVA(0x00119e40, 0x102)
void CGruntVoice::Dispatch(i32 coord) {
    CVActEntry* e = VActLookup(coord);
    if (e->m_fn != 0) {
        CVActEntry* e2 = VActLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// ===========================================================================
// CGruntVoice::Setup  (0x11a7e0)
// ===========================================================================
// Seed the voice from a play request. The `sample` arg (the 2nd, guard) is
// required - on a null sample bail with 0. Otherwise stash the source/sample/
// flag/owner params, snapshot the sample's play duration (sample->ComputeDuration
// 0x137590, __thiscall), seed the icon to the default, clear the running state,
// and swap the +0x14 sub-object's bute node to the "B" key (stashing the
// previous in CUserLogic::m_prevAnimSetNode). Returns 1.
//
// @early-stop
// store-scheduling wall (docs/patterns/statement-schedule-faithful.md): the body
// is structurally byte-exact - every offset, immediate, call arg and branch
// matches retail. The sole residual is a ~2-3 instruction phase shift: MSVC fills
// the latency slot after the ComputeDuration call by hoisting the `m_14` load
// (edx) up, which in turn swaps the adjacent {m_58:=g_iconDefault, m_5c:=0}
// stores; retail loads m_14 late (after the icon stores) and keeps m_58 before
// m_5c. No source reorder flips the allocator (m_58/m_5c swap -> 83%, a captured
// m_14 local -> 70%, m_6c hoist -> 83%); the in-order spelling at 85% is the best.
// Deferred to the final sweep.
RVA(0x0011a7e0, 0x6e)
i32 CGruntVoice::Setup(i32 a0, void* sample, i32 a2, i32 a3) {
    if (sample == 0) {
        return 0;
    }
    m_source = a0;
    m_owner = a3;
    m_sample = (i32)sample;
    m_durationMs = ((StreamVoice*)sample)->ComputeRatio();
    m_64 = 0;
    m_icon = g_iconDefault;
    m_5c = 0;
    m_playFlags = a2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(g_iconBute);
    return 1;
}

// ===========================================================================
// CGruntVoice::Reset  (0x11a870)
// ===========================================================================
// Clear the voice: zero the sample slot (+0x54), swap the +0x14 sub-object's
// bute node to the "A" idle-anim key (stashing the previous in CUserLogic::m_prevAnimSetNode),
// then clear the flag/owner slots (+0x6c/+0x68).
RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(g_voiceKeyA);
    m_playFlags = 0;
    m_source = 0;
}
