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
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
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
