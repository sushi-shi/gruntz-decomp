// LogicWorkerHandlers.cpp - the 0xf1/0xf4 logic-worker message-handler family
// (the same archetype as AnimWorkerHandlers / InGameWorkerHandlers).
//
// ONE original TU (wave2-H merge): the former logicworkerhandlers + msgdispatch
// units were a WOVEN single interval (TU_MIGRATION 0x0a9a40, weave 0.27), plus
// the two in-interval strays HandlerA9E00 (ex DoNothing.cpp) and Handler0AA6E0
// (ex InGameWorkerHandlers.cpp) - all first-link-contiguous members of this TU
// (its own 8-frag init run 0xa9770..0xa9a40 directly precedes the code). The
// msgdispatch trio's leaf identities are now RECOVERED from their retail ctor
// call targets (thunks 0x2e4b/0x2216/0x35d5): CMenuSparkle (0xadbe0), CFrontCandy
// (0xabfa0), CFrontCandyAni (0xacf40) - the CObj placeholder view is gone.
//
// The handlers are __cdecl FREE functions (the owner is a stack arg at
// [esp+0x18]; ecx is never touched - the trace-clusterer's tomalla-72/75/76/...
// "classes" are a false grouping). Each reads owner->m_7c (the worker), then runs
// a /GX message pump keyed on the worker's state tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow
//                   it at worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump
//                   (0x16e4f0, __cdecl, taking the sub-record).
// The handlers are byte-identical bar the sub-record TYPE (hence the `new` size +
// ctor target). The created records are CUserLogic-derived game objects (shared
// base vptr 0x5e70b4). In retail RVA order:
//   0xa9a40 -> CAniCycle          (ctor 0xaad20, size 0x54)
//   0xa9b80 -> CSingleFrameMessage(ctor 0xab310, size 0x54)
//   0xa9cc0 -> CDoNothing         (ctor 0xac1d0, size 0x54)
//   0xa9e00 -> CDoNothingNormal   (INLINED leaf ctor - see HandlerA9E00 below)
//   0xa9f60 -> CSimpleAnimation   (ctor 0xab940, size 0x54)
//   0xaa0a0 -> CMenuSparkle       (ctor 0xadbe0, size 0x54)
//   0xaa1e0 -> CFrontCandy        (ctor 0xabfa0, size 0x54)
//   0xaa320 -> CBehindCandy       (ctor 0xac3f0, size 0x54)
//   0xaa460 -> CFrontCandyAni     (ctor 0xacf40, size 0x54)
//   0xaa5a0 -> CBehindCandyAni    (ctor 0xad540, size 0x54)
//   0xaa6e0 -> CEyeCandy          (ctor 0xac620, size 0x54)
//   0xaa820 -> CEyeCandyAni       (ctor 0xac870, size 0x54)
//   0xaa960 -> CWayPoint          (ctor 0xae3f0, size 0x54)
//   0xaaaa0 -> CSingleAnimation   (ctor 0xae7f0, size 0x54)
//   0xaabe0 -> CGuardPoint        (ctor 0xae5f0, size 0x54)
//   0xaf0a0 -> CRollingBall       (ctor 0xaf820, size 0xa0 -> 0xf4-byte handler)
//   0xaf1e0 -> CSpotLight         (ctor 0xb1200, size 0xa8 -> 0xf4-byte handler)
//   0xaf320 -> CKitchenSlime      (ctor 0xb23a0, size 0x90 -> 0xf4-byte handler)
//   0xaf460 -> CPathHazard        (ctor 0xafc50, size 0x130 -> 0xf4-byte handler)
//   0xaf5a0 -> CRainCloud         (ctor 0xb04d0, size 0x130 -> 0xf4-byte handler)
//   0xaf6e0 -> CUFO               (ctor 0xb0790, size 0x130 -> 0xf4-byte handler)
// (The 0xaf0a0.. block is this unit's second interval - RollingBall territory -
// left as-is pending that region's own resolution.)
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/AniCycle.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/DoNothing.h>
#include <Gruntz/SimpleAnimation.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/FrontCandy.h>
#include <Gruntz/BehindCandy.h>
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/BehindCandyAni.h>
#include <Gruntz/EyeCandy.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/WayPoint.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/GuardPoint.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/Ufo.h>

