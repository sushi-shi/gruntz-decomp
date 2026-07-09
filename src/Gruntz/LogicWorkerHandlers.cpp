// LogicWorkerHandlers.cpp - the 0xf1/0xf4 logic-worker message-handler family
// (the same archetype as AnimWorkerHandlers / InGameWorkerHandlers).
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
//   0xa9f60 -> CSimpleAnimation   (ctor 0xab940, size 0x54)
//   0xaa320 -> CBehindCandy       (ctor 0xac3f0, size 0x54)
//   0xaa5a0 -> CBehindCandyAni    (ctor 0xad540, size 0x54)
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
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/AniCycle.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/DoNothing.h>
#include <Gruntz/SimpleAnimation.h>
#include <Gruntz/BehindCandy.h>
#include <Gruntz/BehindCandyAni.h>
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
#define LOGIC_WORKER_PUMP(LEAF)                                                                      \
    Worker* rec = owner->m_7c;                                                                       \
    switch (rec->m_1c) {                                                                             \
        case 0: {                                                                                    \
            rec->m_1c = 0x3e8;                                                                       \
            CUserLogic* sub = new LEAF((CGameObject*)owner);                                         \
            sub->Activate();  /* slot 6 (+0x18): activate */                                         \
            rec->m_18 = sub;                                                                         \
            break;                                                                                   \
        }                                                                                            \
        case 0x1d:                                                                                   \
            rec->m_18->UserLogicVfunc9(); /* slot 11 (+0x2c) */                                      \
            break;                                                                                   \
        case 0x1e:                                                                                   \
            rec->m_18->UserLogicVfunc8(); /* slot 10 (+0x28) */                                      \
            break;                                                                                   \
        case 0x50:                                                                                   \
            rec->m_18->UserLogicVfuncC(); /* slot 14 (+0x38) */                                      \
            break;                                                                                   \
        case 0x53:                                                                                   \
            rec->m_18->UserLogicVfuncD(); /* slot 15 (+0x3c) */                                      \
            break;                                                                                   \
        case 0x52:                                                                                   \
            rec->m_18->UserLogicVfuncA(); /* slot 12 (+0x30) */                                      \
            break;                                                                                   \
        case 0x51:                                                                                   \
            rec->m_18->UserLogicVfuncB(); /* slot 13 (+0x34) */                                      \
            break;                                                                                   \
        case 0x3e8:                                                                                  \
            break;                                                                                   \
        default:                                                                                     \
            Worker_DefaultPump(rec->m_18);                                                           \
            break;                                                                                   \
    }                                                                                                \
    return 1;

RVA(0x000a9a40, 0xf1)
i32 HandlerA9A40(Owner* owner) { LOGIC_WORKER_PUMP(CAniCycle) }

RVA(0x000a9b80, 0xf1)
i32 HandlerA9B80(Owner* owner) { LOGIC_WORKER_PUMP(CSingleFrameMessage) }

RVA(0x000a9cc0, 0xf1)
i32 HandlerA9CC0(Owner* owner) { LOGIC_WORKER_PUMP(CDoNothing) }

RVA(0x000a9f60, 0xf1)
i32 HandlerA9F60(Owner* owner) { LOGIC_WORKER_PUMP(CSimpleAnimation) }

RVA(0x000aa320, 0xf1)
i32 HandlerAA320(Owner* owner) { LOGIC_WORKER_PUMP(CBehindCandy) }

RVA(0x000aa5a0, 0xf1)
i32 HandlerAA5A0(Owner* owner) { LOGIC_WORKER_PUMP(CBehindCandyAni) }

RVA(0x000aa820, 0xf1)
i32 HandlerAA820(Owner* owner) { LOGIC_WORKER_PUMP(CEyeCandyAni) }

RVA(0x000aa960, 0xf1)
i32 HandlerAA960(Owner* owner) { LOGIC_WORKER_PUMP(CWayPoint) }

RVA(0x000aaaa0, 0xf1)
i32 HandlerAAAA0(Owner* owner) { LOGIC_WORKER_PUMP(CSingleAnimation) }

RVA(0x000aabe0, 0xf1)
i32 HandlerAABE0(Owner* owner) { LOGIC_WORKER_PUMP(CGuardPoint) }

RVA(0x000af0a0, 0xf4)
i32 HandlerAF0A0(Owner* owner) { LOGIC_WORKER_PUMP(CRollingBall) }

RVA(0x000af1e0, 0xf4)
i32 HandlerAF1E0(Owner* owner) { LOGIC_WORKER_PUMP(CSpotLight) }

RVA(0x000af320, 0xf4)
i32 HandlerAF320(Owner* owner) { LOGIC_WORKER_PUMP(CKitchenSlime) }

RVA(0x000af460, 0xf4)
i32 HandlerAF460(Owner* owner) { LOGIC_WORKER_PUMP(CPathHazard) }

RVA(0x000af5a0, 0xf4)
i32 HandlerAF5A0(Owner* owner) { LOGIC_WORKER_PUMP(CRainCloud) }

RVA(0x000af6e0, 0xf4)
i32 HandlerAF6E0(Owner* owner) { LOGIC_WORKER_PUMP(CUFO) }
