// LogicWorkerHandlersB.cpp - block B of the logic-worker message-handler family:
// the contiguous .text run 0x0af0a0..0x0af6e0 (6 handlers = one original obj).
// Split out of the former LogicWorkerHandlers.cpp (block A = LogicWorkerHandlersA.cpp,
// 0xa9a40..0xaabe0). These 6 are the RollingBall-territory sub-record pumps; each
// is a 0xf4-byte handler (a larger leaf record than block A's 0xf1-byte handlers).
//
// OWNER / DISPATCH-TABLE CRACK: same registrar as block A - each handler is
// registered by RegisterGameObjectTypes (0x000a3b0) into the game-object type
// manager at registry+0x14 via its vtable slot +0x24 = RegisterType(name,
// pump_thunk, flags). They are __cdecl FREE per-type logic-pump callbacks, not
// class methods; `Owner` is the shared reduced game-object archetype
// (WorkerHandler.h). Same /GX pump shape as block A, keyed on the UNSIGNED worker
// state tag worker->m_1c. The created records are CUserLogic-derived game objects.
// In RVA order:
//   0xaf0a0 -> CRollingBall  (ctor 0xaf820, size 0xa0)
//   0xaf1e0 -> CSpotLight    (ctor 0xb1200, size 0xa8)
//   0xaf320 -> CKitchenSlime (ctor 0xb23a0, size 0x90)
//   0xaf460 -> CPathHazard   (ctor 0xafc50, size 0x130)
//   0xaf5a0 -> CRainCloud    (ctor 0xb04d0, size 0x130)
//   0xaf6e0 -> CUFO          (ctor 0xb0790, size 0x130)
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/Ufo.h>

#include <Gruntz/WorkerHandler.h> // shared Worker / Owner archetype + LOGIC_WORKER_PUMP

RVA(0x000af0a0, 0xf4)
i32 HandlerAF0A0(CGameObject* owner){LOGIC_WORKER_PUMP(CRollingBall)}

RVA(0x000af1e0, 0xf4)
i32 HandlerAF1E0(CGameObject* owner){LOGIC_WORKER_PUMP(CSpotLight)}

RVA(0x000af320, 0xf4)
i32 HandlerAF320(CGameObject* owner){LOGIC_WORKER_PUMP(CKitchenSlime)}

RVA(0x000af460, 0xf4)
i32 HandlerAF460(CGameObject* owner){LOGIC_WORKER_PUMP(CPathHazard)}

RVA(0x000af5a0, 0xf4)
i32 HandlerAF5A0(CGameObject* owner){LOGIC_WORKER_PUMP(CRainCloud)}

RVA(0x000af6e0, 0xf4)
i32 HandlerAF6E0(CGameObject* owner) {
    LOGIC_WORKER_PUMP(CUFO)
}
