// GruntIndicatorWorkerHandlers.cpp - the anim-worker message-handler cluster for the
// grunt-HUD indicator sprites (selected-highlight / health / toy / stamina / toy-time /
// wingz-time / powerup). One contiguous retail /Gy object (0x7db20..0x7e2a0), split out
// of AnimWorkerHandlers.cpp (matcher-1) - it needs the sprite-leaf size-views
// (AnimWorkerSpriteLeaves.h).
//
// @identity-TODO the reason this file was SPLIT OFF ("the canonical class headers pull the
// Grunt.h world that cannot coexist with the trigger/point leaf headers the sibling TU
// uses") is FALSE - falsified 2026-07-13: Grunt.h and UserLogic.h define no class in common,
// and a TU including UserLogic.h + GruntStaminaSprite.h + GruntWingzTimeSprite.h compiles
// clean under the real MSVC 5.0. See the note atop <Gruntz/AnimWorkerSpriteLeaves.h>. The
// split (and the size-views) can be dissolved; the wall is not real.
//
// Each handler is a __cdecl FREE function byte-identical to the trigger/point handlers
// in AnimWorkerHandlers.cpp bar the CUserLogic leaf it `new`s on worker-state 0 (the
// `new` size immediate + ctor symbol). The switch key worker->m_1c is UNSIGNED (u32) so
// MSVC5 emits the range checks as unsigned ja/jbe, matching retail byte-for-byte (a
// signed i32 key emits jg/jle and caps each at 97.86%; see
// docs/patterns/switch-key-unsigned-ja-vs-jg.md).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine).
#include <rva.h>

#include <Gruntz/AnimWorker.h>             // shared Owner / Worker views + Worker_DefaultPump
#include <Gruntz/UserLogic.h>              // CUserLogic 16-slot vtable the pump dispatches
// The REAL sprite-leaf classes (the AnimWorkerSpriteLeaves.h size-views are dissolved).
// The "canonical headers cannot coexist with UserLogic.h" wall was FALSE: they compile
// together under the real MSVC 5.0, and each computes exactly the retail operator-new
// immediate the views recorded (0x5c / 0x60 / 0x64 / 0x60) - proven with compile-time
// size assertions, so the `new T(owner)` size + ctor target are unchanged.
#include <Gruntz/GruntSelectedSprite.h>  // 0x5c
#include <Gruntz/GruntToySprite.h>       // 0x60
#include <Gruntz/GruntHealthSprite.h>    // 0x64
#include <Gruntz/GruntStaminaSprite.h>   // 0x64
#include <Gruntz/GruntToyTimeSprite.h>   // 0x64
#include <Gruntz/GruntWingzTimeSprite.h> // 0x64
#include <Gruntz/GruntPowerupSprite.h>   // 0x60

// The 0x7dc60.. handlers are byte-identical to the two written out below bar the leaf
// TYPE `new`d on state 0 (the size + ctor target); shared as a macro.
#define ANIM_WORKER_PUMP(LEAF)                                                                     \
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

RVA(0x0007db20, 0xf1)
i32 Handler07db20(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntSelectedSprite((CGameObject*)owner);
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

RVA(0x0007dc60, 0xf1)
i32 Handler07dc60(Owner* owner){ANIM_WORKER_PUMP(CGruntHealthSprite)} // new 0x64, ctor 0x07eb00

RVA(0x0007dda0, 0xf1)
i32 Handler07dda0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntToySprite((CGameObject*)owner);
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

RVA(0x0007dee0, 0xf1)
i32 Handler07dee0(Owner* owner){ANIM_WORKER_PUMP(CGruntStaminaSprite)} // new 0x64, ctor 0x07fae0

RVA(0x0007e020, 0xf1)
i32 Handler07e020(Owner* owner){ANIM_WORKER_PUMP(CGruntToyTimeSprite)} // new 0x64, ctor 0x07fbd0

RVA(0x0007e160, 0xf1)
i32 Handler07e160(Owner* owner){ANIM_WORKER_PUMP(CGruntWingzTimeSprite)} // new 0x64, ctor 0x07fcc0

RVA(0x0007e2a0, 0xf1)
i32 Handler07e2a0(Owner* owner) {
    ANIM_WORKER_PUMP(CGruntPowerupSprite)
} // new 0x60, ctor 0x07fdb0
