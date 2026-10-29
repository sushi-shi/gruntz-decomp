#ifndef GRUNTZ_GRUNTZ_BATTLEZGRUNTINLINE_H
#define GRUNTZ_GRUNTZ_BATTLEZGRUNTINLINE_H

#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>

static inline bool BattlezActDiffersFromIGLPJCR(CGrunt* unit) {
    if (unit->IsAnimationAct("I")) {
        return false;
    }
    if (unit->IsAnimationAct("G")) {
        return false;
    }
    if (unit->IsAnimationAct("L")) {
        return false;
    }
    if (unit->IsAnimationAct("P")) {
        return false;
    }
    if (unit->IsAnimationAct("J")) {
        return false;
    }
    if (unit->IsAnimationAct("C")) {
        return false;
    }
    if (unit->IsAnimationAct("R")) {
        return false;
    }
    return true;
}

static inline bool BattlezActDiffersFromCRCGLPJ(CGrunt* unit) {
    if (!unit->IsNotAnimationAct("C")) {
        return false;
    }
    if (!unit->IsNotAnimationAct("R")) {
        return false;
    }
    if (!unit->IsNotAnimationAct("C")) {
        return false;
    }
    if (!unit->IsNotAnimationAct("G")) {
        return false;
    }
    if (!unit->IsNotAnimationAct("L")) {
        return false;
    }
    if (!unit->IsNotAnimationAct("P")) {
        return false;
    }
    if (!unit->IsNotAnimationAct("J")) {
        return false;
    }
    return true;
}

static inline void ExcludeBattlezSpecialAct(CGrunt* unit, const char* name, i32& eligible) {
    if (unit->IsAnimationAct(name)) {
        eligible = 0;
    }
}

static inline bool UpdateBattlezSpecialEligibility(CGrunt* unit, i32& eligible) {
    ExcludeBattlezSpecialAct(unit, "I", eligible);
    ExcludeBattlezSpecialAct(unit, "G", eligible);
    ExcludeBattlezSpecialAct(unit, "L", eligible);
    if (unit->IsAnimationAct("P")) {
        return false;
    }
    ExcludeBattlezSpecialAct(unit, "J", eligible);
    ExcludeBattlezSpecialAct(unit, "C", eligible);
    ExcludeBattlezSpecialAct(unit, "R", eligible);
    return true;
}

inline void CGrunt::SetBrickPickupType(PickupType brickPickupType) {
    m_brickPickupType = brickPickupType;
}

inline void CGrunt::SetPendingPickupType(PickupType pendingPickupType) {
    m_pendingPickupType = pendingPickupType;
}

#endif // GRUNTZ_GRUNTZ_BATTLEZGRUNTINLINE_H
