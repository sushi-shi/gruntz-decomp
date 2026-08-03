#include <rva.h>

#include <Gruntz/WayPoint.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>

RVA_COMPGEN(0x000102b0, 0x1e, ??_GCWayPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x000102e0, 0x44, ??1CWayPoint@@UAE@XZ)
VTBL(CWayPoint, 0x001e74b4);

// @early-stop
RVA(0x000ae3f0, 0x18f)
CWayPoint::CWayPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_stateFlags |= 1;
}
