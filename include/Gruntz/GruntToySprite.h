#ifndef GRUNTZ_CGRUNTTOYSPRITE_H
#define GRUNTZ_CGRUNTTOYSPRITE_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>

class CGruntToySprite : public CUserLogic, public CWapX {
public:
public:
    RVA(0x0007fa20, 0x89)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        switch (mode) {
            case SERIAL_SAVE:
                ar->Write(&m_cell, 8);
                ar->Write(&m_lastLayer, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_cell, 8);
                ar->Read(&m_lastLayer, 4);
                break;
        }
        if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        return Chain(ar, mode, typeId, pObj) != 0;
    }
    RVA(0x00012260, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYSPRITE;
    }
    CGruntToySprite() {}
    CGruntToySprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 SetCell(i32 x, i32 y);
    i32 Update();

    Coord m_cell;
    PickupType m_lastLayer;
};
SIZE(0x60);

#endif // GRUNTZ_CGRUNTTOYSPRITE_H
