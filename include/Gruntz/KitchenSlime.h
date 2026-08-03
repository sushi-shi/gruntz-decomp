#ifndef GRUNTZ_CKITCHENSLIME_H
#define GRUNTZ_CKITCHENSLIME_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CKitchenSlime : public CUserLogic, public CWapX {
public:
    RVA(0x000b2ff0, 0x11b)
    virtual i32 SerializeMove(CFileMemBase* stream, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        CFileMemBase* s = stream;

        if (tag != SERIAL_SAVE) {
            if (tag == SERIAL_LOAD) {
                s->Read(&m_speed, 8);
                s->Read(&m_posX, 8);
                s->Read(&m_posY, 8);
                s->Read(&m_dirX, 8);
                s->Read(&m_dirY, 8);
                s->Read(&m_tilePosition, 8);
                s->Read(&m_stepMag, 8);
            }
        } else {
            s->Write(&m_speed, 8);
            s->Write(&m_posX, 8);
            s->Write(&m_posY, 8);
            s->Write(&m_dirX, 8);
            s->Write(&m_dirY, 8);
            s->Write(&m_tilePosition, 8);
            s->Write(&m_stepMag, 8);
        }
        if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
            return 0;
        }
        return Chain(stream, tag, c, d) != 0;
    }
    RVA(0x000130b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_KITCHENSLIME;
    }

public:
    static void RegisterType();
    virtual void FireActivation(i32 id) OVERRIDE;
    i32 Tick();
    i32 LoadSprites();
    CKitchenSlime() {}
    CKitchenSlime(CGameObject* obj);

    CGameObject* Level() {
        return m_object;
    }
    CWwdGameObjectA* Anim() {
        return m_wwdObject;
    }
    char m_pad54[0x58 - 0x54];
    double m_speed;
    double m_posX;
    double m_posY;
    double m_dirX;
    double m_dirY;
    Coord m_tilePosition;
    double m_stepMag;
};
SIZE(0x90);

extern const double g_slimeSpeedNum;
#endif // GRUNTZ_CKITCHENSLIME_H
