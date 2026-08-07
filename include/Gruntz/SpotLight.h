#ifndef GRUNTZ_CSPOTLIGHT_H
#define GRUNTZ_CSPOTLIGHT_H

#include <rva.h>

#include <Gruntz/DoubleVector.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSpotLight : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

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

#endif // GRUNTZ_CSPOTLIGHT_H
