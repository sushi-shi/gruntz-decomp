#ifndef GRUNTZ_GRUNTZ_BATTLEZGRUNTINLINE_H
#define GRUNTZ_GRUNTZ_BATTLEZGRUNTINLINE_H

#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>

#include <string.h>

static inline bool HasAnimationActName(CGrunt* unit, const char* name) {
    return strcmp(*g_typeColl.GetNameRecord(unit->m_logicRecord->m_eventCode), name) == 0;
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
