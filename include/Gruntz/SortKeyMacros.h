#ifndef GRUNTZ_SORTKEYMACROS_H
#define GRUNTZ_SORTKEYMACROS_H

#include <Wwd/WwdGameObjectFlags.h>

#define SET_SORT_KEY_IF_CHANGED(object, key)                                                       \
    if (object->m_sortKey != key) {                                                                \
        object->m_sortKey = key;                                                                   \
        object->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);                                 \
    }

#endif // GRUNTZ_SORTKEYMACROS_H
