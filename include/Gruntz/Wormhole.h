#ifndef GRUNTZ_CWORMHOLE_H
#define GRUNTZ_CWORMHOLE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWormhole : public CUserLogic, public CWapX {
public:
    RVA(0x0003fed0, 0xa9)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }
        if (tag == SERIAL_POSTLOAD) {

            i32 kind = m_object->m_smarts;
            CShadeTable* color;
            if (kind == -1) {

                CLightFxMgr* pump = g_gameReg->m_logicPump;
                color = pump->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
            } else {
                color = g_gameReg->m_logicPump->m_tables[kind];
            }

            CWwdGameObjectA* s = m_object;
            s->m_drawActive = 1;
            s->m_drawFillCmd = SHADE_DST_BY_SRC_16;
            s->m_drawFillArg = color;
        }
        return 1;
    }
    RVA(0x00010930, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WORMHOLE;
    }

public:
    virtual void FireActivation(i32 id) OVERRIDE;

    CWormhole() {}
    CWormhole(CGameObject* obj);
    i32 SpawnPartners();
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*CActHandler)();

#endif // GRUNTZ_CWORMHOLE_H
