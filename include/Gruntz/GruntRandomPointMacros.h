#ifndef GRUNTZ_GRUNTRANDOMPOINTMACROS_H
#define GRUNTZ_GRUNTRANDOMPOINTMACROS_H

#define SELECT_RANDOM_EXTENT_POINT_SPANS_FIRST(object, spanX, spanY, outX, outY)                   \
    i32 spanX = abs(object->m_extent.right - object->m_extent.left);                               \
    i32 spanY = abs(object->m_extent.bottom - object->m_extent.top);                               \
    i32 outX = object->m_extent.left;                                                              \
    i32 outY = object->m_extent.top;                                                               \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

#define SELECT_RANDOM_EXTENT_POINT_SIGNED_BASE(object, outX, spanX, outY, spanY)                   \
    i32 outX = object->m_extent.left;                                                              \
    i32 spanX = abs(object->m_extent.right - outX);                                                \
    i32 outY = object->m_extent.top;                                                               \
    i32 spanY = abs(object->m_extent.bottom - outY);                                               \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

#define SELECT_RANDOM_EXTENT_POINT_SIGNED_OUTPUT(object, baseX, spanX, baseY, spanY, outX, outY)   \
    i32 baseX = object->m_extent.left;                                                             \
    i32 spanX = abs(object->m_extent.right - baseX);                                               \
    i32 baseY = object->m_extent.top;                                                              \
    i32 spanY = abs(object->m_extent.bottom - baseY);                                              \
    i32 outX = baseX;                                                                              \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    i32 outY = baseY;                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

#define SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(object, outX, spanX, outY, spanY)                 \
    u32 outX = static_cast<u32>(object->m_extent.left);                                            \
    i32 spanX = abs(object->m_extent.right - static_cast<i32>(outX));                              \
    u32 outY = static_cast<u32>(object->m_extent.top);                                             \
    i32 spanY = abs(object->m_extent.bottom - static_cast<i32>(outY));                             \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

#define SELECT_RANDOM_EXTENT_POINT_UNSIGNED_ASSIGN(object, outX, spanX, outY, spanY)               \
    u32 outX = object->m_extent.left;                                                              \
    i32 spanX = abs(object->m_extent.right - static_cast<i32>(outX));                              \
    u32 outY = object->m_extent.top;                                                               \
    i32 spanY = abs(object->m_extent.bottom - static_cast<i32>(outY));                             \
    if (spanX != 0) {                                                                              \
        outX = outX + rand() % spanX;                                                              \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY = outY + rand() % spanY;                                                              \
    }

#define SELECT_RANDOM_EXTENT_POINT_SPLIT_ABS(object, outX, spanX, outY, spanY)                     \
    i32 outX = object->m_extent.left;                                                              \
    i32 spanX = object->m_extent.right - outX;                                                     \
    i32 outY = object->m_extent.top;                                                               \
    spanX = abs(spanX);                                                                            \
    i32 spanY = object->m_extent.bottom - outY;                                                    \
    spanY = abs(spanY);                                                                            \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

#endif // GRUNTZ_GRUNTRANDOMPOINTMACROS_H
