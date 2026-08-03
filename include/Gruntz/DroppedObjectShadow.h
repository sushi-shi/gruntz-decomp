#ifndef GRUNTZ_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_CDROPPEDOBJECTSHADOW_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CDroppedObjectShadow : public CUserLogic, public CWapX {
public:
    RVA(0x000c7b40, 0x76)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, mode, c, d)) {
            return 0;
        }
        if (!Chain(ar, mode, c, d)) {
            return 0;
        }
        if (mode == SERIAL_POSTLOAD) {
            CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
            CWwdGameObjectA* o = m_object;
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
            o->m_drawFillArg = fill;
        }
        return 1;
    }
    RVA(0x00012620, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DROPPEDOBJECTSHADOW;
    }

public:
    CDroppedObjectShadow() {}
    CDroppedObjectShadow(CGameObject* obj);

    i32 Serialize(CFileMemBase* ar, SerialMode tag, LogicTypeId c, i32 d);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 Advance();
};
SIZE(0x54);

#endif // GRUNTZ_CDROPPEDOBJECTSHADOW_H
