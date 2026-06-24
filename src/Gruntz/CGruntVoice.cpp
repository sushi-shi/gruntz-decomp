// CGruntVoice.cpp - a grunt-voice game/sound object (a CUserLogic-derived leaf).
// Four __thiscall methods reconstructed in ascending-RVA order:
//   0x119ae0  ~CGruntVoice  (the bare CUserLogic teardown, /GX frame)
//   0x119e40  Dispatch      (per-coordinate activation registry -> handler call)
//   0x11a7e0  Setup         (seed the voice from a sample + play params)
//   0x11a870  Reset         (clear the voice, re-bind the "A" idle-anim bute)
//
// CUserLogic / CUserBase / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// the bute keys ("B" g_iconBute / "A" g_voiceKeyA), the default-icon global and
// g_buteTree from <Gruntz/CInGameIcon.h> / this header. Engine callees are
// reloc-masked (no body). Confirmed CGruntVoice by its CUserLogic teardown
// shape and the FireActivation-twin dispatch (see the header).
#include <Gruntz/CGruntVoice.h>

#include <rva.h>

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
// CGruntVoice::Dispatch  (0x119e40)
// ===========================================================================
// The per-coordinate activation registry dispatch - the SAME ActLookup/
// FireActivation shape as CSecretTeleporterTrigger::FireActivation (0x042150),
// on CGruntVoice's OWN registry statics (0x6514xx). Look the coordinate up to an
// Entry*; if its handler slot is set, look it up again and invoke the handler
// __thiscall on `this`.
RVA(0x00119e40, 0x102)
void CGruntVoice::Dispatch(int coord) {
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
// previous in CUserLogic::m_30). Returns 1.
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
int CGruntVoice::Setup(int a0, void* sample, int a2, int a3) {
    if (sample == 0) {
        return 0;
    }
    m_68 = a0;
    m_70 = a3;
    m_54 = (int)sample;
    m_60 = ((CVoiceSample*)sample)->ComputeDuration();
    m_64 = 0;
    m_58 = g_iconDefault;
    m_5c = 0;
    m_6c = a2;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(g_iconBute);
    return 1;
}

// ===========================================================================
// CGruntVoice::Reset  (0x11a870)
// ===========================================================================
// Clear the voice: zero the sample slot (+0x54), swap the +0x14 sub-object's
// bute node to the "A" idle-anim key (stashing the previous in CUserLogic::m_30),
// then clear the flag/owner slots (+0x6c/+0x68).
RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_54 = 0;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(g_voiceKeyA);
    m_6c = 0;
    m_68 = 0;
}
