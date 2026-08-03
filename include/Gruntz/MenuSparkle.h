#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CMenuSparkle : public CUserLogic, public CWapX {
public:
    RVA(0x000ae1c0, 0xae)
    virtual i32
    SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
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
    RVA(0x00010160, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_MENUSPARKLE;
    }

public:
    CMenuSparkle() {}
    CMenuSparkle(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 AdvanceAnim();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CMENUSPARKLE_H
