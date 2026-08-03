#ifndef GRUNTZ_CSPOTLIGHT_H
#define GRUNTZ_CSPOTLIGHT_H

#include <rva.h>

#include <Gruntz/DoubleVector.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSpotLight : public CUserLogic, public CWapX {
public:
    RVA(0x000b2050, 0x295)
    virtual i32 SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (CUserLogic::SerializeMove(arc, mode, c, d) == 0) {
            return 0;
        }
        if (Chain(static_cast<CFileMemBase*>(arc), mode, c, d) == 0) {
            return 0;
        }
        CGruntzMgr* reg = g_gameReg;
        CFileMemBase* s = static_cast<CFileMemBase*>(arc);
        switch (mode) {
            case SERIAL_SAVE:
                s->Write(&m_angularVelocity, 8);
                s->Write(&m_position.x, 8);
                s->Write(&m_position.y, 8);
                s->Write(&m_center.x, 8);
                s->Write(&m_center.y, 8);
                s->Write(&m_offset.x, 8);
                s->Write(&m_offset.y, 8);
                s->Write(&m_angle, 8);
                g_serialCounter++;
                {
                    i32 id = 0;
                    if (m_focus != 0) {
                        id = m_focus->m_objectId;
                    }
                    s->Write(&id, 4);
                }
                s->Write(&m_cellRow, 4);
                s->Write(&m_cellCol, 4);
                s->Write(&m_storyMode, 4);
                break;
            case SERIAL_LOAD:
                s->Read(&m_angularVelocity, 8);
                s->Read(&m_position.x, 8);
                s->Read(&m_position.y, 8);
                s->Read(&m_center.x, 8);
                s->Read(&m_center.y, 8);
                s->Read(&m_offset.x, 8);
                s->Read(&m_offset.y, 8);
                s->Read(&m_angle, 8);
                g_serialCounter++;
                {
                    i32 id;
                    s->Read(&id, 4);
                    CGameObject* out = 0;
                    CGameObject* resolved;
                    if (MapLookupById(reg->m_world->m_childGroup->m_map48, id, out) == 0) {
                        resolved = 0;
                    } else if (out == 0) {
                        resolved = 0;
                    } else {
                        resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
                    }
                    m_focus = static_cast<CWwdGameObjectA*>(resolved);
                    if (m_focus == 0 && id != 0) {
                        return 0;
                    }
                }
                s->Read(&m_cellRow, 4);
                s->Read(&m_cellCol, 4);
                s->Read(&m_storyMode, 4);
                break;
            case SERIAL_POSTLOAD: {
                CWwdGameObjectA* o = m_object;
                CShadeTable* fill = reg->m_logicPump->m_tables[o->m_powerup];
                o->m_drawActive = 1;
                o->m_drawFillArg = fill;
                o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
                break;
            }
        }
        return 1;
    }

    RVA(0x00012ff0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SPOTLIGHT;
    }

public:
    CSpotLight() {}
    CSpotLight(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 Tick();

    int Update();

    char m_pad54[0x58 - 0x54];
    double m_angularVelocity;
    DoubleVector2 m_position;
    DoubleVector2 m_center;
    DoubleVector2 m_offset;
    double m_angle;
    CWwdGameObjectA* m_focus;

    i32 m_cellRow;
    i32 m_cellCol;
    i32 m_storyMode;
};
SIZE(0xa8);

extern u8 g_randSeeded;
extern i32 g_randSeed;
#endif // GRUNTZ_CSPOTLIGHT_H
