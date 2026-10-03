#ifndef GRUNTZ_CDROPPEDOBJECT_H
#define GRUNTZ_CDROPPEDOBJECT_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CDroppedObject : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    i32 AdvanceImpactAnimation();

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_DROPPEDOBJECT;
    }
    virtual i32 AdvanceAnimation()  ;

public:
    CDroppedObject() {}
    CDroppedObject(CGameObject* obj);
    static void RegisterActs();
    virtual void FireActivation(i32 id)  ;
    i32 AdvanceFall();

    double m_timePerTile;
    double m_fallY;
    i32 m_landY;
};

extern const double g_objDropDiv;
extern const double g_dropFallBias;
#endif
