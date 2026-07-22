#include <Gruntz/DoNothing.h>
#include <Gruntz/DoNothingNormalDtor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)

VTBL(CDoNothingNormal, 0x1e859c);
VTBL(CDoNothing, 0x001e85f4); // vtable_names -> code (RTTI game class)
RVA(0x0000f6d0, 0x47)
i32 CDoNothing::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CDoNothing::~CDoNothing @0x00f770 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call
// 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible link forces the
// /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70; the empty body is
// enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CDoNothing() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x0000f770, 0x44, ??1CDoNothing@@UAE@XZ)

RVA(0x0000f800, 0x47)
i32 CDoNothingNormal::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CDoNothingNormal::~CDoNothingNormal @0x0000f8a0 - folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical to ~CDoNothing
// @0x0000f770.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CDoNothingNormal() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x0000f8a0, 0x44, ??1CDoNothingNormal@@UAE@XZ)

CDoNothingNormal* RealizeCDoNothingNormal();
CDoNothingNormal* RealizeCDoNothingNormal() {
    return new CDoNothingNormal();
}

// CDoNothing::CDoNothing @0xac1d0 - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), flag the sub-object, then run the
// shared BigActHeight "big-act" de-prioritize tail (the SAME archetype as
// CSimpleAnimation / CEyeCandy).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ac1d0, 0x1a5)
CDoNothing::CDoNothing(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 1;
    CImage* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}
