#include <rva.h>

#include <Gruntz/GuardPoint.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>

RVA(0x00010370, 0x47)
i32 CGuardPoint::SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x000103e0, 0x1e, ??_GCGuardPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010410, 0x44, ??1CGuardPoint@@UAE@XZ)

// @early-stop
// The vptr stamp and the body's first member re-read are transposed. Every body
// spelling is byte-identical (named local for the receiver, explicit read-modify-
// write, this->, a local for the flag word), as is the TU-state probe.
RVA(0x000ae5f0, 0x18f)
CGuardPoint::CGuardPoint(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
}
