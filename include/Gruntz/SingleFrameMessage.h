#ifndef GRUNTZ_CSINGLEFRAMEMESSAGE_H
#define GRUNTZ_CSINGLEFRAMEMESSAGE_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSingleFrameMessage : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x0000f590, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SINGLEFRAMEMESSAGE;
    }

public:
    CSingleFrameMessage() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CSingleFrameMessage(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();
};

#endif // GRUNTZ_CSINGLEFRAMEMESSAGE_H
