#include <Gruntz/WayPoint.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <rva.h>

RVA(0x00010240, 0x47)
i32 CWayPoint::SerializeMove(CFileMemBase* a, i32 b, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x000102b0, 0x1e, ??_GCWayPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x000102e0, 0x44, ??1CWayPoint@@UAE@XZ)
VTBL(CWayPoint, 0x001e74b4);

// @early-stop
RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_stateFlags |= 1;
}
