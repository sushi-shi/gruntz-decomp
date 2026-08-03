#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

class CGruntPowerupSprite : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00080490, 0xbe)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        if (Chain(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        switch (mode) {
            case SERIAL_SAVE:
                ar->Write(&m_cell, 8);
                ar->Write(&m_powerupId, 4);
                break;
            case SERIAL_LOAD: {
                ar->Read(&m_cell, 8);
                ar->Read(&m_powerupId, 4);
                i32 id = m_powerupId;
                CWwdGameObjectA* r = m_object;
                CShadeTable* v = g_gameReg->m_logicPump->m_tables[id];
                r->m_drawActive = 1;
                r->m_drawFillArg = v;
                r->m_drawFillCmd = SHADE_DST_BY_SRC_16;
                break;
            }
        }
        return 1;
    }
    RVA(0x00012320, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTPOWERUPSPRITE;
    }
    CGruntPowerupSprite() {}
    CGruntPowerupSprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 SetCell(i32 x, i32 y, i32 powerup);
    i32 Update();

    Coord m_cell;
    i32 m_powerupId;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTPOWERUPSPRITE_H
