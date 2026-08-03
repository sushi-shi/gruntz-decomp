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
SIZE_UNKNOWN();

extern "C" u32 g_engineFrameDelta;

extern char g_puddleSpriteKey[];

class CGruntPuddle : public CUserLogic, public CWapX {
public:
    RVA(0x00040e50, 0x170)
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
                ar->Write(&m_tileX, 4);
                ar->Write(&m_tileY, 4);
                ar->Write(&m_pending, 4);
                ar->Write(&m_placed, 4);
                ar->Write(&m_placeArg3, 4);
                ar->Write(&m_gruntType, 4);
                ar->Write(&m_placeIndex, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_tileX, 4);
                ar->Read(&m_tileY, 4);
                ar->Read(&m_pending, 4);
                ar->Read(&m_placed, 4);
                ar->Read(&m_placeArg3, 4);
                ar->Read(&m_gruntType, 4);
                ar->Read(&m_placeIndex, 4);
                break;
            case SERIAL_POSTLOAD: {
                CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(m_placeIndex, 0);
                if (sel == 0) {
                    sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                }
                CGameObject* obj = m_object;
                obj->m_drawFillArg = sel;
                obj->m_drawActive = 1;
                obj->m_drawFillCmd = SHADE_PAL_16;
                break;
            }
        }
        return 1;
    }
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
SIZE_UNKNOWN();

SIZE_UNKNOWN();

extern "C" i32 CellTargetable(i32 col, i32 row);

#endif // GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
