// AnimWorkerHandlers.cpp - the anim-worker message-handler family for the map's
// trigger / point / spawn game objects. Each handler is a __cdecl FREE function
// (the owner is a stack arg at [esp+0x18], ecx is never `this`); it reads
// owner->m_7c (the worker), then runs a /GX message pump keyed on the worker's
// state tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow
//                   it at worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump
//                   (0x16e4f0, __cdecl, taking the sub-record).
// The handlers are byte-identical bar the sub-record TYPE (the `new` size + ctor
// target). The switch key worker->m_1c is UNSIGNED (u32) so MSVC5 emits the range
// checks as unsigned ja/jbe, matching retail byte-for-byte (a signed i32 key emits
// jg/jle and caps each at 97.86%; see docs/patterns/switch-key-unsigned-ja-vs-jg.md).
//
// De-fragmentation: this TU held FIVE distinct retail /Gy objects conflated into one
// 1.1 MB span. Split (matcher-1) -> the grunt-HUD indicator-sprite handler cluster
// (0x7db20..0x7e2a0) is now GruntIndicatorWorkerHandlers.cpp; the out-of-line worker
// ctor (0x15b300) is now AnimWorkerCtor.cpp. This TU keeps the contiguous trigger/
// point handler cluster (0x3d2b0..0x3ddf0) plus the two same-shape Owner*-handlers
// flanking it: 0x3a200 (news CCursorSnapSprite; sits next to CursorSnapSprite's ctor)
// and 0x46990 (news CExplosion; sits next to LogicRecordDispatch's LogicDispatchC).
// Those two are attributable to CursorSnapSprite.cpp / the Explosion+LogicDispatch
// neighbourhood, but those files are matcher-3-contended, so they stay in the
// handler family here (mirrors the scattered-handler role TUs LogicRecordDispatch.cpp
// / InGameWorkerHandlers.cpp).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine).
#include <rva.h>

#include <Gruntz/AnimWorker.h> // shared Owner / Worker views + Worker_DefaultPump

// The dispatched sub-records are real CUserLogic leaves (vftable 0x5e705c, 16 slots);
// the worker pump calls their inherited slots (slot 6 activate @+0x18, slots 10..15
// @+0x28..+0x3c) directly - no fabricated view class.
#include <Gruntz/UserLogic.h>

// The real CUserLogic game-object leaves each contiguous handler builds in its state-0
// case. Sizes are proven from the `new` immediates.
#include <Gruntz/CursorSnapSprite.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SecretTeleporterTrigger.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/Wormhole.h>

// The earliest member of the state-0 dispatch family (builds a CCursorSnapSprite,
// a real header leaf whose ctor takes CGameObject*).
RVA(0x0003a200, 0xf1)
i32 Handler03a200(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CCursorSnapSprite((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// The state-0 anim-worker dispatch family (0x3d2b0..0x3ddf0) - one contiguous retail
// /Gy object. Each handler is byte-identical to Handler03d670 bar the CUserLogic leaf
// it `new`s in state 0 (hence the `new` size immediate + ctor symbol).
RVA(0x0003d2b0, 0xf1)
i32 Handler03d2b0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntStartingPoint((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d3f0, 0xf1)
i32 Handler03d3f0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CExitTrigger((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d530, 0xf1)
i32 Handler03d530(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntCreationPoint((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d670, 0xf1)
i32 Handler03d670(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CWormhole((CGameObject*)owner);
            sub->Activate(); // slot 6 (+0x18): activate
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d7b0, 0xf1)
i32 Handler03d7b0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntPuddle((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d8f0, 0xf1)
i32 Handler03d8f0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CTeleporter((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003da30, 0xf1)
i32 Handler03da30(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CSecretTeleporterTrigger((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// CWarlord's ctor is the odd one out: it takes an int (??0CWarlord@@QAE@H@Z), so
// the owner pointer is passed as (i32)owner (retail `push edi`); size 0xb0.
RVA(0x0003db70, 0xf4)
i32 Handler03db70(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CWarlord((i32)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003dcb0, 0xf1)
i32 Handler03dcb0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CFortressFlag((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003ddf0, 0xf1)
i32 Handler03ddf0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CSecretLevelTrigger((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// The latest flanking Owner*-handler (builds a CExplosion). Sits in retail next to
// LogicRecordDispatch's LogicDispatchC (0x46850) - same __cdecl dispatch shape.
RVA(0x00046990, 0xf1)
i32 Handler046990(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CExplosion((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}
