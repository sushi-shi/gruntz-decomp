#ifndef GRUNTZ_CDONOTHINGNORMAL_H
#define GRUNTZ_CDONOTHINGNORMAL_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CDoNothingNormal : public CUserLogic, public CWapX {
public:
    CDoNothingNormal() : CUserLogic(CUserLogic::INLINE_BASE) {}

    CDoNothingNormal(CGameObject* owner) : CUserLogic(owner), CWapX(owner) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_DONOTHINGNORMAL;
    }

public:
};

#endif
