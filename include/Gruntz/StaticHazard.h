#ifndef GRUNTZ_CSTATICHAZARD_H
#define GRUNTZ_CSTATICHAZARD_H

#include <Ints.h>

#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CStaticHazard : public CUserLogic, public CWapX {
public:
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_STATICHAZARD;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
    CStaticHazard() {}
    CStaticHazard(CGameObject* obj);
    static void RegisterActs();
    i32 UpdateIdleState();
    i32 UpdateActiveState();
    virtual void FireActivation(i32 id)  ;

    u32 m_pulseEpoch;
    i32 m_activeWindow;
    i32 m_idleWindow;
    b32 m_fired;
    i32 m_tileCol;
    i32 m_tileRow;
};

i32 DispatchStaticHazardLogic(CGameObject* obj);

#endif
