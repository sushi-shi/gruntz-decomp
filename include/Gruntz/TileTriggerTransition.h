#ifndef GRUNTZ_TILETRIGGERTRANSITION_H
#define GRUNTZ_TILETRIGGERTRANSITION_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>
#include <Ints.h>

class CTileTriggerTransition : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;

public:
    CTileTriggerTransition() {}
    CTileTriggerTransition(CGameObject* obj);

    virtual LogicTypeId GetTypeTag() OVERRIDE;
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 ApplyAnimation(char* sprite, char* geom);
    i32 TransitionAct();
};

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
