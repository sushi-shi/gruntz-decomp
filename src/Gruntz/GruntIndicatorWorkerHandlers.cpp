#include <rva.h>

#include <Gruntz/AnimWorker.h> // shared Owner / Worker views + Worker_DefaultPump
#include <Gruntz/UserLogic.h>  // CUserLogic 16-slot vtable the pump dispatches
#include <Gruntz/GruntSelectedSprite.h>  // 0x5c
#include <Gruntz/GruntToySprite.h>       // 0x60
#include <Gruntz/GruntHealthSprite.h>    // 0x64
#include <Gruntz/GruntStaminaSprite.h>   // 0x64
#include <Gruntz/GruntToyTimeSprite.h>   // 0x64
#include <Gruntz/GruntWingzTimeSprite.h> // 0x64
#include <Gruntz/GruntPowerupSprite.h>   // 0x60

#define ANIM_WORKER_PUMP(LEAF)                                                                     \
    AnimWorkerObj* rec = owner->m_7c;                                                                     \
    switch (reinterpret_cast<u32>(rec->m_1c)) {                                                                           \
        case 0: {                                                                                  \
            rec->m_1c = reinterpret_cast<void*>(0x3e8);                                                                     \
            CUserLogic* sub = new LEAF(owner);                                       \
            sub->Activate(); /* slot 6 (+0x18): activate */                                        \
            rec->m_logic = sub;                                                                       \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_logic->UserLogicVfunc9(); /* slot 11 (+0x2c) */                                    \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_logic->UserLogicVfunc8(); /* slot 10 (+0x28) */                                    \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_logic->UserLogicVfuncC(); /* slot 14 (+0x38) */                                    \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_logic->UserLogicVfuncD(); /* slot 15 (+0x3c) */                                    \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_logic->UserLogicVfuncA(); /* slot 12 (+0x30) */                                    \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_logic->UserLogicVfuncB(); /* slot 13 (+0x34) */                                    \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                         \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x0007db20, 0xf1)
extern "C" i32 CreateGruntSelectedSprite(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CGruntSelectedSprite(owner);
            sub->Activate(); // slot 6 (+0x18): activate
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0007dc60, 0xf1)
extern "C" i32 CreateGruntHealthSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntHealthSprite)} // new 0x64, ctor 0x07eb00

RVA(0x0007dda0, 0xf1)
extern "C" i32 CreateGruntToySprite(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CGruntToySprite(owner);
            sub->Activate(); // slot 6 (+0x18): activate
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0007dee0, 0xf1)
extern "C" i32 CreateGruntStaminaSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntStaminaSprite)} // new 0x64, ctor 0x07fae0

RVA(0x0007e020, 0xf1)
extern "C" i32 CreateGruntToyTimeSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntToyTimeSprite)} // new 0x64, ctor 0x07fbd0

RVA(0x0007e160, 0xf1)
extern "C" i32 CreateGruntWingzTimeSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntWingzTimeSprite)} // new 0x64, ctor 0x07fcc0

RVA(0x0007e2a0, 0xf1)
extern "C" i32 CreateGruntPowerupSprite(CGameObject* owner) {
    ANIM_WORKER_PUMP(CGruntPowerupSprite)
} // new 0x60, ctor 0x07fdb0
