// CExitTrigger.cpp - the level-exit trigger tile-logic game-object (C:\Proj\Gruntz).
//
// Two CExitTrigger methods, defined in ascending retail-RVA order:
//   GetTypeTag     @0x010870 - the 6-byte per-class logic-type id accessor (0x3f7).
//   ~CExitTrigger  @0x0108c0 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CExitTrigger : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CExitTrigger.h>
#include <Gruntz/CGameRegistry.h>

// CExitTrigger::GetTypeTag @0x010870 - return the class's logic-type id. The same
// 6-byte `mov eax,<id>; ret` virtual archetype as CTileTriggerTransition::
// GetTypeTag (0x011730).
RVA(0x00010870, 0x6)
i32 CExitTrigger::GetTypeTag() {
    return 0x3f7;
}

// CExitTrigger::~CExitTrigger @0x0108c0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x000108c0, 0x44)
CExitTrigger::~CExitTrigger() {}

// The global bute store the leaf interns "A" into (g_buteTree @0x6bf620; Find
// 0x16d190 __thiscall ret 4). The "A" key (0x60a454).
#include <Bute/ButeMgr.h>
extern CButeTree g_buteTree;
DATA(0x0020a454)
extern char s_actKeyA[]; // "A"

// The active-area index (DAT_00644c54): the exit trigger pins the focused warlord
// HUD only for the active area.
extern "C" i32 g_644c54;

// The level-exit entity the bring-up resolves under the trigger's tile (the
// "Warlord" head probed at the bound object's screen pos). Its +0x7c carries a
// finalize sub-object: a fn-ptr at +0x10 (cdecl, takes the entity) + an id at
// +0x18. +0x124 is the area/owner index, +0x188 the resolved value stored back.
struct CExitSubObj {
    char m_pad00[0x10];
    void(__cdecl* m_10)(void* e); // +0x10  finalize fn-ptr
    char m_pad14[0x18 - 0x14];
    i32 m_18; // +0x18  resolved id
};
SIZE_UNKNOWN(CExitSubObj);
struct CExitEntity {
    char m_pad00[0x7c];
    CExitSubObj* m_7c; // +0x7c
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124  area/owner index
    char m_pad128[0x188 - 0x128];
    i32 m_188; // +0x188
};
SIZE_UNKNOWN(CExitEntity);

// The registry probe sink (g_gameReg->m_world->m_8): Probe (0x1597b0, the 0x60a668
// "Warlord" lookup at the bound screen pos) resolves the exit entity or 0.
struct CExitProbeSink {
    CExitEntity* Probe(i32 a, i32 x, i32 y, i32 b, const char* key, i32 flag); // 0x1597b0
};
SIZE_UNKNOWN(CExitProbeSink);
struct CExitMgr30 {
    char m_pad00[0x08];
    CExitProbeSink* m_8; // +0x08
};
SIZE_UNKNOWN(CExitMgr30);

// The focused-warlord cue slot (g_gameReg+0x150, stride 0x238, indexed by the
// bound object's area index m_124): +0x20 the live gate, +0xc the stored id,
// +0x220/+0x224 the snapped trigger position.
struct CExitFocusSlot {
    char m_pad00[0x0c];
    i32 m_0c; // +0x0c
    char m_pad10[0x20 - 0x10];
    i32 m_20; // +0x20  live gate
    char m_pad24[0x220 - 0x24];
    i32 m_220; // +0x220
    i32 m_224; // +0x224
};
SIZE_UNKNOWN(CExitFocusSlot);

// The cue receiver (g_gameReg->m_68): +0x2a0 holds the active warlord id.
struct CExitCueSink {
    char m_pad00[0x2a0];
    i32 m_2a0; // +0x2a0
};
SIZE_UNKNOWN(CExitCueSink);

// The game registry singleton (g_mgrSettings @0x64556c): +0x30 the probe-sink
// holder, +0x68 the cue receiver, the per-area focus slots at +0x150.
SIZE_UNKNOWN(CGameRegistry);
DATA(0x0024556c)
extern CGameRegistry* g_exitGameReg;

// CExitTrigger::CExitTrigger(CGameObject*) @0x03ecf0 - the 1-arg leaf ctor: the
// standard CUserLogic(obj) init (folded inline) plus the exit tail - cl emits the
// implicit leaf vftable (??_7CExitTrigger @0x5e822c) stamp, then raise the bound
// object's logic bit, cache the "A" bute node, snap its screen position, set its
// z-gate (0x124f8) + the four unit bounds, apply the GAME_CYCLE100 geometry, then
// resolve the warlord into the per-area focus slot: when the slot is live, store the
// trigger position, probe the "Warlord" entity, run its finalize fn-ptr, snapshot
// its id, and bind it to the active area + cue. Constructs a throwing CUserBaseLink,
// so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful (the CUserLogic init, the implicit leaf vptr stamp, the "A" cache, the tile
// snaps, the z-gate, the four `=1` bounds, the geometry apply, the focus-slot
// resolve + warlord probe + finalize); the residue is this ctor's own __ehfuncinfo
// state numbering + the zero-register-pinning callee-saved choice (the shared
// CUserLogic-init wall) + the `and al,0xe0` byte-AND codegen pick. The SAME plateau
// as CTimeBomb / the other bute ctors; not source-steerable. Parked for the final sweep.
RVA(0x0003ecf0, 0x292)
CExitTrigger::CExitTrigger(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 2;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_actKeyA);
    m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
    m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
    if (m_10->m_74 != 0x124f8) {
        m_10->m_74 = 0x124f8;
        m_10->m_08 |= 0x20000;
    }
    m_10->m_144 = 1;
    m_10->m_14c = 1;
    m_10->m_148 = 1;
    m_10->m_150 = 1;
    m_savedGeoId = m_38->m_1b4;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_warlordId = 0;
    CExitFocusSlot* slot = (CExitFocusSlot*)((char*)g_exitGameReg + m_10->m_124 * 0x238 + 0x150);
    if (slot->m_20 == 0) {
        m_resolved = 0;
        return;
    }
    slot->m_220 = m_10->m_5c;
    slot->m_224 = m_10->m_60;
    CExitEntity* e = ((CExitMgr30*)g_exitGameReg->m_world)
                         ->m_8->Probe(0, m_10->m_5c, m_10->m_60, 0, "Warlord", 0x40003);
    if (e != 0) {
        e->m_124 = m_10->m_124;
        e->m_7c->m_10(e);
        m_warlordId = e->m_7c->m_18;
        if (m_10->m_124 == g_644c54) {
            ((CExitCueSink*)g_exitGameReg->m_68)->m_2a0 = m_warlordId;
        }
        CExitFocusSlot* slot2 =
            (CExitFocusSlot*)((char*)g_exitGameReg + m_10->m_124 * 0x238 + 0x150);
        if (slot2 != 0) {
            slot2->m_0c = e->m_188;
        }
    }
    m_resolved = 1;
}
