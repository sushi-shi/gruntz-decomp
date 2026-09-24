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
    char equal = unit->IsAnimationAct(name);
    if (equal) {
        eligible = 0;
    }
}

static inline bool UpdateBattlezSpecialEligibility(CGrunt* unit, i32& eligible) {
    ExcludeBattlezSpecialAct(unit, "I", eligible);
    ExcludeBattlezSpecialAct(unit, "G", eligible);
    ExcludeBattlezSpecialAct(unit, "L", eligible);
    char equal = unit->IsAnimationAct("P");
    if (equal) {
        return false;
    }
    ExcludeBattlezSpecialAct(unit, "J", eligible);
    ExcludeBattlezSpecialAct(unit, "C", eligible);
    ExcludeBattlezSpecialAct(unit, "R", eligible);
    return true;
}

static inline void BuildUnitSearchBox(CGrunt* unit, RECT* box, i32 radius) {
    i32 bottom;
    i32 right;
    i32 top;
    i32 left;
    {
        Coord bottomProbe;
        Coord rightProbe;
        Coord topProbe;
        Coord leftProbe;
        unit->GetScreenTile(&bottomProbe);
        leftProbe.m_x = bottomProbe.m_x;
        bottom = bottomProbe.m_y;
        unit->GetScreenTile(&rightProbe);
        leftProbe.m_y = rightProbe.m_y;
        right = rightProbe.m_x;
        unit->GetScreenTile(&topProbe);
        leftProbe.m_x = topProbe.m_x;
        top = topProbe.m_y;
        unit->GetScreenTile(&leftProbe);
        left = leftProbe.m_x;
    }
    box->left = left - radius;
    box->top = top - radius;
    box->right = right + radius;
    box->bottom = bottom + radius;
}

#endif // GRUNTZ_GRUNTZ_BATTLEZGRUNTINLINE_H
