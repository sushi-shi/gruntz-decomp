#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CMenuSparkle : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_MENUSPARKLE;
    }

public:
    CMenuSparkle() {}
    CMenuSparkle(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    i32 AdvanceAnim();
};

#endif
