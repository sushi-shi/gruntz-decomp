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

class CGiantRock : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CGiantRock() {}
    CGiantRock(CGameObject* obj);
};

class CCoveredPowerup : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE;
    CCoveredPowerup() : CTileTrigger(CUserLogic::INLINE_BASE) {}
    CCoveredPowerup(CGameObject* obj);
};

#endif // GRUNTZ_TILETRIGGER_H
