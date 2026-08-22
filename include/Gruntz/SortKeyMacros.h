#ifndef GRUNTZ_SORTKEYMACROS_H
#define GRUNTZ_SORTKEYMACROS_H

#define SET_SORT_KEY_IF_CHANGED(object, key)                                                       \
    if (object->m_sortKey != key) {                                                                \
        object->m_sortKey = key;                                                                   \
        object->m_flags |= 0x20000;                                                                \
    }

#define SET_SORT_KEY_IF_CHANGED_AS(object, compareKey, assignedKey)                                \
    if (object->m_sortKey != compareKey) {                                                         \
        object->m_sortKey = assignedKey;                                                           \
        object->m_flags |= 0x20000;                                                                \
    }

#endif // GRUNTZ_SORTKEYMACROS_H
