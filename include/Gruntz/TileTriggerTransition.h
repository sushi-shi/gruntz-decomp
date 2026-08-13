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
    RVA(0x00011750, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(ar, mode, typeId, pObj) != 0;
    }

public:
    CTileTriggerTransition() {}
    CTileTriggerTransition(CGameObject* obj);

    RVA(0x00011730, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILETRIGGERTRANSITION;
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 ApplyAnimation(char* sprite, char* geom);
    i32 TransitionAct();
};

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
