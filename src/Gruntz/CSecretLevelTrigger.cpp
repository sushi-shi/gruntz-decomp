// CSecretLevelTrigger.cpp - the secret-level trigger game-object (C:\Proj\Gruntz).
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
#include <Gruntz/CGameRegistry.h>
#include <Gruntz/CTrigger.h>  // shared point-probe result object
#include <Gruntz/UserLogic.h> // CUserLogic base (CSecretLevelTrigger : CUserLogic)

class CSecretLevelTrigger : public CUserLogic {
public:
    static void InitActReg();   // 0x0426e0 (construct g_secretActReg over [2000,2010])
    static void RegisterActs(); // 0x0428c0 (register the class's activation handlers)
    i32 Tick();                 // 0x042ac0
    ~CSecretLevelTrigger();     // 0x010c50 (folds the CUserLogic teardown)
};
SIZE_UNKNOWN(CSecretLevelTrigger);

// The class registry entry: its first dword receives the Tick handler PMF (a
// 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CSecretLevelTrigger::*SecretActHandler)();
struct CSecretActEntry {
    SecretActHandler m_fn;
};
SIZE_UNKNOWN(CSecretActEntry);

// The class's activation-coordinate registry singleton (@0x644598). Same shape as
// CIndicatorActReg: a fixed [2000,2010] range built by the shared registry ctor
// FUN_00408710 (__thiscall ret 8). The id->entry resolve (ResolveEntry) folds the
// VActLookup archetype; the slow Insert is __thiscall on m_coll2.
struct CSecretActReg {
    void* m_vptr;       // +0x00
    CActColl2* m_coll2; // +0x04
    i32 m_lo;           // +0x08
    i32 m_hi;           // +0x0c
    char* m_base;       // +0x10
    char* m_cur;        // +0x14
    i32 m_stride;       // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_scratch; // +0x20

    void Construct(i32 lo, i32 hi); // 0x408710 (__thiscall ret 8)

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
SIZE_UNKNOWN(CSecretActReg);
DATA(0x00244598)
extern CSecretActReg g_secretActReg; // 0x644598

// The on-screen-cue receiver (g_gameReg->m_68). Probe (0x75af0, via the 0x35f3
// thunk) point-probes the trigger's screen position and returns the trigger
// object under it (or 0); ScrollTo (0x6bcb0, via the 0x2e96 thunk) posts a scroll
// to bring it on screen. Both __thiscall, modeled NO-body so the calls
// reloc-mask (their bodies live in src/Gruntz/TriggerMgr.cpp).
struct CTriggerSink {
    CTrigger* Probe(i32 x, i32 y, i32* outA, i32* outB, i32 flag); // 0x75af0
    void ScrollTo(i32 a, i32 b, i32 mode, i32 flags);              // 0x6bcb0
};
SIZE_UNKNOWN(CTriggerSink);

// The probed trigger object is the shared <Gruntz/CTrigger.h> class: its
// +0x170/+0x198 are the level/layer ids the bound sprite's +0x11c/+0x120 must
// match for the trigger to fire.
SIZE_UNKNOWN(CTrigger);

// The bound sprite (this->m_10): +0x5c/+0x60 = screen x/y, +0x11c/+0x120 =
// required level/layer ids. Typed view of the CUserLogic m_10 object.
struct CTrigSprite {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c  screen x
    i32 m_60; // +0x60  screen y
    char m_pad64[0x11c - 0x64];
    i32 m_11c; // +0x11c required level id
    i32 m_120; // +0x120 required layer id
};
SIZE_UNKNOWN(CTrigSprite);

// The on-screen window object (this->m_38): +0x08 holds the per-frame status bits
// (bit 0x10000 == "stalled / handled this frame").
struct CTrigWindow {
    char m_pad0[0x8];
    i32 m_8; // +0x08 status bits
};
SIZE_UNKNOWN(CTrigWindow);

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
    g_secretActReg.Construct(2000, 2010);
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
    CTrigSprite* spr = (CTrigSprite*)m_10;
    CTrigger* hit = ((CTriggerSink*)g_gameReg->m_68)->Probe(spr->m_5c, spr->m_60, &outB, &outA, 1);
    if (hit) {
        spr = (CTrigSprite*)m_10;
        i32 ok = 1;
        i32 lvl = spr->m_11c;
        i32 lyr = spr->m_120;
        if (lvl != 0 && hit->m_170 != lvl) {
            ok = 0;
        }
        if (lyr != 0 && hit->m_198 != lyr) {
            ok = 0;
        }
        if (ok) {
            ((CTriggerSink*)g_gameReg->m_68)->ScrollTo(outB, outA, 0xc, -1);
        }
        ((CTrigWindow*)m_38)->m_8 |= 0x10000;
    }
    return 0;
}
