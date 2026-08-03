#include <rva.h>

#include <Gruntz/MenuSparkleSerial.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

DATA(0x001ea3d4)
i32 g_menuSparkleLo = 1000;
DATA(0x001ea3d8)
i32 g_menuSparkleHi = 5000;

RVA(0x000ae1c0, 0xae)
i32 CMenuSparkle::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (arc == 0) {
        return 0;
    }

    if (!CUserLogic::SerializeMove(static_cast<CFileMemBase*>(arc), mode, typeId, pObj)) {
        return 0;
    }
    if (!Chain(static_cast<CFileMemBase*>(arc), mode, typeId, pObj)) {
        return 0;
    }
    if (mode != SERIAL_SAVE) {
        if (mode != SERIAL_LOAD) {
            return 1;
        }
        arc->Read(&g_menuSparkleLo, 4);
        arc->Read(&g_menuSparkleHi, 4);
        return 1;
    }
    arc->Write(&g_menuSparkleLo, 4);
    arc->Write(&g_menuSparkleHi, 4);
    return 1;
}
