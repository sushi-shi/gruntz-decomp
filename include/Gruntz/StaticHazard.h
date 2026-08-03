#ifndef GRUNTZ_CSTATICHAZARD_H
#define GRUNTZ_CSTATICHAZARD_H

#include <rva.h>

#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CStaticHazard : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00012ae0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_STATICHAZARD;
    }
    RVA(0x000fc5b0, 0xf5)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        CFileMemBase* arc = ar;
        switch (mode) {
            case SERIAL_SAVE:
                arc->Write(&m_pulseEpoch, 4);
                arc->Write(&m_activeWindow, 4);
                arc->Write(&m_idleWindow, 4);
                arc->Write(&m_fired, 4);
                arc->Write(&m_tileCol, 4);
                arc->Write(&m_tileRow, 4);
                break;
            case SERIAL_LOAD:
                arc->Read(&m_pulseEpoch, 4);
                arc->Read(&m_activeWindow, 4);
                arc->Read(&m_idleWindow, 4);
                arc->Read(&m_fired, 4);
                arc->Read(&m_tileCol, 4);
                arc->Read(&m_tileRow, 4);
                break;
        }
        if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(arc, mode, typeId, pObj) != 0;
    }
    CStaticHazard() {}
    CStaticHazard(CGameObject* obj);
    static void RegisterActs();
    i32 LoadAttributes2();
    i32 LoadAttributes();
    virtual void FireActivation(i32 id) OVERRIDE;

    u32 m_pulseEpoch;
    i32 m_activeWindow;
    i32 m_idleWindow;
    i32 m_fired;
    i32 m_tileCol;
    i32 m_tileRow;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CSTATICHAZARD_H
