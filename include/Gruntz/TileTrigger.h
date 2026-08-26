#ifndef GRUNTZ_TILETRIGGER_H
#define GRUNTZ_TILETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CTileSecretTrigger : public CTileTrigger {
public:
    RVA(0x00011500, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILESECRETTRIGGER;
    }
    CTileSecretTrigger() {}
    CTileSecretTrigger(CGameObject* obj);
};

class CGiantRock : public CTileTrigger {
public:
    RVA(0x000115c0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GIANTROCK;
    }
    CGiantRock() {}
    CGiantRock(CGameObject* obj);
};

class CCoveredPowerup : public CTileTrigger {
public:
    RVA(0x00011680, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_COVEREDPOWERUP;
    }
    CCoveredPowerup() : CTileTrigger(CUserLogic::INLINE_BASE) {}
    CCoveredPowerup(CGameObject* obj);
};

#endif // GRUNTZ_TILETRIGGER_H
