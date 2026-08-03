#ifndef GRUNTZ_CGRUNTHEALTHSPRITE_H
#define GRUNTZ_CGRUNTHEALTHSPRITE_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGrunt;

class CGruntHealthSprite : public CUserLogic, public CWapX {
public:
public:
    RVA(0x0007f270, 0xa3)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        switch (mode) {
            case SERIAL_SAVE:
                ar->Write(&m_cell, 8);
                ar->Write(&m_health, 4);
                ar->Write(&m_yOffset, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_cell, 8);
                ar->Read(&m_health, 4);
                ar->Read(&m_yOffset, 4);
                break;
        }
        if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        return Chain(ar, mode, typeId, pObj) != 0;
    }
    RVA(0x00011f60, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTHEALTHSPRITE;
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 HealthUpdate();
    i32 SetHealthGlyph(i32 x, i32 y, i32 health);

    virtual i32 GetDisplayedValue(CGrunt* grunt);
    CGruntHealthSprite();
    CGruntHealthSprite(CGameObject* obj);

    Coord m_cell;
    i32 m_health;
    i32 m_yOffset;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTHEALTHSPRITE_H
