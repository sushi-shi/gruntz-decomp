// BehindCandy.cpp - the behind-candy eyecandy tile-logic game-object (C:\Proj\Gruntz).
//
// Two CBehindCandy methods, defined in ascending retail-RVA order:
//   GetTypeTag      @0x00fb70 - the 6-byte per-class logic-type id accessor (0x3f0).
//   ~CBehindCandy   @0x00fc30 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CBehindCandy : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/BehindCandy.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

// CBehindCandy::GetTypeTag (0x0000fb70) is now an inline member in the class header.

// CBehindCandy::Serialize @0x00fb90 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool (the retail neg/sbb/neg idiom). Byte-identical to
// CCursorSnapSprite::Serialize (0x011880) save the two call displacements.
RVA(0x0000fb90, 0x47)
i32 CBehindCandy::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, (CGameObject*)d) != 0;
}

// CBehindCandy::~CBehindCandy @0x00fc30 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CBehindCandy() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CBehindCandy@@UAE@XZ 0x0000fc30 0x44

// --- CBehindCandy (0x0ac3f0), vptr 0x5e8494 --- the ctor anchors GetTypeTag @0xfb70
// + the ??_7CBehindCandy vtable in this TU. Folds the inline CUserLogic(obj) base +
// the shared z-clamp tail.
RVA(0x000ac3f0, 0x1b1)
CBehindCandy::CBehindCandy(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    if (m_object->m_latchedAnimId != 0) {
        m_object->m_latchedAnimId = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != 0) {
        if (m_object->m_layer->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
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

#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
