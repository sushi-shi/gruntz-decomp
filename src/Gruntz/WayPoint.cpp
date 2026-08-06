#include <rva.h>

#include <Gruntz/WayPoint.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>

RVA(0x00010240, 0x47)
i32 CWayPoint::SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x000102b0, 0x1e, ??_GCWayPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x000102e0, 0x44, ??1CWayPoint@@UAE@XZ)

// @early-stop
// The vptr stamp and the body's first member re-read are transposed. Every body
// spelling is byte-identical (named local for the receiver, explicit read-modify-
// write, this->, a local for the flag word), as is the TU-state probe.
RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
}
