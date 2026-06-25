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
#include <Gruntz/UserLogic.h> // CUserLogic base (CSecretLevelTrigger : CUserLogic)

class CSecretLevelTrigger : public CUserLogic {
public:
    i32 Tick();              // 0x042ac0
    ~CSecretLevelTrigger();  // 0x010c50 (folds the CUserLogic teardown)
};

// The on-screen-cue receiver (g_gameReg->m_68). Probe (0x75af0, via the 0x35f3
// thunk) point-probes the trigger's screen position and returns the trigger
// object under it (or 0); ScrollTo (0x6bcb0, via the 0x2e96 thunk) posts a scroll
// to bring it on screen. Both __thiscall, modeled NO-body so the calls
// reloc-mask (their bodies live in src/Gruntz/TriggerMgr.cpp).
struct CTrigger; // the trigger object Probe returns
struct CTriggerSink {
    CTrigger* Probe(i32 x, i32 y, i32* outA, i32* outB, i32 flag); // 0x75af0
    void ScrollTo(i32 a, i32 b, i32 mode, i32 flags);              // 0x6bcb0
};

// The probed trigger object: its +0x170/+0x198 are the level/layer ids the bound
// sprite's +0x11c/+0x120 must match for the trigger to fire.
struct CTrigger {
    char m_pad0[0x170];
    i32 m_170; // +0x170 level id
    char m_pad174[0x198 - 0x174];
    i32 m_198; // +0x198 layer id
};

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

// The on-screen window object (this->m_38): +0x08 holds the per-frame status bits
// (bit 0x10000 == "stalled / handled this frame").
struct CTrigWindow {
    char m_pad0[0x8];
    i32 m_8; // +0x08 status bits
};

// The global game registry (WwdGameReg, RVA 0x24556c; wwdfile owns the DATA
// label); only the on-screen-cue receiver at +0x68 is touched.
struct WwdGameReg {
    char m_pad0[0x68];
    CTriggerSink* m_68; // +0x68 on-screen-cue receiver (Probe / ScrollTo)
};
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

// CSecretLevelTrigger::~CSecretLevelTrigger @0x010c50 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x00010c50, 0x44)
CSecretLevelTrigger::~CSecretLevelTrigger() {}

// CSecretLevelTrigger::Tick @0x042ac0 - probe the trigger's screen position; if a
// trigger object is under it whose level/layer ids match the bound sprite's
// (or are unset), post a scroll to bring it on screen and mark the window stalled
// this frame. Always returns 0.
RVA(0x00042ac0, 0x90)
i32 CSecretLevelTrigger::Tick() {
    i32 outA, outB;
    CTrigSprite* spr = (CTrigSprite*)m_10;
    CTrigger* hit = g_gameReg->m_68->Probe(spr->m_5c, spr->m_60, &outB, &outA, 1);
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
            g_gameReg->m_68->ScrollTo(outB, outA, 0xc, -1);
        }
        ((CTrigWindow*)m_38)->m_8 |= 0x10000;
    }
    return 0;
}
