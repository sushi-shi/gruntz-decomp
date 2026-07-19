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
// aux is AnimWorkerObj).  The former local CStateHandler/CStateObj/CStateCtx views
// are dissolved onto the canonical <Gruntz/LevelTimeDtor.h> / <Gruntz/UserLogic.h>
// classes (slot [6]=Activate, [10..15]=UserLogicVfunc8..D).
#include <rva.h>

#include <Gruntz/LevelTimeDtor.h> // canonical CLevelTime : CTileLogic : CUserLogic (+ CGameObject/AnimWorkerObj)

// The default case runs the type-keyed record transfer/dispatch: NOT a bespoke
// "fallback" - it is the shared CTypeKeyColl serializer ProjTypeXfer (0x16e4f0,
// owned + matched in TypeKeyColl.cpp), which resolves the handler's type-registry
// entry and xfers it through the handler's own vtable slots. Its REAL arg is the
// CXferArchive record (<Gruntz/XferArchive.h>, ?ProjTypeXfer@@YAHPAUCXferArchive@@@Z);
// the active handler (the logic leaf) aliases that record at this call site.
struct CXferArchive;
i32 ProjTypeXfer(CXferArchive* ar); // 0x16e4f0

// The old "throwing-operator-new /GX frame wall" was a FLAGS MIS-PROFILE, not a
// codegen wall: this unit was compiled flags="base" (no /GX at all), so nothing
// could raise the frame. Under the true /GX profile the bare out-of-line ctor
// `new CLevelTime(obj)` raises retail's operator-delete-on-ctor-throw frame
// exactly (proven on the three ObjectDropper/DroppedObject/Shadow pumps, wave2-H
// - 33% -> 100% with flags=eh + the UNSIGNED switch key ja/jbe idiom,
// docs/patterns/switch-key-unsigned-ja-vs-jg.md). __cdecl like its pump siblings
// (retail epilogue is a plain `ret`; the old __stdcall(obj,a1,a2) emitted ret 0xc).
RVA(0x0009b770, 0xf1)
i32 StateDispatch(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    // aux->m_1c doubles as the state id here (0 / 0x1d / 0x1e / 0x50..0x53 / 0x3e8)
    // - the same proven-heterogeneous aux slot other sprite classes use as a
    // lookup-node pointer; kept generically typed in the canonical (documented
    // variant), read/written through the int view at this site.
    switch (reinterpret_cast<u32>(aux->m_1c)) {
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
            ProjTypeXfer((CXferArchive*)aux->m_logic);
            break;
    }
    return 1;
}
