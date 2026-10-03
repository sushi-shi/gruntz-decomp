#ifndef GRUNTZ_CSINGLEFRAMEMESSAGE_H
#define GRUNTZ_CSINGLEFRAMEMESSAGE_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSingleFrameMessage : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_SINGLEFRAMEMESSAGE;
    }

public:
    CSingleFrameMessage() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CSingleFrameMessage(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    static void RegisterActs();

    i32 AdvanceAnim();
};

#endif
