// ObjectLogicPump.cpp - the per-frame state/message pumps for the three dropped-object
// tile-logic leaves (CObjectDropper / CDroppedObject / CDroppedObjectShadow), homed from
// the GapFunctions stubs. Each is the SAME archetype as StateDispatch.cpp's CLevelTime
// pump (0x9b770): a __cdecl(CGameObject*) that reads the object's +0x7c aux state id
// (+0x1c) and, on id 0, operator-new's + constructs its leaf (id then latched to 0x3e8),
// Activates it (vtable slot 6) and installs it into aux->m_logic (+0x18); routes ids
// 0x1d/0x1e/0x50..0x53 to the active handler's matching CUserLogic virtual slot; and
// (default) hands the handler to the shared type-keyed serializer. Always returns 1.
//
// The active handler is the canonical CUserLogic (UserLogic.h): slot [6]=Activate,
// [10..15]=UserLogicVfunc8..D. Only offsets + code bytes are load-bearing.
#include <rva.h>

#include <Gruntz/ObjectDropper.h>       // CObjectDropper : CUserLogic (ctor 0xc59f0)
#include <Gruntz/DroppedObject.h>       // CDroppedObject : CUserLogic (ctor 0xc68b0)
#include <Gruntz/DroppedObjectShadow.h> // CDroppedObjectShadow : CUserLogic (ctor 0xc7490)

// The default case's shared type-keyed record serializer (0x16e4f0, owned + matched in
// TypeKeyColl.cpp); the active logic leaf is the record arg (CUserLogic -> cast-free).
i32 ProjTypeXfer(CUserLogic* rec); // 0x16e4f0

// @early-stop
// throwing-operator-new /GX frame wall (docs/patterns/gx-frame-outofline-ctor.md): same
// as StateDispatch.cpp (~32%) - retail's id-0 `new C<Leaf>(obj)` wraps the BARE out-of-
// line ctor call in the operator-delete-on-ctor-throw cleanup, i.e. a full /GX frame
// (push -1/fs:0, [esp+0x10] state 0-during-ctor/-1-after, saved raw ptr) with every case
// jmp'ing one shared fs:0-restoring epilogue. MSVC5 reconstruction cannot re-raise that
// frame for a bare out-of-line ctor `new` (proven in StateDispatch). Body/switch/dispatch
// byte-faithful; frameless plateau. Deferred to the final sweep.
RVA(0x000c5630, 0xf4)
i32 ObjectDropperPump(CGameObject* obj) {
    CGameObjAux* aux = obj->m_7c;
    switch ((i32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CObjectDropper* h = new CObjectDropper(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

// @early-stop
// same throwing-operator-new /GX frame wall as ObjectDropperPump above (~32%).
RVA(0x000c5770, 0xf1)
i32 DroppedObjectPump(CGameObject* obj) {
    CGameObjAux* aux = obj->m_7c;
    switch ((i32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CDroppedObject* h = new CDroppedObject(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

// @early-stop
// same throwing-operator-new /GX frame wall as ObjectDropperPump above (~32%).
RVA(0x000c58b0, 0xf1)
i32 DroppedObjectShadowPump(CGameObject* obj) {
    CGameObjAux* aux = obj->m_7c;
    switch ((i32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CDroppedObjectShadow* h = new CDroppedObjectShadow(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}