#include <Gruntz/WorkerHandler.h> // shared Worker / Owner archetype + CUserLogic base

// The dispatched records are real CUserLogic leaves (their most-derived 1-arg
// __thiscall ctors are matched in other TUs). `new LEAF((CGameObject*)owner)`
// lowers to push sizeof(LEAF); call operator new; mov ecx,raw; push owner; call
// <ctor> (all reloc-masked); the post-construction activate + pump dispatches
// lower to `mov eax,[obj]; call [eax+N]` through the inherited 16-slot CUserLogic
// vtable - no fabricated view class.
//
// The switch key worker->m_1c is UNSIGNED (u32); MSVC5 then emits the range checks
// as unsigned ja/jbe, matching retail byte-for-byte. A signed i32 key emits jg/jle
// and caps the function at 97.86%. See docs/patterns/switch-key-unsigned-ja-vs-jg.md.
#define LOGIC_WORKER_PUMP(LEAF)                                                                    \
    Worker* rec = owner->m_7c;                                                                     \
    switch (rec->m_1c) {                                                                           \
        case 0: {                                                                                  \
            rec->m_1c = 0x3e8;                                                                     \
            CUserLogic* sub = new LEAF((CGameObject*)owner);                                       \
            sub->Activate(); /* slot 6 (+0x18): activate */                                        \
            rec->m_18 = sub;                                                                       \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_18->UserLogicVfunc9(); /* slot 11 (+0x2c) */                                    \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_18->UserLogicVfunc8(); /* slot 10 (+0x28) */                                    \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_18->UserLogicVfuncC(); /* slot 14 (+0x38) */                                    \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_18->UserLogicVfuncD(); /* slot 15 (+0x3c) */                                    \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_18->UserLogicVfuncA(); /* slot 12 (+0x30) */                                    \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_18->UserLogicVfuncB(); /* slot 13 (+0x34) */                                    \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_18);                                                         \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x000a9a40, 0xf1)
i32 HandlerA9A40(Owner* owner){LOGIC_WORKER_PUMP(CAniCycle)}

RVA(0x000a9b80, 0xf1)
i32 HandlerA9B80(Owner* owner){LOGIC_WORKER_PUMP(CSingleFrameMessage)}

RVA(0x000a9cc0, 0xf1)
i32 HandlerA9CC0(Owner* owner) {
    LOGIC_WORKER_PUMP(CDoNothing)
}

// ===========================================================================
// HandlerA9E00 (@0x0a9e00, ex DoNothing.cpp) - the CDoNothingNormal pump: the
// SAME archetype, but case 0 INLINES the CDoNothingNormal leaf ctor instead of
// calling a single external full ctor. That leaf ctor chains the OUT-OF-LINE
// shared CUserLogic(owner) base (CUserLogic_058cd0 @0x58cd0, reached through the
// 0x3828 ILT thunk; owned by the CUserLogic TU) - which does NOT set
// m_34/m_38/m_3c - then sets m_34/m_38/m_3c, stamps the CDoNothingNormal vtable
// (0x5e859c) AFTER those stores, and raises owner->m_08 bit 0.
//
// WHY THESE VIEWS ARE NOT FOLDED onto the shared WorkerHandler.h Owner/Worker +
// the real CUserLogic: the inlined leaf ctor CALLS the out-of-line CUserLogic
// base ctor (0x58cd0), whereas the sibling leaf ctors (e.g. CDoNothing @0xac1d0,
// DoNothing.cpp) INLINE the CUserLogic init. One TU cannot model CUserLogic's
// ctor both inline AND out-of-line-called: the inline-XOR-out-of-line ctor wall.
// CUserLogicOOL is the escape (a distinct class whose ctor IS the 0x58cd0 call);
// DnnRec derives it (not the real CUserLogic), so the worker's m_18 slot cannot
// be the canonical CUserLogic* either, and the owner view adds the m_08 flag
// word the reduced WorkerHandler.h Owner lacks. Matching scaffolding for the
// inlined-ctor variant, not fakes to fold.
// ===========================================================================

struct DnnOwner;

