#ifndef GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
#define GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

struct CGruntPuddleSink {};

extern char g_puddleSpriteKey[];

class CGruntPuddle : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00010cc0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTPUDDLE;
    }

public:
    CGruntPuddle() {}
    CGruntPuddle(CGameObject* obj);

    i32 Idle();
    i32 Place(i32 gruntType, i32 placeIndex, i32 color, i32 a3);
    i32 Remove();
    void SetBute(char* key);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 m_tileX;
    i32 m_tileY;
    i32 m_pending;

    i32 m_placed;
    i32 m_placeArg3;
    i32 m_gruntType;

    i32 m_placeIndex;
};

extern "C" i32 CellTargetable(i32 col, i32 row);

#endif // GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
