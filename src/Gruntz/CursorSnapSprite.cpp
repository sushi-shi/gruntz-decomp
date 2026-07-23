#include <Gruntz/CursorSnapSprite.h>
#include <Bute/ButeTree.h> // g_buteTree

#include <Gruntz/AnimWorker.h> // shared Owner / Worker views + Worker_DefaultPump (CursorSnapWorkerPump)
#include <Gruntz/UserLogic.h> // the dispatched CUserLogic slot layout

RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
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
#include <rva.h>
RVA_COMPGEN(0x00011920, 0x44, ??1CCursorSnapSprite@@UAE@XZ)

RVA(0x0003a200, 0xf1)
i32 CursorSnapWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CCursorSnapSprite(owner);
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

RVA(0x0003a5b0, 0x102)
void CCursorSnapSprite::FireActivation(i32 id) {
    CSnapActEntry* e =
        reinterpret_cast<CSnapActEntry*>(CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
    if (e->m_fn != 0) {
        CSnapActEntry* e2 = reinterpret_cast<CSnapActEntry*>(
            CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id)
        );
        (this->*(e2->m_fn))();
    }
}

#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
VTBL(CCursorSnapSprite, 0x001e8074);