// The OUT-OF-LINE shared CUserLogic(CGameObject*) base ctor (CUserLogic_058cd0
// @0x58cd0). External/no-body so the chained `call` reloc-masks. It sets
// m_0c/m_10/m_14/m_04/m_08/m_28/m_2c + the throwing link + AddLogic*; it does NOT
// set m_34/m_38/m_3c (the leaf does). Modeled as a non-polymorphic base class so
// the leaf record chains it; the throwing link ctor inside forces the /GX EH frame.
struct CUserLogicOOL {
    virtual void Vf0();             // +0x00  declared-only vptr anchor (polymorphic base)
    CUserLogicOOL(DnnOwner* owner); // 0x58cd0
    char m_pad04[0x34 - 0x04];      // +0x04..+0x33
    DnnOwner* m_34;                 // +0x34
    DnnOwner* m_38;                 // +0x38
    void* m_3c;                     // +0x3c
};

// Real polymorphic: DnnRec (a distinct polymorphic leaf) makes cl auto-stamp its own
// ??_7DnnRec at ctor entry, replacing the old vptr-MIDDLE manual stamp of the engine
// CDoNothingNormal vtable (0x5e859c; realized as ??_7CDoNothingNormal in
// DoNothing.cpp via RealizeCDoNothingNormal). The auto-stamp lands at ctor entry
// (after the base ctor) rather than after the member stores - an accepted codegen
// shift (see the @early-stop).
struct DnnRec : CUserLogicOOL {
    char m_leaf[0x54 - 0x40]; // +0x40..+0x53
    DnnRec(DnnOwner* owner);
};

// The dispatch interface: a polymorphic class with the same vtable slot layout as
// the engine record (slot 6 @ +0x18 = activate; slots 10..15 @ +0x28..+0x3c = the
// per-state handlers). Declared-only + never constructed here, so no ??_7 is
// emitted; a record* is reinterpreted to it only to lower `mov eax,[rec]; call
// [eax+N]`.
class EngRec {
public:
    virtual void s0();       // +0x00
    virtual void s1();       // +0x04
    virtual void s2();       // +0x08
    virtual void s3();       // +0x0c
    virtual void s4();       // +0x10
    virtual void s5();       // +0x14
    virtual void Activate(); // +0x18  (slot 6)
    virtual void s7();       // +0x1c
    virtual void s8();       // +0x20
    virtual void s9();       // +0x24
    virtual void V28();      // +0x28
    virtual void V2C();      // +0x2c
    virtual void V30();      // +0x30
    virtual void V34();      // +0x34
    virtual void V38();      // +0x38
    virtual void V3C();      // +0x3c
};

