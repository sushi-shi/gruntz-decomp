// DoNothing.cpp - the inert "do nothing" tile-logic game-object family
// (C:\Proj\Gruntz): CDoNothing and its sibling CDoNothingNormal.
//
// One dev TU, formerly split across DoNothing.cpp + DoNothingNormalDtor.cpp +
// DoNothingNormalLogic.cpp (all /GX, all this family). Methods in ascending
// retail-RVA order:
//   CDoNothing::GetTypeTag         @0x00f6b0 - 6-byte logic-type id accessor (0x3ec)
//   CDoNothing::~CDoNothing        @0x00f770 - /GX leaf dtor (CUserLogic teardown)
//   CDoNothingNormal::~CDoNothingNormal @0x00f8a0 - /GX leaf dtor
//   (the DoNothingNormal pump HandlerA9E00 @0x0a9e00 lives in its retail TU,
//    LogicWorkerHandlers.cpp - wave2-H)
//   CDoNothing::CDoNothing         @0x0ac1d0 - the ctor (BigActHeight de-prioritize)
//
// CDoNothing / CDoNothingNormal : CUserLogic (base hierarchy from
// <Gruntz/UserLogic.h>). Only offsets / code bytes are load-bearing; names are
// placeholders for the recovered engine identities.
#include <Gruntz/DoNothing.h>
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/DoNothingNormalDtor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// CDoNothing::GetTypeTag (0x0000f6b0) is now an inline member in the class header.

// CDoNothing::Serialize @0x00f6d0 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, then (on success) the +0x34 sub-object's
// chain; normalize to a bool. Byte-identical to CEyeCandy::Serialize (0x00fcc0).
RVA(0x0000f6d0, 0x47)
i32 CDoNothing::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CDoNothing::~CDoNothing @0x00f770 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call
// 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible link forces the
// /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70; the empty body is
// enough for cl.
RVA(0x0000f770, 0x44)
CDoNothing::~CDoNothing() {}

// CDoNothingNormal::Serialize @0x00f800 - the vtable slot-1 override (same shape as
// CDoNothing::Serialize): base CUserLogic chain + the +0x34 sub-object chain.
RVA(0x0000f800, 0x47)
i32 CDoNothingNormal::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CDoNothingNormal::~CDoNothingNormal @0x0000f8a0 - folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical to ~CDoNothing
// @0x0000f770.
RVA(0x0000f8a0, 0x44)
CDoNothingNormal::~CDoNothingNormal() {}

// Realize ??_7CDoNothingNormal@@6B@ (0x1e859c): retail's dtor folds straight to the
// CUserLogic teardown and never references the leaf vtable (so ~CDoNothingNormal only
// emits the base ??_7CUserLogic/??_7CUserBase restamps), and the logic-worker ctor
// stamps the leaf vtable vptr-MIDDLE - neither anchors the leaf COMDAT. A spurious
// `new CDoNothingNormal` references the implicit vptr-FIRST leaf ctor, whose stamp
// (the escaping object keeps it) emits ??_7CDoNothingNormal. Unpaired (no RVA) ->
// matching-neutral; it does NOT touch the 0xf8a0 dtor codegen.
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
CDoNothing::CDoNothing(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 1;
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
