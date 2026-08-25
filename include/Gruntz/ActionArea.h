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
    CActionArea() {
        m_timestamp = 0;
        m_duration = 0;
    }
    CActionArea(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 ApplyColor(i32 owner);

    RVA(0x00007f80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ACTIONAREA;
    }

    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

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

typedef i32 (CUserLogic::*CActHandler)();

#endif // GRUNTZ_CACTIONAREA_H
