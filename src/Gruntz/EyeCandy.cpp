// EyeCandy.cpp - the eyecandy tile-logic game-object (C:\Proj\Gruntz).
//
// Two CEyeCandy methods, defined in ascending retail-RVA order:
//   GetTypeTag    @0x00fca0 - the 6-byte per-class logic-type id accessor (0x3f1).
//   ~CEyeCandy    @0x00fd60 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CEyeCandy : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/EyeCandy.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)

// CEyeCandy::GetTypeTag (0x0000fca0) is now an inline member in the class header.

// CEyeCandy::Serialize @0x00fcc0 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool. Byte-identical to CCursorSnapSprite::Serialize (0x011880)
// save the two call displacements.
RVA(0x0000fcc0, 0x47)
i32 CEyeCandy::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, (CGameObject*)d) != 0;
}

// CEyeCandy::~CEyeCandy @0x00fd60 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call
// 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible link forces the
// /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70; the empty body is
// enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CEyeCandy() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CEyeCandy@@UAE@XZ 0x0000fd60 0x44

// --- CEyeCandy (0x0ac620), vptr 0x5e843c --- the ctor is the vtable-emission anchor
// for this class (GetTypeTag @0xfca0 + the ??_7 vtable emit in this TU because the
// ctor lives here). Folds the inline CUserLogic(obj) base + the shared z-clamp tail.
RVA(0x000ac620, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    CGameObject* o = m_object;
    if (o->m_latchedAnimId == 0 && o->m_layer != 0) {
        i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
        if (o->m_latchedAnimId != v) {
            o->m_latchedAnimId = v;
            o->m_flags |= 0x20000;
        }
    }
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

#include <rva.h>
