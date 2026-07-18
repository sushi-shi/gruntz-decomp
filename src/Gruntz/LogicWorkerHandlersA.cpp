// LogicWorkerHandlersA.cpp - block A of the logic-worker message-handler family:
// the contiguous .text run 0x0a9a40..0x0aabe0 (15 handlers = one original obj).
// Split out of the former LogicWorkerHandlers.cpp, whose functions sat in two
// widely-separated RVA blocks (0xa9a40.. and 0xaf0a0..) = two original objs.
// Block B (0xaf0a0..0xaf6e0) is LogicWorkerHandlersB.cpp.
//
// OWNER / DISPATCH-TABLE CRACK (xref of each handler's ILT jmp-thunk):
//   Every handler is registered by RegisterGameObjectTypes (0x000a3b0) into the
//   game-object type manager held at registry+0x14, via that manager's vtable
//   slot +0x24 = RegisterType(name, pump_thunk, flags). The registered NAME
//   string for each thunk (in .data ~0x60a9c0..) is the game-object type name and
//   matches the leaf class 1:1 (AniCycle, DoNothingNormal, DoNothing,
//   SimpleAnimation, MenuSparkle, FrontCandy, BehindCandy, FrontCandyAni,
//   BehindCandyAni, EyeCandy, EyeCandyAni, WayPoint, SingleAnimation, GuardPoint).
//   So these are genuine __cdecl FREE callbacks (the type-registry's per-type
//   logic pump), NOT class methods; `Owner` is the shared reduced game-object
//   archetype (WorkerHandler.h), not a fake per-TU view.
//
// The handlers are __cdecl free functions (the owner is a stack arg at [esp+0x18];
// ecx is never touched). Each reads owner->m_7c (the worker), then runs a /GX
// message pump keyed on the worker's UNSIGNED state tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow it at
//                   worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump.
// The handlers are byte-identical bar the sub-record TYPE (the `new` size + ctor
// target). The created records are CUserLogic-derived game objects. In RVA order:
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
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine).
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

#include <Gruntz/WorkerHandler.h> // shared Worker / Owner archetype + LOGIC_WORKER_PUMP

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

// The inline-XOR-out-of-line ctor-wall scaffolding (CUserLogicOOL / DnnRec / EngRec /
// DnnWorker / DnnOwner) lives in <Gruntz/LogicWorkerHandlersAViews.h> - genuine matching
// scaffolding for a real MSVC5 codegen limitation, not fakes to fold.
#include <Gruntz/LogicWorkerHandlersAViews.h>

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

SIZE_UNKNOWN(CUserLogicOOL);
SIZE_UNKNOWN(DnnRec);
SIZE_UNKNOWN(EngRec);
SIZE_UNKNOWN(DnnOwner);
SIZE_UNKNOWN(DnnWorker);
