#ifndef GRUNTZ_CSPOTLIGHT_H
#define GRUNTZ_CSPOTLIGHT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CSpotLight : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;

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
    double m_58;
    double m_60;
    double m_68;
    double m_70;
    double m_78;
    double m_80;
    double m_88;
    double m_90;
    CWwdGameObjectA* m_focus;

    i32 m_9c;
    i32 m_a0;
    i32 m_a4;
};
SIZE(0xa8);

extern "C" void* Probe_32ce(i32 x, i32 y, void* rect, i32* outA, i32* outB, i32 flag);
extern "C" void Activate_4322(void* target, i32 f);
extern u8 g_randSeeded;
extern i32 g_randSeed;
#endif // GRUNTZ_CSPOTLIGHT_H
