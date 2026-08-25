#ifndef GRUNTZ_CDONOTHINGNORMAL_H
#define GRUNTZ_CDONOTHINGNORMAL_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CDoNothingNormal : public CUserLogic, public CWapX {
public:
    CDoNothingNormal() : CUserLogic(CUserLogic::INLINE_BASE) {}

    CDoNothingNormal(CGameObject* owner) : CUserLogic(owner), CWapX(owner) {
        SetObjectFlags(1);
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x0000f7e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DONOTHINGNORMAL;
    }

public:
};

#endif // GRUNTZ_CDONOTHINGNORMAL_H
