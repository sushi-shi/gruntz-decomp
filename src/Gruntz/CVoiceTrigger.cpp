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
#include <Gruntz/CGameRegistry.h>

class CVoiceTrigger : public CUserLogic {
public:
    CVoiceTrigger(CGameObject* obj);   // 0x119b50 (1-arg leaf ctor)
    static void InitActReg();          // 0x11a320 (constructs g_vtrigColl @0x651500)
    void RegisterActs();               // 0x11a500 (binds Tick to the activation key "A")
    i32 Tick();                        // 0x11a700
    virtual ~CVoiceTrigger() OVERRIDE; // 0x0135a0 (folds the CUserLogic teardown)
};
SIZE_UNKNOWN(CVoiceTrigger);

// The bound CGameObject (m_10/m_38) viewed by the ctor: the tile-config bound
// counts at +0x134..+0x140 and the derived screen-rect bounds at +0x144..+0x150
// (the SAME shape CExitTrigger/CCheckpointTrigger seed). +0x08 flag word, +0x40
// pending bit. Only the touched offsets are modeled.
struct VTrigCtorObj {
    char m_pad00[0x08];
    i32 m_08; // +0x08  flag word
    char m_pad0c[0x40 - 0x0c];
    i32 m_40; // +0x40  pending bit
    char m_pad44[0x5c - 0x44];
    i32 m_5c; // +0x5c  screen X
    i32 m_60; // +0x60  screen Y
    char m_pad64[0x134 - 0x64];
    i32 m_134; // +0x134  left tile span
    i32 m_138; // +0x138  top tile span
    i32 m_13c; // +0x13c  right tile span
    i32 m_140; // +0x140  bottom tile span
    i32 m_144; // +0x144  derived left bound
    i32 m_148; // +0x148  derived top bound
    i32 m_14c; // +0x14c  derived right bound
    i32 m_150; // +0x150  derived bottom bound
};
SIZE_UNKNOWN(VTrigCtorObj);

// ---------------------------------------------------------------------------
// The activation registry CVoiceTrigger::RegisterActs (0x11a500) binds into - the
// trigger's OWN instance at 0x651500 (the SAME range/cache shape as every
// FireActivation registry: g_vtrigColl base + the lo/hi/base/stride/cur/scratch
// fields). The slow path Finds (0x16da80), and on miss rebuilds (ActAlloc 0x16d990
// -> g_actCache, Insert 0x16d850) yielding g_vtrigCur. All BSS globals DATA-pinned
// so the loads reloc-mask; the collection methods are external/no-body.
// ---------------------------------------------------------------------------
struct CVTrigEntry; // an entry: first dword is the registered handler
struct CVTrigColl {
    i32 Find(i32 coord, i32 z);     // 0x16da80 (__thiscall ret 8)
    void Construct(i32 lo, i32 hi); // 0x408710 (shared registry ctor, __thiscall ret 8)
};
SIZE_UNKNOWN(CVTrigColl);
struct CVTrigColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
SIZE_UNKNOWN(CVTrigColl2);
extern "C" i32 ActAlloc(); // 0x16d990

extern i32 g_vtrigLo;
extern i32 g_vtrigHi;
extern char* g_vtrigBase;
extern i32 g_vtrigStride;
extern CVTrigEntry* g_vtrigCur;
extern i32 g_vtrigScratch;
DATA(0x00251500)
extern CVTrigColl g_vtrigColl;
extern CVTrigColl2* g_vtrigColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

struct CVTrigEntry {
    void (CVoiceTrigger::*m_fn)(); // [entry]
};
SIZE_UNKNOWN(CVTrigEntry);

