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

RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    Hide();
}
