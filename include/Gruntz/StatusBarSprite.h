#ifndef GRUNTZ_CSTATUSBARSPRITE_H
#define GRUNTZ_CSTATUSBARSPRITE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CStatusBarSprite : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE;

public:
    CStatusBarSprite() {}
    CStatusBarSprite(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};
SIZE(0x54);

SIZE_UNKNOWN();

#endif // GRUNTZ_CSTATUSBARSPRITE_H
