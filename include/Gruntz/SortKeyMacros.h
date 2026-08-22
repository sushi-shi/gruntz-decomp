#ifndef GRUNTZ_SORTKEYMACROS_H
#define GRUNTZ_SORTKEYMACROS_H

// A two-key twin existed for one site in RollingBall.cpp, whose only difference
// was writing the same sum as `base + snapY` at the compare and `snapY + base`
// at the assign; folding it away 2026-08-22 was byte-neutral.
#define SET_SORT_KEY_IF_CHANGED(object, key)                                                       \
    if (object->m_sortKey != key) {                                                                \
        object->m_sortKey = key;                                                                   \
        object->m_flags |= 0x20000;                                                                \
    }

#endif // GRUNTZ_SORTKEYMACROS_H
