#ifndef GRUNTZ_COBJECTDROPPER_H
#define GRUNTZ_COBJECTDROPPER_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

GZ_ENUM_BEGIN(ObjectDropScope)
    OBJECT_DROP_ALL_PLAYERS = 0,
    OBJECT_DROP_PLAYER_ZERO_ONLY = 1
GZ_ENUM_END(ObjectDropScope)

class CObjectDropper : public CUserLogic, public CWapX {
public:
public:
    RVA(0x000124b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_OBJECTDROPPER;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
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
    i32 m_lastDropPlayerIndex;
    i32 m_lastDropUnitIndex;
    ObjectDropScope m_scrollMode;
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
