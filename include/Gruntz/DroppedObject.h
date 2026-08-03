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
    RVA(0x000c73a0, 0xb5)
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
                ar->Write(&m_timePerTile, 8);
                ar->Write(&m_fallY, 8);
                ar->Write(&m_landY, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_timePerTile, 8);
                ar->Read(&m_fallY, 8);
                ar->Read(&m_landY, 4);
                break;
        }
        return 1;
    }

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
SIZE_UNKNOWN();

extern const double g_objDropDiv;
extern double g_dropFallBias;
#endif // GRUNTZ_CDROPPEDOBJECT_H
