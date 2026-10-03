#ifndef GRUNTZ_TILETRIGGER_H
#define GRUNTZ_TILETRIGGER_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CTileSecretTrigger : public CTileTrigger {
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_TILESECRETTRIGGER;
    }
    CTileSecretTrigger() {}
    CTileSecretTrigger(CGameObject* obj);
};

class CGiantRock : public CTileTrigger {
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GIANTROCK;
    }
    CGiantRock() {}
    CGiantRock(CGameObject* obj);
};

class CCoveredPowerup : public CTileTrigger {
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_COVEREDPOWERUP;
    }
    CCoveredPowerup() : CTileTrigger(CUserLogic::INLINE_BASE) {}
    CCoveredPowerup(CGameObject* obj);
};

#endif
