#include <rva.h>

#include <Gruntz/LevelTime.h>

#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>

RVA(0x000119b0, 0x47)
i32 CLevelTime::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}
