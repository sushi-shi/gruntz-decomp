#include <Gruntz/LevelTimeDtor.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>

#include <rva.h>

VTBL(CLevelTime, 0x001e801c);
RVA(0x000119b0, 0x47)
i32 CLevelTime::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x00011a20, 0x1e, ??_GCLevelTime@@UAEPAXI@Z)
RVA_COMPGEN(0x00011a50, 0x44, ??1CLevelTime@@UAE@XZ)

// @early-stop
RVA(0x0009b8b0, 0x18f)
CLevelTime::CLevelTime(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
}
