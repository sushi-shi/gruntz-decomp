#ifndef GRUNTZ_TILETRIGGER_H
#define GRUNTZ_TILETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CTileSecretTrigger : public CTileTrigger {
public:
    RVA(0x000114f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILESECRETTRIGGER;
    }
    CTileSecretTrigger() {}
    CTileSecretTrigger(CGameObject* obj);
};
SIZE_UNKNOWN();

class CGiantRock : public CTileTrigger {
public:
    RVA(0x000115b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GIANTROCK;
    }
    CGiantRock() {}
    CGiantRock(CGameObject* obj);
};
SIZE_UNKNOWN();

class CCoveredPowerup : public CTileTrigger {
public:
    RVA(0x00011670, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_COVEREDPOWERUP;
    }
    CCoveredPowerup() {}
    CCoveredPowerup(CGameObject* obj);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TILETRIGGER_H
