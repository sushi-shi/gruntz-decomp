#ifndef GRUNTZ_CUFO_H
#define GRUNTZ_CUFO_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>

class CUFO : public CPathHazard {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_UFO;
    }
    CUFO() {}
    CUFO(CGameObject* obj);

    virtual i32 Tick()  ;
};

#endif
