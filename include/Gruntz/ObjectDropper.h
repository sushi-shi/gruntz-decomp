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
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    CObjectDropper() {
        m_lastDropTime = 0;
        m_dropInterval = 0;
    }
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

#endif // GRUNTZ_COBJECTDROPPER_H
