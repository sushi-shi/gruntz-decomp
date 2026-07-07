#include <Mfc.h>
// SecretLevelTrigger.cpp - the secret-level trigger game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CSecretLevelTrigger methods, defined in ascending
// retail-RVA order:
//   ~CSecretLevelTrigger @0x010c50 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Tick                 @0x042ac0 - the per-frame "is the trigger on screen?" probe.
//
// CSecretLevelTrigger : CUserLogic. The class also has its no-arg/1-arg ctors in
// src/Gruntz/UserLogic.cpp; this TU adds the out-of-line dtor copy + the Tick
// method. A minimal local class redeclaration is enough (the dtor fold depends
// only on the CUserLogic base hierarchy from <Gruntz/UserLogic.h>, and Tick is a
// plain __thiscall member whose codegen depends only on its body + offsets).
// Only offsets / code bytes are load-bearing; names are placeholders.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Trigger.h>            // shared point-probe result object
#include <Gruntz/SecretLevelTrigger.h> // canonical CSecretLevelTrigger : CUserLogic
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// CSecretLevelTrigger : CUserLogic - the canonical class shape now lives in
// <Gruntz/SecretLevelTrigger.h> (shared with the ctors in UserLogic.cpp).

// The class registry entry: its first dword receives the Tick handler PMF (a
// 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CSecretLevelTrigger::*SecretActHandler)();
struct CSecretActEntry {
    SecretActHandler m_fn;
};
SIZE_UNKNOWN(CSecretActEntry);

// The class's activation-coordinate registry singleton (@0x644598): the fixed
// [2000,2010] range built by the shared registry ctor (0x408710). CSecretActReg is
// the shared <Gruntz/ActReg.h> CActReg archetype (was a per-file duplicate of its
// layout + ResolveEntry); it keeps its own placeholder name so the DATA-pinned
// global symbol is unchanged.
struct CSecretActReg : public CActReg {};
SIZE_UNKNOWN(CSecretActReg);
DATA(0x00244598)
extern CSecretActReg g_secretActReg; // 0x644598

// The on-screen-cue receiver (g_gameReg->m_68). Probe (0x75af0, via the 0x35f3
// thunk) point-probes the trigger's screen position and returns the trigger
// object under it (or 0); ScrollTo (0x6bcb0, via the 0x2e96 thunk) posts a scroll
// to bring it on screen. Both __thiscall, modeled NO-body so the calls
// reloc-mask (their bodies live in src/Gruntz/TriggerMgr.cpp).

// The probed trigger object is the shared <Gruntz/Trigger.h> class: its
// +0x170/+0x198 are the level/layer ids the bound sprite's +0x11c/+0x120 must
// match for the trigger to fire.
SIZE_UNKNOWN(CTrigger);

// The bound sprite (this->m_10, a CGameObject*): +0x5c/+0x60 = screen x/y,
// +0x11c/+0x120 = required level/layer ids; the on-screen window (this->m_38)'s
// +0x08 holds the per-frame status bits (bit 0x10000 == "stalled / handled this
// frame"). All modeled on CGameObject (<Gruntz/UserLogic.h>) - read directly.

// The global game registry (CGameRegistry, RVA 0x24556c; wwdfile owns the DATA
// label); only the on-screen-cue receiver at +0x68 is touched.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// CSecretLevelTrigger::~CSecretLevelTrigger @0x010c50 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x00010c50, 0x44)
CSecretLevelTrigger::~CSecretLevelTrigger() {}

// CSecretLevelTrigger::InitActReg @0x0426e0 - construct the class's activation-
// coordinate registry singleton (g_secretActReg @0x644598) over the fixed range
// [2000, 2010] via the shared registry ctor (FUN_00408710). Free init thunk;
// reloc-masked.
RVA(0x000426e0, 0x15)
void CSecretLevelTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_secretActReg)->Construct(2000, 2010);
}

// CSecretLevelTrigger::RegisterActs @0x0428c0 - bind the class's per-frame handler
// (Tick @0x042ac0) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset Tick`
// handler store match retail); residual is the slot-vs-id callee-saved register
// choice cascading into the free-loop count materialization. Deferred.
RVA(0x000428c0, 0x18d)
void CSecretLevelTrigger::RegisterActs() {
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
    ((CSecretActEntry*)g_secretActReg.ResolveEntry(id))->m_fn = &CSecretLevelTrigger::Tick;
}

// CSecretLevelTrigger::Tick @0x042ac0 - probe the trigger's screen position; if a
// trigger object is under it whose level/layer ids match the bound sprite's
// (or are unset), post a scroll to bring it on screen and mark the window stalled
// this frame. Always returns 0.
RVA(0x00042ac0, 0x90)
i32 CSecretLevelTrigger::Tick() {
    i32 outA, outB;
    CGameObject* spr = m_object;
    CTrigger* hit = (CTrigger*)((CTriggerMgr*)g_gameReg->m_cmdGrid)
                        ->HitTestCell(spr->m_screenX, spr->m_screenY, &outB, &outA, 1);
    if (hit) {
        spr = m_object;
        i32 ok = 1;
        i32 lvl = spr->m_11c;
        i32 lyr = spr->m_120;
        // (m_11c/m_120 = required level/layer ids on the bound CGameObject)
        if (lvl != 0 && hit->m_170 != lvl) {
            ok = 0;
        }
        if (lyr != 0 && hit->m_198 != lyr) {
            ok = 0;
        }
        if (ok) {
            ((CTriggerMgr*)g_gameReg->m_cmdGrid)->CellDispatch(outB, outA, 0xc, -1);
        }
        m_38->m_flags |= 0x10000;
    }
    return 0;
}
