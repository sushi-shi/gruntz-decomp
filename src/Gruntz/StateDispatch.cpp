// StateDispatch.cpp - the CLevelTime message/state-id dispatcher (0x9b770).
//
// __stdcall(obj, a1, a2) reached with a CGameObject whose +0x7c aux carries the
// current state id (+0x1c) and the active logic handler (+0x18).  A sparse switch
// over the id either (id 0) operator-new's + constructs a fresh CLevelTime (0x54 B,
// id then 0x3e8), Activates it (vtable +0x18) and installs it, or routes one of a
// handful of ids to the active handler's matching CUserLogic virtual slot, or
// (default) hands the active handler to the shared type-keyed serializer.  Always
// returns 1.
//
// IDENTITY (view-burndown, 2026-07-05): the handler class is CLevelTime - the ctor
// this new-site bare-calls (0x9b8b0 via ILT thunk 0x404d) is the claimed+matched
// ??0CLevelTime@@QAE@PAUCGameObject@@@Z (LevelTimeDtor.cpp, 94.8%), the new size
// 0x54 == sizeof(CLevelTime), and the ctx arg is the ctor's CGameObject (its +0x7c
// aux is CGameObjAux).  The former local CStateHandler/CStateObj/CStateCtx views
// are dissolved onto the canonical <Gruntz/LevelTimeDtor.h> / <Gruntz/UserLogic.h>
// classes (slot [6]=Activate, [10..15]=UserLogicVfunc8..D).
#include <rva.h>

#include <Gruntz/LevelTimeDtor.h> // canonical CLevelTime : CTileLogic : CUserLogic (+ CGameObject/CGameObjAux)

// The default case runs the type-keyed record transfer/dispatch: NOT a bespoke
// "fallback" - it is the shared CTypeKeyColl serializer ProjTypeXfer (0x16e4f0,
// owned + matched in TypeKeyColl.cpp), which resolves the handler's type-registry
// entry and xfers it through the handler's own vtable slots. The active handler (the
// logic leaf) is passed as the record arg (CUserLogic -> cast-free).
i32 ProjTypeXfer(CUserLogic* rec); // 0x16e4f0

// @early-stop
// throwing-operator-new /GX frame wall (docs/patterns/gx-frame-outofline-ctor.md +
// rezalloc-placement-new-no-eh-frame.md): retail's id-0 `new CLevelTime(obj)` wraps
// the BARE out-of-line ctor call (0x9b8b0) in the operator-delete-on-ctor-throw
// cleanup -> a full /GX frame (push -1/fs:0, [esp+0x10] state 0-during-ctor / -1-after,
// saved raw ptr [esp+0x18]) + every case `jmp`ing one shared fs:0-restoring epilogue.
// MSVC5 reconstruction CANNOT re-raise that frame for a bare out-of-line ctor `new`
// (verified: real-virtuals+new, +declared-dtor, and inline-derived-wrapping-out-of-line-
// base-ctor all stay frameless; the last one regresses to 23% on regalloc). The frame
// mechanism needs the throwing call inside an INLINE ctor that ALSO stamps a vtable,
// which would add a new-site vptr store retail does not have. Body/switch/dispatch are
// byte-faithful; ~32% is the frameless plateau. Deferred to the final sweep.
RVA(0x0009b770, 0xf1)
i32 __stdcall StateDispatch(CGameObject* obj, i32 a1, i32 a2) {
    CGameObjAux* aux = obj->m_7c;
    // aux->m_1c doubles as the state id here (0 / 0x1d / 0x1e / 0x50..0x53 / 0x3e8)
    // - the same proven-heterogeneous aux slot other sprite classes use as a
    // lookup-node pointer; kept generically typed in the canonical (documented
    // variant), read/written through the int view at this site.
    switch ((i32)aux->m_1c) {
        case 0: {
            aux->m_1c = (void*)0x3e8;
            CLevelTime* h = new CLevelTime(obj);
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
