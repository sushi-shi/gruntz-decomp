#ifndef GRUNTZ_CDONOTHING_H
#define GRUNTZ_CDONOTHING_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CDoNothing : public CUserLogic, public CWapX {
public:
public:
    CDoNothing() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CDoNothing(CGameObject* obj);

    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DONOTHING;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
};

#endif // GRUNTZ_CDONOTHING_H
