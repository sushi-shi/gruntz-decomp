#include <rva.h>

#include <Gruntz/GuardPoint.h>

#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>

RVA(0x00010370, 0x47)
i32 CGuardPoint::SerializeMove(CFileMemBase* a, i32 b, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x000103e0, 0x1e, ??_GCGuardPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010410, 0x44, ??1CGuardPoint@@UAE@XZ)
VTBL(CGuardPoint, 0x001e7154);

// @early-stop
RVA(0x000ae5f0, 0x18f)
CGuardPoint::CGuardPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_stateFlags |= 1;
}
