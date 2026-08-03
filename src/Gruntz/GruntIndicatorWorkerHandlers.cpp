#include <rva.h>

#include <Gruntz/AnimWorker.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntStaminaSprite.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntToyTimeSprite.h>
#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/AnimWorkerAct.h>

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
        case ACT_OBJECT_REMOVED:                                                                   \
            rec->m_logic->OnObjectRemoved();                                                       \
            break;                                                                                 \
        case ACT_LEAVE_ACTIVE_REGION:                                                              \
            rec->m_logic->OnLeaveActiveRegion();                                                   \
            break;                                                                                 \
        case ACT_PREPARE_SAVE:                                                                     \
            rec->m_logic->PrepareSave();                                                           \
            break;                                                                                 \
        case ACT_AFTER_LOAD_REFERENCES:                                                            \
            rec->m_logic->AfterLoadReferences();                                                   \
            break;                                                                                 \
        case ACT_AFTER_LOAD:                                                                       \
            rec->m_logic->AfterLoad();                                                             \
            break;                                                                                 \
        case ACT_AFTER_SAVE:                                                                       \
            rec->m_logic->AfterSave();                                                             \
            break;                                                                                 \
        case ACT_LIVE:                                                                             \
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
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CGruntSelectedSprite(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
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
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CGruntToySprite(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
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
