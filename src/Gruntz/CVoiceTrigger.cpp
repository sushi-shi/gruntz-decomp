// CVoiceTrigger.cpp - the voice-trigger game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CVoiceTrigger methods, defined in ascending retail-RVA
// order:
//   ~CVoiceTrigger @0x0135a0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Tick           @0x11a700 - the per-frame "play the voice cue when on screen" probe.
//
// CVoiceTrigger : CUserLogic. The class also has its no-arg ctor in
// src/Gruntz/UserLogic.cpp; this TU adds the out-of-line dtor copy + the Tick
// method. A minimal local class redeclaration is enough (the dtor fold depends
// only on the CUserLogic base hierarchy from <Gruntz/UserLogic.h>, and Tick is a
// plain __thiscall member whose codegen depends only on its body + offsets).
// Only offsets / code bytes are load-bearing; names are placeholders.
#include <Gruntz/UserLogic.h> // CUserLogic base (CVoiceTrigger : CUserLogic)

class CVoiceTrigger : public CUserLogic {
public:
    i32 Tick();       // 0x11a700
    ~CVoiceTrigger(); // 0x0135a0 (folds the CUserLogic teardown)
};

// The on-screen-cue receiver (g_gameReg->m_68). QueryAt (0x75c60, via the 0x32ce
// thunk) resolves the entity whose screen rect overlaps the trigger; CueA
// (0x11b3b0, via the 0x39f4 thunk) fires the voice cue on it. Both __thiscall,
// modeled NO-body so the calls reloc-mask.
struct CVoiceHit; // the entity QueryAt returns
struct CVoiceSink {
    // QueryAt(x, y, &m_10->m_134, &outA, &outB, &m_10->m_144) -> entity* (or 0).
    CVoiceHit* QueryAt(i32 x, i32 y, i32* rect, i32* outA, i32* outB, i32* z); // 0x75c60
    // CueA(hit, m_10->m_124, m_10->m_128, 0, -1, -1) -> nonzero on fire.
    i32 CueA(CVoiceHit* hit, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x11b3b0
};

// The entity QueryAt returns: its bound sprite (+0x10) carries the screen x/y
// (+0x5c/+0x60) the on-screen window test reads.
struct CVoiceHitSprite {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c screen x
    i32 m_60; // +0x60 screen y
};
struct CVoiceHit {
    char m_pad0[0x10];
    CVoiceHitSprite* m_10; // +0x10 bound sprite
};

// The bound sprite (this->m_10): +0x5c/+0x60 = screen x/y, +0x124/+0x128 = the
// voice-cue ids passed to CueA, +0x134/+0x144 = the probe rect bounds.
struct CVoiceSprite {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c  screen x
    i32 m_60; // +0x60  screen y
    char m_pad64[0x124 - 0x64];
    i32 m_124; // +0x124 voice-cue id a
    i32 m_128; // +0x128 voice-cue id b
    char m_pad12c[0x134 - 0x12c];
    i32 m_134; // +0x134 probe rect lo
    char m_pad138[0x144 - 0x138];
    i32 m_144; // +0x144 probe rect hi
};

// The on-screen window object (this->m_38): +0x08 holds the per-frame status
// bits (bit 0x10000 == "fired / handled this frame").
struct CVoiceWindow {
    char m_pad0[0x8];
    i32 m_8; // +0x08 status bits
};

// The global game registry (WwdGameReg, RVA 0x24556c; wwdfile owns the DATA
// label). The on-screen window bounds are at +0x13c/+0x140/+0x144/+0x148; the
// cue receiver at +0x60 (CueA's `this`) and the probe sink at +0x68 (QueryAt's
// `this`).
struct WwdGameReg {
    char m_pad0[0x60];
    CVoiceSink* m_60; // +0x60 cue receiver (CueA's this)
    char m_pad64[0x68 - 0x64];
    CVoiceSink* m_68; // +0x68 probe sink (QueryAt's this)
    char m_pad6c[0x13c - 0x6c];
    i32 m_13c; // +0x13c window min x
    i32 m_140; // +0x140 window min y
    i32 m_144; // +0x144 window max x
    i32 m_148; // +0x148 window max y
};
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

// The current-area index (DAT_00644c54, VA 0x644c54 / RVA 0x244c54); the trigger
// only fires for the active area. extern "C" so the load reloc-masks against the
// already-named global.
extern "C" i32 g_644c54;

// CVoiceTrigger::~CVoiceTrigger @0x0135a0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x000135a0, 0x44)
CVoiceTrigger::~CVoiceTrigger() {}

// CVoiceTrigger::Tick @0x11a700 - query the entity under the trigger's screen
// rect; if it is in the active area and its sprite sits inside the on-screen
// window, fire the voice cue and mark the window handled this frame. Always
// returns 0.
RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CVoiceHit* hit = g_gameReg->m_68->QueryAt(((CVoiceSprite*)m_10)->m_5c, ((CVoiceSprite*)m_10)->m_60,
                                              &((CVoiceSprite*)m_10)->m_134, &outA, &outB,
                                              &((CVoiceSprite*)m_10)->m_144);
    if (hit && outA == g_644c54) {
        CVoiceHitSprite* hs = hit->m_10;
        i32 hy = hs->m_60;
        i32 hx = hs->m_5c;
        if (hx < g_gameReg->m_144 && hx >= g_gameReg->m_13c && hy < g_gameReg->m_148
            && hy >= g_gameReg->m_140) {
            if (g_gameReg->m_60->CueA(hit, ((CVoiceSprite*)m_10)->m_124,
                                      ((CVoiceSprite*)m_10)->m_128, 0, -1, -1)) {
                ((CVoiceWindow*)m_38)->m_8 |= 0x10000;
            }
        }
    }
    return 0;
}
