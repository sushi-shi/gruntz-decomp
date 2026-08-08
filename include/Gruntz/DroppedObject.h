#ifndef GRUNTZ_CDROPPEDOBJECT_H
#define GRUNTZ_CDROPPEDOBJECT_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CDroppedObject : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

    i32 AdvanceImpactAnimation();
    RVA(0x00012560, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DROPPEDOBJECT;
    }
    virtual i32 AdvanceAnimation() OVERRIDE;

public:
    CDroppedObject() {}
    CDroppedObject(CGameObject* obj);
    static void RegisterActs();
    virtual void FireActivation(i32 id) OVERRIDE;
    i32 AdvanceFall();

    char m_pad54[0x58 - 0x54];
    double m_timePerTile;
    double m_fallY;
    i32 m_landY;
};
SIZE(0x70);

extern const double g_objDropDiv;
extern double g_dropFallBias;
#endif // GRUNTZ_CDROPPEDOBJECT_H