// The inlined coordinate->Entry* lookup the registration resolves the slot with.
static inline CVTrigEntry* VTrigLookup(i32 coord) {
    g_vtrigScratch = 0;
    if (coord >= g_vtrigLo && coord <= g_vtrigHi) {
        return (CVTrigEntry*)(g_vtrigBase + (coord - g_vtrigLo) * g_vtrigStride);
    }
    if (g_vtrigColl.Find(coord, 0)) {
        return (CVTrigEntry*)(g_vtrigBase + (coord - g_vtrigLo) * g_vtrigStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_vtrigColl2->Insert(&g_vtrigColl, item, 0xc);
    return g_vtrigCur;
}

// The shared activation-NAME registry (the first block interns "A"). g_buteTree
// (0x6bf620, mangled-named) doubles as the name->id map; g_nextActId (0x61aea8)
// is the running id counter; s_actKeyA (0x60a454) is the "A" key; the scratch
// name registry is @0x6bf650 (same shape as g_vtrigColl).
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CVTrigColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CVTrigColl2* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo;
DATA(0x002bf65c)
extern i32 g_nameRegHi;
DATA(0x002bf660)
extern char* g_nameRegBase;
DATA(0x002bf668)
extern i32 g_nameRegStride;
DATA(0x002bf664)
extern char* g_nameRegCur;
DATA(0x002bf66c)
extern void** g_nameRegCurList;
DATA(0x002bf670)
extern i32 g_nameRegScratch;

extern CButeTree g_buteTree; // ?g_buteTree@@3VCButeTree@@A @0x6bf620

#include <Gruntz/CActName.h> // CActName (shared)

static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if (g_nameReg.Find(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_nameReg2->Insert(&g_nameReg, item, 0xc);
    return g_nameRegCur;
}

// The logic handler bound into the slot (the ILT to CVoiceTrigger::Tick @0x11a700);
// referenced by address so the DIR32 operand reloc-masks.
extern i32 VTrigLogic_11a700();

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
SIZE_UNKNOWN(CVoiceSink);

// The entity QueryAt returns: its bound sprite (+0x10) carries the screen x/y
// (+0x5c/+0x60) the on-screen window test reads.
struct CVoiceHitSprite {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c screen x
    i32 m_60; // +0x60 screen y
};
SIZE_UNKNOWN(CVoiceHitSprite);
struct CVoiceHit {
    char m_pad0[0x10];
    CVoiceHitSprite* m_10; // +0x10 bound sprite
};
SIZE_UNKNOWN(CVoiceHit);

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
SIZE_UNKNOWN(CVoiceSprite);

// The on-screen window object (this->m_38): +0x08 holds the per-frame status
// bits (bit 0x10000 == "fired / handled this frame").
struct CVoiceWindow {
    char m_pad0[0x8];
    i32 m_8; // +0x08 status bits
};
SIZE_UNKNOWN(CVoiceWindow);

// The global game registry (CGameRegistry, RVA 0x24556c; wwdfile owns the DATA
// label). The on-screen window bounds are at +0x13c/+0x140/+0x144/+0x148; the
// cue receiver at +0x60 (CueA's `this`) and the probe sink at +0x68 (QueryAt's
// `this`).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The current-area index (DAT_00644c54, VA 0x644c54 / RVA 0x244c54); the trigger
// only fires for the active area. extern "C" so the load reloc-masks against the
// already-named global.
extern "C" i32 g_644c54;

// CVoiceTrigger::~CVoiceTrigger @0x0135a0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl. It is also
// the out-of-line virtual key function, so cl emits ??_7CVoiceTrigger@@6B@
// (0x5e885c) directly - no manual g_voiceTriggerVtbl extern.
RVA(0x000135a0, 0x44)
CVoiceTrigger::~CVoiceTrigger() {}

// CVoiceTrigger::CVoiceTrigger(CGameObject*) @0x119b50 - the 1-arg leaf ctor: the
// standard CUserLogic(obj) init (folded inline) plus the voice-trigger tail - cl
// emits the implicit leaf vftable (??_7CVoiceTrigger @0x5e885c) stamp, then raise
// the bound object's logic + pending bits, cache the "A" bute node, snap the bound
// object's screen position to the 0x20 tile grid, then derive the on-screen probe
// rect from the per-side tile spans (m_134..m_140). Constructs a throwing
// CUserBaseLink, so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful (the CUserLogic init, the implicit leaf vptr stamp, the two flag
// RMWs, the "A" cache, the two tile snaps, the four rect derivations); the residue
// is this ctor's own __ehfuncinfo state numbering + the leaf vptr-restamp scheduling
// position (docs/patterns/eh-ctor-vptr-restamp-position.md) + the `and al,0xe0`
// byte-AND codegen pick. The SAME plateau as CTimeBomb / the other bute ctors; not
// source-steerable. Parked for the final sweep.
RVA(0x00119b50, 0x1ce)
CVoiceTrigger::CVoiceTrigger(CGameObject* obj) : CUserLogic(obj) {
    ((VTrigCtorObj*)m_38)->m_08 |= 2;
    ((VTrigCtorObj*)m_38)->m_40 |= 1;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_actKeyA);
    ((VTrigCtorObj*)m_10)->m_5c = (((VTrigCtorObj*)m_10)->m_5c & ~0x1f) + 0x10;
    ((VTrigCtorObj*)m_10)->m_60 = (((VTrigCtorObj*)m_10)->m_60 & ~0x1f) + 0x10;
    ((VTrigCtorObj*)m_10)->m_144 =
        ((VTrigCtorObj*)m_10)->m_5c - (((VTrigCtorObj*)m_10)->m_134 << 5) - 7;
    ((VTrigCtorObj*)m_10)->m_14c =
        ((VTrigCtorObj*)m_10)->m_5c + (((VTrigCtorObj*)m_10)->m_13c << 5) + 7;
    ((VTrigCtorObj*)m_10)->m_148 =
        ((VTrigCtorObj*)m_10)->m_60 - (((VTrigCtorObj*)m_10)->m_138 << 5) - 7;
    ((VTrigCtorObj*)m_10)->m_150 =
        ((VTrigCtorObj*)m_10)->m_60 + (((VTrigCtorObj*)m_10)->m_140 << 5) + 7;
}

// CVoiceTrigger::InitActReg @0x11a320 - construct the trigger's OWN activation-
// coordinate registry singleton (g_vtrigColl @0x651500) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0011a320, 0x15)
void CVoiceTrigger::InitActReg() {
    g_vtrigColl.Construct(2000, 2010);
}

// CVoiceTrigger::RegisterActs @0x11a500 - bind the per-frame Tick handler to the
// activation key "A" in the trigger's OWN registry (g_vtrigColl). The SAME
// archetype as CParticlez::RegisterActs.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail. Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x0011a500, 0x18d)
void CVoiceTrigger::RegisterActs() {
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
    *(void**)VTrigLookup(id) = (void*)&VTrigLogic_11a700;
}

// CVoiceTrigger::Tick @0x11a700 - query the entity under the trigger's screen
// rect; if it is in the active area and its sprite sits inside the on-screen
// window, fire the voice cue and mark the window handled this frame. Always
// returns 0.
RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CVoiceHit* hit = ((CVoiceSink*)g_gameReg->m_68)
                         ->QueryAt(
                             ((CVoiceSprite*)m_10)->m_5c,
                             ((CVoiceSprite*)m_10)->m_60,
                             &((CVoiceSprite*)m_10)->m_134,
                             &outA,
                             &outB,
                             &((CVoiceSprite*)m_10)->m_144
                         );
    if (hit && outA == g_644c54) {
        CVoiceHitSprite* hs = hit->m_10;
        i32 hy = hs->m_60;
        i32 hx = hs->m_5c;
        if (hx < g_gameReg->m_144 && hx >= g_gameReg->m_13c && hy < g_gameReg->m_148
            && hy >= g_gameReg->m_140) {
            if (((CVoiceSink*)g_gameReg->m_60)
                    ->CueA(
                        hit,
                        ((CVoiceSprite*)m_10)->m_124,
                        ((CVoiceSprite*)m_10)->m_128,
                        0,
                        -1,
                        -1
                    )) {
                ((CVoiceWindow*)m_38)->m_8 |= 0x10000;
            }
        }
    }
    return 0;
}
