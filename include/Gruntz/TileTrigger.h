#ifndef GRUNTZ_TILETRIGGER_H
#define GRUNTZ_TILETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CTileSecretTrigger : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CTileSecretTrigger() {}
    CTileSecretTrigger(CGameObject* obj);
};
SIZE(0x54);

class CGiantRock : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CGiantRock() {}
    CGiantRock(CGameObject* obj);
};
SIZE(0x54);

class CCoveredPowerup : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CCoveredPowerup() {}
    CCoveredPowerup(CGameObject* obj);
};
SIZE(0x54);

#endif // GRUNTZ_TILETRIGGER_H
