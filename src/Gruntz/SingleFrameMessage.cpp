#include <Mfc.h>                  // RECT / CopyRect (the ctor centers the object in a bounds rect)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/WwdGameReg.h>    // g_gameReg->GetRect (on-screen message bounds)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <rva.h>
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

template<> DATA(0x00245ef0)
CActReg CActRegPool<CSingleFrameMessage>::s_table(2000, 2010);

RVA(0x0000f5a0, 0x47)
i32 CSingleFrameMessage::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

// CSingleFrameMessage::~CSingleFrameMessage @0x0f640 - empty vtable-anchor dtor;
// folds the CUserLogic teardown (the /GX leaf-dtor archetype).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CSingleFrameMessage() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x0000f610, 0x1e, ??_GCSingleFrameMessage@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f640, 0x44, ??1CSingleFrameMessage@@UAE@XZ)

RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_object->ApplyLookupSprite("GAME_MESSAGEZ", m_38->m_id);
    RECT bounds;
    RECT r;
    CopyRect(&r, g_gameReg->GetRect(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}

VTBL(CSingleFrameMessage, 0x001e864c);

RVA(0x000ab5b0, 0x102)
void CSingleFrameMessage::FireActivation(i32 id) {
    CSingleFrameActEntry* e = reinterpret_cast<CSingleFrameActEntry*>(
        CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)
    );
    if (e->m_fn != 0) {
        (this
             ->*(reinterpret_cast<CSingleFrameActEntry*>(
                 CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)
             ))
             ->m_fn)();
    }
}

// CSingleFrameMessage::RegisterActs @0x0ab710 - bind the class's per-frame handler
// (AdvanceAnim @0x0ab910) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ab710, 0x18d)
void CSingleFrameMessage::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        id = g_typeCounter;
        ActInsertId("A", id);
        CString* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (reinterpret_cast<CSingleFrameActEntry*>(
         CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)
     ))
        ->m_fn = static_cast<i32 (CUserLogic::*)()>(&CSingleFrameMessage::AdvanceAnim);
}


RVA(0x000ab910, 0x12)
i32 CSingleFrameMessage::AdvanceAnim() {
    m_38->m_flags |= 0x10000;
    return 0;
}
