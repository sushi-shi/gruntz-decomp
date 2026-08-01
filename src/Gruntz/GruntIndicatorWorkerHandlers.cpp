#include <Gruntz/GameObjectFactory.h>
#include <rva.h>

#include <Gruntz/AnimWorker.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntStaminaSprite.h>
#include <Gruntz/GruntToyTimeSprite.h>
#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/GruntPowerupSprite.h>

#define ANIM_WORKER_PUMP(LEAF)                                                                     \
    AnimWorkerObj* rec = owner->m_animWorker;                                                      \
    switch (static_cast<u32>(rec->ActKey())) {                                                     \
        case 0: {                                                                                  \
            rec->SetActKey(0x3e8);                                                                 \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate();                                                                       \
            rec->m_logic = sub;                                                                    \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_logic->UserLogicVfunc9();                                                       \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_logic->UserLogicVfunc8();                                                       \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_logic->UserLogicVfuncC();                                                       \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_logic->UserLogicVfuncD();                                                       \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_logic->UserLogicVfuncA();                                                       \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_logic->UserLogicVfuncB();                                                       \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                      \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x0007db20, 0xf1)
i32 CreateGruntSelectedSprite(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CGruntSelectedSprite(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
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
i32 CreateGruntHealthSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntHealthSprite)}

RVA(0x0007dda0, 0xf1)
i32 CreateGruntToySprite(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CGruntToySprite(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
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
i32 CreateGruntStaminaSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntStaminaSprite)}

RVA(0x0007e020, 0xf1)
i32 CreateGruntToyTimeSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntToyTimeSprite)}

RVA(0x0007e160, 0xf1)
i32 CreateGruntWingzTimeSprite(CGameObject* owner){ANIM_WORKER_PUMP(CGruntWingzTimeSprite)}

RVA(0x0007e2a0, 0xf1)
i32 CreateGruntPowerupSprite(CGameObject* owner) {
    ANIM_WORKER_PUMP(CGruntPowerupSprite)
}
