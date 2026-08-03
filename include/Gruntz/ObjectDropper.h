#ifndef GRUNTZ_COBJECTDROPPER_H
#define GRUNTZ_COBJECTDROPPER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CObjectDropper : public CUserLogic, public CWapX {
public:
public:
    RVA(0x000124a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_OBJECTDROPPER;
    }
    RVA(0x000c6680, 0x1b4)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }

        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_lastDropTime, 8);
                ar->Write(&m_dropInterval, 8);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_lastDropTime, 8);
                ar->Read(&m_dropInterval, 8);
                break;
        }

        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_speed, 8);
                ar->Write(&m_posX, 8);
                ar->Write(&m_posY, 8);
                ar->Write(&m_travelDx, 4);
                ar->Write(&m_travelDy, 4);
                ar->Write(&m_lastDropTileX, 4);
                ar->Write(&m_lastDropTileY, 4);
                ar->Write(&m_scrollMode, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_speed, 8);
                ar->Read(&m_posX, 8);
                ar->Read(&m_posY, 8);
                ar->Read(&m_travelDx, 4);
                ar->Read(&m_travelDy, 4);
                ar->Read(&m_lastDropTileX, 4);
                ar->Read(&m_lastDropTileY, 4);
                ar->Read(&m_scrollMode, 4);
                break;
            case SERIAL_POSTLOAD: {
                CShadeTable* fill = g_gameReg->m_logicPump->m_tables[5];
                CWwdGameObjectA* o = m_object;
                o->m_drawActive = 1;
                o->m_drawFillArg = fill;
                o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
                break;
            }
        }
        return 1;
    }
    CObjectDropper() {}
    CObjectDropper(CGameObject* obj);

    i32 Update();
    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    char m_pad54[0x58 - 0x54];
    double m_speed;
    double m_posX;
    double m_posY;
    i32 m_travelDx;
    i32 m_travelDy;
    i32 m_lastDropTileX;
    i32 m_lastDropTileY;
    i32 m_scrollMode;
    char m_pad84[0x88 - 0x84];
    union {
        struct {
            i64 m_lastDropTime;
            i64 m_dropInterval;
        };
        CPairRecord m_dropTiming;
    };
};
SIZE(0x98);

#endif // GRUNTZ_COBJECTDROPPER_H