// The worker held at owner->m_7c; only the pump fields are modeled.
struct DnnWorker {
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    char m_pad04[0x18 - 0x04];
    DnnRec* m_18; // +0x18  the live record
    u32 m_1c;     // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to the pump; its worker hangs at +0x7c.
struct DnnOwner {
    char m_pad00[0x08];
    u32 m_08; // +0x08
    char m_pad0c[0x7c - 0x0c];
    DnnWorker* m_7c; // +0x7c
};

// The engine default message pump (0x16e4f0) taking the DnnRec view: a C++
// overload of the extern "C" Worker_DefaultPump above (one address, two views;
// both undefined externs, reloc-masked rel32).
void Worker_DefaultPump(DnnRec* rec); // 0x16e4f0

// case 0: the inlined CDoNothingNormal leaf ctor (base OOL ctor + leaf tail).
inline DnnRec::DnnRec(DnnOwner* owner) : CUserLogicOOL(owner) {
    m_34 = owner;
    m_38 = owner;
    m_3c = owner->m_7c;
    m_38->m_08 |= 1;
}

// The switch key worker->m_1c is UNSIGNED (u32); MSVC5 then emits the range checks
// as unsigned ja/jbe, matching retail byte-for-byte (switch-key-unsigned-ja-vs-jg).
//
// @early-stop
// case-0 leaf-ctor-tail scheduling/regalloc coin-flip (~97.9%, topic:wall
// topic:scheduling): the whole pump + the new/base-ctor/EH-state framework are
// byte-identical (verified base vs target with llvm-objdump -dr). Two residual
// deltas, both in the inlined case-0 tail: (1) MSVC now auto-stamps the ??_7DnnRec
// vptr at ctor entry (after the base ctor), whereas retail stamps the leaf vtable
// vptr-MIDDLE, after the `m_3c = owner->m_7c` load/store; (2) MSVC lowers
// `m_38->m_08 |= 1` as an 8-byte load / or al,1 /
// store through the live param edi, whereas retail reloads m_38 from [esi+0x38]
// and does the 7-byte `or dword [eax+0x8],1` RMW - making the body 1 byte longer.
// Neither is source-steerable (tried reordered stores, a temp for the load, and a
// cast-through-pointer vptr write - all identical codegen). Parked for the final
// sweep.
RVA(0x000a9e00, 0x10c)
i32 HandlerA9E00(DnnOwner* owner) {
    DnnWorker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            DnnRec* sub = new DnnRec(owner);
            ((EngRec*)sub)->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            ((EngRec*)rec->m_18)->V2C();
            break;
        case 0x1e:
            ((EngRec*)rec->m_18)->V28();
            break;
        case 0x50:
            ((EngRec*)rec->m_18)->V38();
            break;
        case 0x53:
            ((EngRec*)rec->m_18)->V3C();
            break;
        case 0x52:
            ((EngRec*)rec->m_18)->V30();
            break;
        case 0x51:
            ((EngRec*)rec->m_18)->V34();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x000a9f60, 0xf1)
i32 HandlerA9F60(Owner* owner){LOGIC_WORKER_PUMP(CSimpleAnimation)}

RVA(0x000aa0a0, 0xf1)
i32 HandlerAA0A0(Owner* owner){LOGIC_WORKER_PUMP(CMenuSparkle)}

RVA(0x000aa1e0, 0xf1)
i32 HandlerAA1E0(Owner* owner){LOGIC_WORKER_PUMP(CFrontCandy)}

RVA(0x000aa320, 0xf1)
i32 HandlerAA320(Owner* owner){LOGIC_WORKER_PUMP(CBehindCandy)}

RVA(0x000aa460, 0xf1)
i32 HandlerAA460(Owner* owner){LOGIC_WORKER_PUMP(CFrontCandyAni)}

RVA(0x000aa5a0, 0xf1)
i32 HandlerAA5A0(Owner* owner){LOGIC_WORKER_PUMP(CBehindCandyAni)}

RVA(0x000aa6e0, 0xf1)
i32 Handler0aa6e0(Owner* owner){LOGIC_WORKER_PUMP(CEyeCandy)}

RVA(0x000aa820, 0xf1)
i32 HandlerAA820(Owner* owner){LOGIC_WORKER_PUMP(CEyeCandyAni)}

RVA(0x000aa960, 0xf1)
i32 HandlerAA960(Owner* owner){LOGIC_WORKER_PUMP(CWayPoint)}

RVA(0x000aaaa0, 0xf1)
i32 HandlerAAAA0(Owner* owner){LOGIC_WORKER_PUMP(CSingleAnimation)}

RVA(0x000aabe0, 0xf1)
i32 HandlerAABE0(Owner* owner){LOGIC_WORKER_PUMP(CGuardPoint)}

RVA(0x000af0a0, 0xf4)
i32 HandlerAF0A0(Owner* owner){LOGIC_WORKER_PUMP(CRollingBall)}

RVA(0x000af1e0, 0xf4)
i32 HandlerAF1E0(Owner* owner){LOGIC_WORKER_PUMP(CSpotLight)}

RVA(0x000af320, 0xf4)
i32 HandlerAF320(Owner* owner){LOGIC_WORKER_PUMP(CKitchenSlime)}

RVA(0x000af460, 0xf4)
i32 HandlerAF460(Owner* owner){LOGIC_WORKER_PUMP(CPathHazard)}

RVA(0x000af5a0, 0xf4)
i32 HandlerAF5A0(Owner* owner){LOGIC_WORKER_PUMP(CRainCloud)}

RVA(0x000af6e0, 0xf4)
i32 HandlerAF6E0(Owner* owner){LOGIC_WORKER_PUMP(CUFO)}

SIZE_UNKNOWN(CUserLogicOOL);
SIZE_UNKNOWN(DnnRec);
SIZE_UNKNOWN(EngRec);
SIZE_UNKNOWN(DnnOwner);
SIZE_UNKNOWN(DnnWorker);
