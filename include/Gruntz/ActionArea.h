#ifndef GRUNTZ_CACTIONAREA_H
#define GRUNTZ_CACTIONAREA_H

#include <rva.h>

#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/UserLogic.h>

class CActionArea : public CUserLogic, public CWapX {
public:
public:
    CActionArea() {}
    CActionArea(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 ApplyColor(i32 owner);

    RVA(0x00007f80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ACTIONAREA;
    }

    RVA(0x00008600, 0xcd)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (ar == 0) {
            return 0;
        }
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }
        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_timestamp, 8);
                ar->Write(&m_duration, 8);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_timestamp, 8);
                ar->Read(&m_duration, 8);
                break;
        }
        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_phase, 4);
                break;
            case SERIAL_LOAD:
                ar->Read(&m_phase, 4);
                break;
        }
        return 1;
    }

    i32 Tick();

    i32 m_phase;
    union {
        struct {
            i64 m_timestamp;
            i64 m_duration;
        };
        CPairRecord m_timing;
    };
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*CActHandler)();
SIZE_UNKNOWN();

#endif // GRUNTZ_CACTIONAREA_H
