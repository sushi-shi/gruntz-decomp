#define USERLOGIC_OOL_CTOR // retail CALLS ??0CUserLogic (0x58cd0) at this TU case-0; decl-only base ctor
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
i32 HandlerA9A40(CGameObject* owner){LOGIC_WORKER_PUMP(CAniCycle)}

RVA(0x000a9b80, 0xf1)
i32 HandlerA9B80(CGameObject* owner){LOGIC_WORKER_PUMP(CSingleFrameMessage)}

RVA(0x000a9cc0, 0xf1)
i32 HandlerA9CC0(CGameObject* owner) {
    LOGIC_WORKER_PUMP(CDoNothing)
}

#include <Gruntz/DoNothingNormalDtor.h> // the real CDoNothingNormal leaf (ex the DnnRec pen)

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
i32 HandlerA9E00(CGameObject* owner){LOGIC_WORKER_PUMP(CDoNothingNormal)}

RVA(0x000a9f60, 0xf1)
i32 HandlerA9F60(CGameObject* owner){LOGIC_WORKER_PUMP(CSimpleAnimation)}

RVA(0x000aa0a0, 0xf1)
i32 HandlerAA0A0(CGameObject* owner){LOGIC_WORKER_PUMP(CMenuSparkle)}

RVA(0x000aa1e0, 0xf1)
i32 HandlerAA1E0(CGameObject* owner){LOGIC_WORKER_PUMP(CFrontCandy)}

RVA(0x000aa320, 0xf1)
i32 HandlerAA320(CGameObject* owner){LOGIC_WORKER_PUMP(CBehindCandy)}

RVA(0x000aa460, 0xf1)
i32 HandlerAA460(CGameObject* owner){LOGIC_WORKER_PUMP(CFrontCandyAni)}

RVA(0x000aa5a0, 0xf1)
i32 HandlerAA5A0(CGameObject* owner){LOGIC_WORKER_PUMP(CBehindCandyAni)}

RVA(0x000aa6e0, 0xf1)
i32 EyeCandyWorkerPump(CGameObject* owner){LOGIC_WORKER_PUMP(CEyeCandy)}

RVA(0x000aa820, 0xf1)
i32 HandlerAA820(CGameObject* owner){LOGIC_WORKER_PUMP(CEyeCandyAni)}

RVA(0x000aa960, 0xf1)
i32 HandlerAA960(CGameObject* owner){LOGIC_WORKER_PUMP(CWayPoint)}

RVA(0x000aaaa0, 0xf1)
i32 HandlerAAAA0(CGameObject* owner){LOGIC_WORKER_PUMP(CSingleAnimation)}

RVA(0x000aabe0, 0xf1)
i32 HandlerAABE0(CGameObject* owner){LOGIC_WORKER_PUMP(CGuardPoint)}
