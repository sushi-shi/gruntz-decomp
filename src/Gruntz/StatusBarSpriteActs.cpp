#include <Gruntz/ActNameRegistry.h>       // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>                // the shared CActReg coordinate-registry archetype
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/UserLogic.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

#include <Rez/FrameClock.h> // g_engineFrameDelta (the anim-advance clock)
#include <rva.h>
#include <Wap32/ZVec.h>

#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/SerialArchive.h>       // the serialize stream (== the real CFileMemBase)
#include <Gruntz/StatusBarSpriteActs.h> // CActRegPool<CStatusBarSprite>::s_table decl

// CActRegPool<CStatusBarSprite>::s_table (0x0024e670): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x0024e670)
CActReg CActRegPool<CStatusBarSprite>::s_table(2000, 2010);

VTBL(CStatusBarSprite, 0x001e7fc4);

// CStatusBarSprite::~CStatusBarSprite @0x11b80 - empty vtable-anchor dtor; folds the
// CUserLogic teardown (the /GX leaf-dtor archetype). Adjacent to SerializeMove (0x11ae0).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CStatusBarSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00011b80, 0x44, ??1CStatusBarSprite@@UAE@XZ)

RVA(0x0010c0f0, 0xf1)
extern "C" i32 CreateStatusBarSprite(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (reinterpret_cast<u32>(ctl->m_1c)) {
        case 0: {
            ctl->m_1c = reinterpret_cast<void*>(0x3e8);
            CStatusBarSprite* t = new CStatusBarSprite(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            ctl->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            ctl->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            ctl->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            ctl->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            ctl->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

// CStatusBarSprite::CStatusBarSprite @0x10c230 - fold the shared CUserLogic(obj)
// init, name the bound object "GAME_STATUSBARSPRITE", snapshot the geometry id,
// apply the single-image-ani geometry, bind the "A" bute node and lock the draw
// order to 0xf4240.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0010c230, 0x178)
CStatusBarSprite::CStatusBarSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_STATUSBARSPRITE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_object->m_sortKey != 0xf4240) {
        m_object->m_sortKey = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
}

RVA_COMPGEN(0x0010c410, 0xa, _$E1098768)
RVA_COMPGEN(0x0010c430, 0x15, _$E1098800)
RVA_COMPGEN(0x0010c460, 0xe, _$E1098848)
RVA_COMPGEN(0x0010c480, 0x1f, _$E1098880)

RVA(0x0010c4b0, 0x102)
void CStatusBarSprite::FireActivation(i32 coord) {
    CStatusBarSpriteActEntry* e = reinterpret_cast<CStatusBarSpriteActEntry*>(
        CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(coord)
    );
    if (e->m_fn != 0) {
        CStatusBarSpriteActEntry* e2 = reinterpret_cast<CStatusBarSpriteActEntry*>(
            CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(coord)
        );
        (this->*(e2->m_fn))();
    }
}

// CStatusBarSprite::RegisterActs @0x10c610 - bind the per-frame handler (AdvanceAnim
// @0x10c810) to the activation key "A" via the shared name registry. The SAME
// archetype as CWarpStonePad::RegisterActs, driving the status-bar-sprite registry.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0010c610, 0x18d)
void CStatusBarSprite::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CStatusBarSpriteActEntry*>(
         CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(id)
     ))
        ->m_fn = static_cast<i32 (CUserLogic::*)()>(&CStatusBarSprite::AdvanceAnim);
}

RVA(0x0010c810, 0x17)
i32 CStatusBarSprite::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x00011ac0, 0x6)
LogicTypeId CStatusBarSprite::GetTypeTag() {
    return LOGIC_STATUSBARSPRITE;
}

RVA(0x00011ae0, 0x47)
i32 CStatusBarSprite::SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove(
            reinterpret_cast<CFileMemBase*>((reinterpret_cast<i32>(ar))),
            mode,
            a3,
            a4
        )) {
        return 0;
    }
    return Chain(static_cast<CFileMemBase*>(ar), mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}
