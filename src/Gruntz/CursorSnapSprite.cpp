// CursorSnapSprite.cpp - the cursor-snap sprite game object (C:\Proj\Gruntz).
//
// Two trace-discovered CCursorSnapSprite methods, defined in ascending retail-RVA
// order:
//   Serialize         @0x011880 - the two-chain Serialize override.
//   ~CCursorSnapSprite @0x011920 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CCursorSnapSprite : CUserLogic (RTTI .?AVCCursorSnapSprite@@). Only offsets /
// code bytes are load-bearing; names are placeholders for the recovered engine
// identities.
#include <Gruntz/CursorSnapSprite.h>
#include <Bute/ButeTree.h>          // g_buteTree

#include <Gruntz/AnimWorker.h> // shared Owner / Worker views + Worker_DefaultPump (Handler03a200)
#include <Gruntz/UserLogic.h>  // the dispatched CUserLogic slot layout

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.

// CCursorSnapSprite::Serialize @0x011880 - chain the shared CUserLogic serialize
// helper on `this`, and (only on success) the +0x34 sub-object's chain; both run
// the same (ar, tag, c, d) tuple. Returns the second chain's success normalized
// to a bool (the retail neg/sbb/neg idiom). The SAME archetype as
// CFortressFlag::Serialize (0x46410), minus the tag-8 sprite fixup.
RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CCursorSnapSprite::~CCursorSnapSprite @0x011920 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CFortressFlag
// (0x010e90) / ~CTeleporter (0x010dd0); the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CCursorSnapSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CCursorSnapSprite@@UAE@XZ 0x00011920 0x44

// Handler03a200 @0x3a200 - this class's state-0 anim-worker dispatch handler (the
// same pump archetype as Demo.cpp's Handler03d2b0 family; the class file carries
// its own handler here, text-adjacent to the ctor below - dossier #16). __cdecl
// FREE function; reads owner->m_7c (the worker) and pumps on the state tag.
RVA(0x0003a200, 0xf1)
i32 Handler03a200(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CCursorSnapSprite(reinterpret_cast<CGameObject*>(owner));
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

// CCursorSnapSprite::CCursorSnapSprite @0x03a340 - fold the shared CUserLogic(obj)
// init, name the bound object, snapshot its geometry id (+0x40), apply the single-
// image-ani geometry, bind the "A" bute node, then flag the sub-object (+0x08
// bit 2, +0x40 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0003a340, 0x16e)
CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_CURSORSNAPSPRITE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
}

// The class dispatch table filled by RegisterXLogic_62bfa0 (LogicActReg.cpp);
// referenced here for the slot-4 lookup.

// CCursorSnapSprite::FireActivation @0x03a5b0 - slot-4 (UserLogicVfunc2) override:
// resolve `id` in the class dispatch table; if the entry carries a handler,
// re-resolve and dispatch it __thiscall on `this`. Same archetype as
// CTeleporter::FireActivation (the ResolveEntry inline expands twice).
RVA(0x0003a5b0, 0x102)
void CCursorSnapSprite::FireActivation(i32 id) {
    CSnapActEntry* e = reinterpret_cast<CSnapActEntry*>(g_logicActReg_62bfa0.ResolveEntry(id));
    if (e->m_fn != 0) {
        CSnapActEntry* e2 = reinterpret_cast<CSnapActEntry*>(g_logicActReg_62bfa0.ResolveEntry(id));
        (this->*(e2->m_fn))();
    }
}

#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
